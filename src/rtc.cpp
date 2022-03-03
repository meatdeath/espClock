

#include "rtc.h"
#include "display.h"
 
RTC_DS3231 rtc;

DateTime rtc_dt;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
 

volatile uint16_t rtc_SecondsSinceUpdate;

volatile bool local_time_updated = false;


#define DEFAULT_SENSOR_UPDATE_TIME  60 // in seconds (max 255)
//#define COLLECT_PRESSURE_HISTORY_PERIOD 60*120//60*30   // add point to pressure history period = 30min


uint8_t sensor_update_time = DEFAULT_SENSOR_UPDATE_TIME;

SoftTimer swTimer[SW_TIMER_MAX];

void rtc_InitSoftTimers() {
    swTimer[SW_TIMER_SENSOR_UPDATE].Init(true,sensor_update_time,sensor_update_time,false,true,SW_TIMER_PRECISION_S);
    swTimer[SW_TIMER_RTC_MODULE_UPDATE].Init(true,60,60,false,true,SW_TIMER_PRECISION_S);
    swTimer[SW_TIMER_GET_TIME_FROM_RTC_MODULE].Init(true,60,10,true,true,SW_TIMER_PRECISION_S);
    swTimer[SW_TIMER_NTP_TIME_UPDATE].Init(true,60,30,true,true,SW_TIMER_PRECISION_S);
    swTimer[SW_TIMER_SWITCH_DISPLAY].Init(true,CLOCK_SHOW_TIME,CLOCK_SHOW_TIME,true,true,SW_TIMER_PRECISION_S);
    swTimer[SW_TIMER_COLLECT_PRESSURE_HISTORY].Init(true,COLLECT_PRESSURE_HISTORY_PERIOD,COLLECT_PRESSURE_HISTORY_PERIOD,true,true,SW_TIMER_PRECISION_S);
    swTimer[SW_TIMER_GET_AMBIANCE].Init(true,1,1,true,true,SW_TIMER_PRECISION_MS);
}

SoftTimer::SoftTimer() {
    this->active = false;
    this->triggered = false;
    this->autoupdate = false;
    this->updatetime = 0;
    this->downcounter = 0;
    this->precision = SW_TIMER_PRECISION_S;
}

void SoftTimer::Init(   bool active,
                        uint16_t updatetime, 
                        uint16_t downcounter, 
                        bool triggered,
                        bool autoupdate,
                        sw_timer_precision_t precision)
{
    this->active = active;
    this->triggered = triggered;
    this->autoupdate = autoupdate;
    this->updatetime = updatetime;
    this->downcounter = downcounter;
    this->precision = precision;
}

void SoftTimer::SetTriggered( bool triggered )
{
    noInterrupts();
    this->triggered = triggered;
    interrupts();
}

bool SoftTimer::IsTriggered( bool autoreset )
{
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

void SoftTimer::SetDowncounter( uint16_t downcounter )
{
    noInterrupts();
    this->downcounter = downcounter;
    interrupts();
}

uint16_t SoftTimer::GetDowncounter()
{
    uint16_t downcounter;
    noInterrupts();
    downcounter = this->downcounter;
    interrupts();
    return downcounter;
}

void SoftTimer::SetUpdateTime( uint16_t update_time)
{
    noInterrupts();
    this->updatetime = update_time;
    interrupts();
}

uint16_t SoftTimer::GetUpdateTime()
{
    uint16_t updatetime;
    noInterrupts();
    updatetime = this->updatetime;
    interrupts();
    return updatetime;
}

void SoftTimer::_Tick()
{
    if( this->active && this->downcounter ) {
        this->downcounter--;
        if( this->downcounter == 0 ) {
            this->triggered = true;
            if(this->autoupdate) {
                this->downcounter = this->updatetime;
            }
        }
    } 
}

IRAM_ATTR void time_tick500ms() {
    //Serial.println("Enter pin interrupt");
    if( digitalRead(RTC_SQW_PIN) ) {
        rtc_SecondsSinceUpdate++;
        for( int i = 0; i < SW_TIMER_MAX; i++ ) {
            swTimer[i]._Tick();
        }
    }
    //Serial.printf("Seconds since last sync: %ld\r\n", rtc_SecondsSinceUpdate);
    local_time_updated = true;
}

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
        // This line sets the RTC with an explicit date & time, for example to set
        // January 21, 2014 at 3am you would call:
        // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }
    DateTime dt = rtc.now();
    Serial.printf("RTC init time: %02d:%02d:%02d\r\n", dt.hour(), dt.minute(), dt.second());
    rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
    rtc_SecondsSinceUpdate = 0;
    
    
    delay(2000);
    Serial.println("Init pin interrupt");
    pinMode(RTC_SQW_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(RTC_SQW_PIN), time_tick500ms, CHANGE);
    Serial.println("Init pin interrupt done");

}

void rtc_GetDT(DateTime *dst_dt) {
    Serial.println("rtc_GetDT");
    *dst_dt = rtc.now();
    rtc_SecondsSinceUpdate = 0;
}

void rtc_SetDT(DateTime dt) {
    rtc.adjust(dt);
    rtc_SecondsSinceUpdate = 0;
    Serial.println("rtc_SetDT");
}

void rtc_SetEpoch(uint32_t epoch_time) {
    rtc.adjust( DateTime(epoch_time) );
    rtc_SecondsSinceUpdate = 0;
    //Serial.println("rtc_SetEpoch");
}

