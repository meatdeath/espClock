#include "rtc.h"
#include "display.h"
#include "button.h"

//-----------------------------------------------------------------------------

#define DEFAULT_SENSOR_UPDATE_TIME  60

//-----------------------------------------------------------------------------

uint8_t sensor_update_time = DEFAULT_SENSOR_UPDATE_TIME;
volatile uint16_t rtc_SecondsSinceUpdate;
volatile bool local_time_updated = false;
SoftTimer swTimer[SW_TIMER_MAX];
RTC_DS3231 rtc;
DateTime rtc_dt;
static unsigned long last_time_ms;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void rtc_InitSoftTimers() {
    swTimer[SW_TIMER_SENSOR_UPDATE           ].Init("Sens_upd",true,sensor_update_time,sensor_update_time,false,true,SW_TIMER_PRECISION_S);
    swTimer[SW_TIMER_RTC_MODULE_UPDATE       ].Init("RTC_modul_upd",true,60,60,false,true,SW_TIMER_PRECISION_S);
    swTimer[SW_TIMER_GET_TIME_FROM_RTC_MODULE].Init("Get_RTC_time",true,60,20,true,true,SW_TIMER_PRECISION_S);
    swTimer[SW_TIMER_NTP_TIME_UPDATE         ].Init("Get_NTP_time",true,67,30,true,true,SW_TIMER_PRECISION_S);
    swTimer[SW_TIMER_SWITCH_DISPLAY          ].Init("Switch_disp",true,2,2,false,true,SW_TIMER_PRECISION_S);
    swTimer[SW_TIMER_COLLECT_PRESSURE_HISTORY].Init("Collect_P_history",true,COLLECT_PRESSURE_HISTORY_PERIOD,COLLECT_PRESSURE_HISTORY_PERIOD,true,true,SW_TIMER_PRECISION_S);
    swTimer[SW_TIMER_GET_AMBIANCE            ].Init("Get_ambiance",true,50,50,true,true,SW_TIMER_PRECISION_MS);
    swTimer[SW_TIMER_BUTTON                  ].Init("Button",false,0,0,false,false,SW_TIMER_PRECISION_S);
    last_time_ms = millis();
}

SoftTimer::SoftTimer() {
    this->active = false;
    this->triggered = false;
    this->autoupdate = false;
    this->updatetime = 0;
    this->downcounter = 0;
    this->precision = SW_TIMER_PRECISION_S;
}

void SoftTimer::Init(   const char *name,
                        bool active,
                        uint16_t updatetime, 
                        uint16_t downcounter, 
                        bool triggered,
                        bool autoupdate,
                        sw_timer_precision_t precision) {
    strncpy(this->name, name, TIMER_NAME_SIZE);
    this->active = active;
    this->triggered = triggered;
    this->autoupdate = autoupdate;
    this->updatetime = updatetime;
    this->downcounter = downcounter;
    this->precision = precision;
}

void SoftTimer::SetTriggered( bool triggered ) {
    noInterrupts();
    this->triggered = triggered;
    interrupts();
}

bool SoftTimer::IsTriggered( bool autoreset ) {
    bool triggered = false;
    noInterrupts();
    if( this->triggered ) {
        triggered = true;
        if( autoreset )
            this->triggered = false;
    }
    interrupts();
    return triggered;
}

void SoftTimer::SetDowncounter( uint16_t downcounter ) {
    noInterrupts();
    this->downcounter = downcounter;
    interrupts();
}

uint16_t SoftTimer::GetDowncounter() {
    uint16_t downcounter;
    noInterrupts();
    downcounter = this->downcounter;
    interrupts();
    return downcounter;
}

void SoftTimer::SetUpdateTime( uint16_t update_time) {
    noInterrupts();
    this->updatetime = update_time;
    interrupts();
}

uint16_t SoftTimer::GetUpdateTime() {
    uint16_t updatetime;
    noInterrupts();
    updatetime = this->updatetime;
    interrupts();
    return updatetime;
}

