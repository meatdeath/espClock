

#include "rtc.h"
 
RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
 
void rtc_Init(void) {
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
}

void rtc_GetDT(int8_t hour_offset, int8_t minute_offset, DateTime *dst_dt) {
    *dst_dt = rtc.now() + TimeSpan( 0, hour_offset, minute_offset, 0 );
}

void rtc_SetDT(DateTime dt) {
    rtc.adjust(dt);
}

void rtc_SetEpoch(uint32_t epoch_time) {
    rtc.adjust( DateTime(epoch_time) );
}