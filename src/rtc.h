#ifndef __RTC_H__
#define __RTC_H__

#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"

#define RTC_SQW_PIN 12

#define DEFAULT_SENSOR_UPDATE_TIME          (60)        // in seconds (max 255)
#define COLLECT_PRESSURE_HISTORY_PERIOD     (2*60*60)   // 60*30   // add point to pressure history period = 30min

#define EPOCH_2000_01_01__12_00_AM  ((unsigned long)946684800)

void rtc_Init(void);
void rtc_GetDT(DateTime *dst_dt);
void rtc_SetEpoch(uint32_t epoch_time);
bool rtc_LocalTimeRequireProcessing(void);
void rtc_SetLocalTimeProcessed(void);

enum sw_timers_en {
    SW_TIMER_SENSOR_UPDATE = 0,
    SW_TIMER_RTC_MODULE_UPDATE,
    SW_TIMER_GET_TIME_FROM_RTC_MODULE,
    SW_TIMER_NTP_TIME_UPDATE,
    SW_TIMER_SWITCH_DISPLAY,
    SW_TIMER_COLLECT_PRESSURE_HISTORY,
    SW_TIMER_GET_AMBIANCE,
    SW_TIMER_WIFI_CONNECTING,
    SW_TIMER_MAX
};

typedef struct soft_timer_st {
    bool enabled;
    bool triggered;
    bool autoupdate;
    uint16_t updatetime;
    uint16_t downcounter;
} soft_timer_t;

void swTimerStart(enum sw_timers_en sw_timer_index, uint16_t updatetime=(uint16_t)-1);
void swTimerStop(enum sw_timers_en sw_timer_index);
void swTimerSetTriggered( enum sw_timers_en sw_timer_index, bool value );
bool swTimerIsTriggered( enum sw_timers_en sw_timer_index, bool autoreset=false );
bool swTimerIsActive( enum sw_timers_en sw_timer_index );

extern volatile unsigned long rtc_SecondsSinceUpdate;
extern DateTime rtc_dt;
extern volatile soft_timer_t sw_timer[SW_TIMER_MAX];

#endif //__RTC_H__
