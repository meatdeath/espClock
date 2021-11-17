

#include "rtc.h"
 
RTC_DS3231 rtc;

DateTime rtc_dt;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
 

volatile unsigned long rtc_SecondsSinceUpdate;

volatile bool was_updated = false;

IRAM_ATTR void time_tick500ms() {
    //Serial.println("Enter pin interrupt");
    if( digitalRead(RTC_SQW_PIN) )
        rtc_SecondsSinceUpdate++;
    Serial.printf("Seconds since last sync: %ld\r\n", rtc_SecondsSinceUpdate);
    was_updated = true;
}

bool rtc_wasUpdated(void) {
    return was_updated;
}

void rtc_updated(void) {
    was_updated = false;
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
    was_updated = true;
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
    Serial.println("rtc_SetEpoch");
}

