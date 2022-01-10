

#include "rtc.h"
#include "display.h"
 
RTC_DS3231 rtc;

DateTime rtc_dt;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
 

volatile unsigned long rtc_SecondsSinceUpdate;

volatile bool local_time_updated = false;


#define DEFAULT_SENSOR_UPDATE_TIME  60 // in seconds (max 255)
#define COLLECT_PRESSURE_HISTORY_PERIOD 60*120//60*30   // add point to pressure history period = 30min


uint8_t sensor_update_time = DEFAULT_SENSOR_UPDATE_TIME;

volatile soft_timer_t sw_timer[SW_TIMER_MAX] = {
    {   // SW_TIMER_SENSOR_UPDATE
        .triggered = false,
        .autoupdate = true,
        .updatetime = sensor_update_time,
        .downcounter = sensor_update_time
    },
    {   // SW_TIMER_GET_TIME_FROM_RTC_MODULE
        .triggered = true,
        .autoupdate = true,
        .updatetime = 60, // each 1 min
        .downcounter = 10
    },
    {   // SW_TIMER_RTC_MODULE_UPDATE,
        .triggered = false,
        .autoupdate = true,
        .updatetime = 60, // each 1 min
        .downcounter = 60
    },
    {   // SW_TIMER_NTP_TIME_UPDATE,
        .triggered = true,
        .autoupdate = true,
        .updatetime = 60, // each 1 min
        .downcounter = 30
    },
    {   // SW_TIMER_SWITCH_DISPLAY
        .triggered = false,
        .autoupdate = true,
        .updatetime = CLOCK_SHOW_TIME,
        .downcounter = CLOCK_SHOW_TIME
    },
    {   // SW_TIMER_COLLECT_PRESSURE_HISTORY
        .triggered = true,
        .autoupdate = true,
        .updatetime = COLLECT_PRESSURE_HISTORY_PERIOD,
        .downcounter = COLLECT_PRESSURE_HISTORY_PERIOD
    },
    {   // SW_TIMER_GET_AMBIANCE
        .triggered = true,
        .autoupdate = true,
        .updatetime = 1,
        .downcounter = 1
    }
};

void swTimerSetTriggered( enum sw_timers_en sw_timer_index, bool value )
{
    sw_timer[sw_timer_index].triggered = value;
}

bool swTimerIsTriggered( enum sw_timers_en sw_timer_index, bool autoreset )
{
    bool triggered = false;
    if( sw_timer[sw_timer_index].triggered ) {
        triggered = true;
        if( autoreset )
            sw_timer[sw_timer_index].triggered = false;
    }
    return triggered;
}


IRAM_ATTR void time_tick500ms() {
    //Serial.println("Enter pin interrupt");
    if( digitalRead(RTC_SQW_PIN) ) {
        rtc_SecondsSinceUpdate++;
        for( int i = 0; i < SW_TIMER_MAX; i++ ) {
            if( sw_timer[i].downcounter ) {
                sw_timer[i].downcounter--;
                if( sw_timer[i].downcounter == 0 ) {
                    sw_timer[i].triggered = true;
                    if(sw_timer[i].autoupdate) {
                        sw_timer[i].downcounter = sw_timer[i].updatetime;
                    }
                }
            }
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
    
    Serial.println("Init pin interrupt");
    pinMode(RTC_SQW_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(RTC_SQW_PIN), time_tick500ms, CHANGE);
    Serial.println("Init pin interrupt done");

    delay(3000); // wait for console opening
 
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
    delay(1000);
    DateTime dt = rtc.now();
    Serial.printf("RTC init time: %02d:%02d:%02d\r\n", dt.hour(), dt.minute(), dt.second());
    rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
    rtc_SecondsSinceUpdate = 0;
}

void rtc_GetDT(DateTime *dst_dt) {
    //Serial.println("rtc_GetDT");
    *dst_dt = rtc.now();
    rtc_SecondsSinceUpdate = 0;
}

void rtc_SetDT(DateTime dt) {
    rtc.adjust(dt);
    rtc_SecondsSinceUpdate = 0;
    //Serial.println("rtc_SetDT");
}

void rtc_SetEpoch(uint32_t epoch_time) {
    rtc.adjust( DateTime(epoch_time) );
    rtc_SecondsSinceUpdate = 0;
    //Serial.println("rtc_SetEpoch");
}

