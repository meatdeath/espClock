#ifndef __RTC_H__
#define __RTC_H__

#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"
#include "pressure_history.h"

#define RTC_SQW_PIN 0

void rtc_Init(void);
void rtc_GetDT(DateTime *dst_dt);
void rtc_SetEpoch(uint32_t epoch_time);
bool rtc_LocalTimeRequireProcessing(void);
void rtc_SetLocalTimeProcessed(void);

#define RTC_SECONDS_2000_01_01   946684800L

enum sw_timers_en {
    SW_TIMER_SENSOR_UPDATE = 0,
    SW_TIMER_RTC_MODULE_UPDATE,
    SW_TIMER_GET_TIME_FROM_RTC_MODULE,
    SW_TIMER_NTP_TIME_UPDATE,
    SW_TIMER_SWITCH_DISPLAY,
    SW_TIMER_COLLECT_PRESSURE_HISTORY,
    SW_TIMER_GET_AMBIANCE,
    SW_TIMER_BUTTON,
    SW_TIMER_MAX
};

typedef enum sw_timer_precision_en {
    SW_TIMER_PRECISION_S = 0,
    SW_TIMER_PRECISION_MS
} sw_timer_precision_t;

class SoftTimer {
    public:
        SoftTimer();
        void Init(  bool active = false,
                    uint16_t updatetime = 0, 
                    uint16_t downcounter = 0, 
                    bool triggered = false,
                    bool autoupdate = false,
                    sw_timer_precision_t precision = SW_TIMER_PRECISION_S);
        void SetTriggered( bool triggered );
        bool IsTriggered( bool autoreset );
        void SetDowncounter( uint16_t downcounter );
        uint16_t GetDowncounter();
        void SetUpdateTime( uint16_t update_time);
        uint16_t GetUpdateTime();
        void _Tick();
        bool isActive();
    private:
        bool active;
        bool triggered;
        bool autoupdate;
        uint16_t updatetime;
        uint16_t downcounter;
        sw_timer_precision_t precision;
};


extern volatile uint16_t rtc_SecondsSinceUpdate;
extern DateTime rtc_dt;
extern SoftTimer swTimer[SW_TIMER_MAX];

#endif //__RTC_H__
