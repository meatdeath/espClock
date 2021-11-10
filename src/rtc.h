#ifndef __RTC_H__
#define __RTC_H__

#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"

void rtc_Init(void);
void rtc_GetDT(int8_t hour_offset, int8_t minute_offset, DateTime *dst_dt);
void rtc_SetEpoch(uint32_t epoch_time);

#endif //__RTC_H__
