#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <Arduino.h>

typedef enum button_state_en {
    BUTTON_STATE_DOWN = 0,
    BUTTON_STATE_UP = 1
} button_state_t;

#define BUTTON_EVENT_PRESSED            (1<<0)
#define BUTTON_EVENT_SHORT_PRESS        (1<<1)
#define BUTTON_EVENT_MIDDLE_PRESS       (1<<2)
#define BUTTON_EVENT_LONG_PRESS         (1<<3)
#define BUTTON_EVENT_LONG_LONG_PRESS    (1<<4)

// Short press 0-2 sec
#define BUTTON_SHORT_PRESS_TICKS_LOW    0   // 4*0,5s = 2s
#define BUTTON_SHORT_PRESS_TICKS_HIGH   3   // 4*0,5s = 2s
// Middle press 2-6 sec
#define BUTTON_MIDDLE_PRESS_TICKS_LOW   (BUTTON_SHORT_PRESS_TICKS_HIGH+1)   // 4*0,5s = 2s
#define BUTTON_MIDDLE_PRESS_TICKS_HIGH  (BUTTON_MIDDLE_PRESS_TICKS_LOW+7)   // 4*0,5s = 2s
// Long press 6-12 sec
#define BUTTON_LONG_PRESS_TICKS_LOW     (BUTTON_MIDDLE_PRESS_TICKS_HIGH+1)   // 4*0,5s = 2s
#define BUTTON_LONG_PRESS_TICKS_HIGH    (BUTTON_LONG_PRESS_TICKS_LOW+11)   // 4*0,5s = 2s
// Long press 12-20 sec
#define BUTTON_LONG_LONG_PRESS_TICKS_LOW     (BUTTON_LONG_PRESS_TICKS_HIGH+1)   // 4*0,5s = 2s
#define BUTTON_LONG_LONG_PRESS_TICKS_HIGH    (BUTTON_LONG_LONG_PRESS_TICKS_LOW+15)   // 4*0,5s = 2s

#endif // __BUTTON_H__