void SoftTimer::_TickS() {
    if( this->active && this->precision == SW_TIMER_PRECISION_S && this->downcounter ) {
        this->downcounter--;
        if( this->downcounter == 0 ) {
        // Serial.printf("#%s#\n",this->name);
            this->triggered = true;
            if(this->autoupdate) {
                this->downcounter = this->updatetime;
            }
        }
    } 
}

void SoftTimer::HandleTickMs() {
    unsigned long time_ms = millis()-last_time_ms;
    if( this->active && this->precision == SW_TIMER_PRECISION_MS && this->downcounter ) {
        if( this->downcounter > time_ms)
            this->downcounter -= time_ms;
        else
            this->downcounter = 0;
        if( this->downcounter == 0 ) {
            this->triggered = true;
            if(this->autoupdate) {
                this->downcounter = this->updatetime;
            }
        }
    } 
    last_time_ms += time_ms;
}

bool SoftTimer::isActive() {
    return this->active;
}

IRAM_ATTR void time_tick500ms() {
    //Serial.println("Enter pin interrupt");
    if( digitalRead(RTC_SQW_PIN) ) {
        rtc_SecondsSinceUpdate++;
        for( int i = 0; i < SW_TIMER_MAX; i++ ) {
            swTimer[i]._TickS();
        }
    }
    //Serial.printf("Seconds since last sync: %ld\r\n", rtc_SecondsSinceUpdate);
    local_time_updated = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool rtc_LocalTimeRequireProcessing(void) {
    return local_time_updated;
}

void rtc_SetLocalTimeProcessed(void) {
    local_time_updated = false;
}

void rtc_Init(void) {
    rtc_InitSoftTimers();
    delay(100);
    if (! rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while (1);
    }
    if (rtc.lostPower()) {
        Serial.println("RTC lost power, lets set the time!");
        // following line sets the RTC to the date & time this sketch was compiled
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
    delay(500);
    DateTime dt = rtc.now();
    Serial.printf("RTC init time: %02d:%02d:%02d\r\n", dt.hour(), dt.minute(), dt.second());
    rtc_SecondsSinceUpdate = 0;
    
    Serial.print("Init pin interrupt... ");
    pinMode(RTC_SQW_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(RTC_SQW_PIN), time_tick500ms, CHANGE);
    Serial.println("OK");
}

void rtc_GetDT(DateTime *dst_dt) {
    //Serial.println("Get time from RTC module");
    *dst_dt = rtc.now();
    rtc_SecondsSinceUpdate = 0;
}

void rtc_SetDT(DateTime dt) {
    //Serial.println("Adjust RTC module with datetime");
    rtc.adjust(dt);
    rtc_SecondsSinceUpdate = 0;
}

void rtc_SetEpoch(uint32_t epoch_time) {
    //Serial.println("Adjust RTC module with epoch time");
    rtc.adjust( DateTime(epoch_time) );
}

#include <ESPDateTime.h>

extern bool time_sync_with_ntp_enabled;
extern unsigned long ntp_time;

#include "TimeLib.h"

String rtc_GetTimeString() {
    unsigned long timeinsec;
    if( time_sync_with_ntp_enabled ) {
        timeinsec = ntp_time + rtc_SecondsSinceUpdate;
    } else {
        timeinsec = rtc_dt.unixtime() + rtc_SecondsSinceUpdate;
    }

Serial.print("[rtc_GetTimeString] convert epoch seconds... ");
    int iyear  = year(timeinsec);
    int imonth = month(timeinsec);
    int iday   = day(timeinsec);
    int ihour  = hour(timeinsec);
    int iminute = minute(timeinsec);
    int isecond = second(timeinsec);
Serial.println("done");

    String timestr = 
                    String( iyear )+'/'+
                    String((imonth<10)?"0":"")+
                    String( imonth )+'/'+
                    String((iday<10)?"0":"")+
                    String( iday )+' '+
                    String((ihour<10)?"0":"")+
                    String( ihour )+':'+
                    String((iminute<10)?"0":"")+
                    String( iminute )+':'+
                    String((isecond<10)?"0":"")+
                    String( isecond );
    return timestr;
}

//-----------------------------------------------------------------------------
