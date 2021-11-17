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

extern volatile unsigned long rtc_SecondsSinceUpdate;
extern DateTime rtc_dt;

#endif //__RTC_H__
