#ifndef __RTC_H__
#define __RTC_H__

#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"

#define RTC_SQW_PIN 12

void rtc_Init(void);
void rtc_GetDT(DateTime *dst_dt);
void rtc_SetEpoch(uint32_t epoch_time);
bool rtc_wasUpdated(void);
void rtc_updated(void);

enum sw_timers_en {
    SW_TIMER_SENSOR_UPDATE = 0,
    SW_TIMER_SWITCH_DISPLAY,
    SW_TIMER_MAX
};

typedef struct soft_timer_st {
    bool triggered;
    uint16_t update_time;
    uint16_t downcounter;
} soft_timer_t;

extern volatile unsigned long rtc_SecondsSinceUpdate;
extern DateTime rtc_dt;
extern volatile soft_timer_t sw_timer[SW_TIMER_MAX];

#endif //__RTC_H__
