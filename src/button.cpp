#include "button.h"
#include "rtc.h"
#include "config.h"

#define BUTTON_PIN  16

typedef enum button_state_en {
    BUTTON_STATE_DOWN = 0,
    BUTTON_STATE_UP = 1
} button_state_t;


#define BUTTON_EVENT_PRESSED            (1<<0)
#define BUTTON_EVENT_SHORT_PRESS        (1<<1)
#define BUTTON_EVENT_MIDDLE_PRESS       (1<<2)
#define BUTTON_EVENT_LONG_PRESS         (1<<3)
#define BUTTON_EVENT_LONG_LONG_PRESS    (1<<4)

void button_Init() 
{
    pinMode( BUTTON_PIN, INPUT_PULLDOWN_16 );
}

void button_InitTimer(uin16_t timeout)
{
    swTimer[SW_TIMER_BUTTON].Init("Button",true,timeout,timeout);
}

void button_Process() 
{
    int button_event = 0;
    static int btn_evt = 0;

    if( digitalRead(BUTTON_PIN) == HIGH ) {
        if( !swTimer[SW_TIMER_BUTTON].isActive() ) {
            Serial.print("press");
            button_InitTimer(3);
            btn_evt = BUTTON_EVENT_SHORT_PRESS;
            Serial.print(btn_evt);
        } else if( swTimer[SW_TIMER_BUTTON].IsTriggered(true) ) {
            Serial.print("B_timeout_");
            switch(btn_evt) {
                case BUTTON_EVENT_SHORT_PRESS: 
                    btn_evt = BUTTON_EVENT_MIDDLE_PRESS;
                    button_InitTimer(7);
                    Serial.print(btn_evt);
                    break;
                case BUTTON_EVENT_MIDDLE_PRESS: 
                    btn_evt = BUTTON_EVENT_LONG_PRESS;
                    button_InitTimer(10);
                    Serial.print(btn_evt);
                    break;
                case BUTTON_EVENT_LONG_PRESS: 
                    btn_evt = BUTTON_EVENT_LONG_LONG_PRESS;
                    button_InitTimer(10);
                    Serial.print(btn_evt);
                    break;
            }
        }
    } else {
        if( swTimer[SW_TIMER_BUTTON].isActive() ) {
            swTimer[SW_TIMER_BUTTON].Init("Button",false,0,0);
            button_event = btn_evt;
            btn_evt = 0;
        }
    }
    switch( button_event ) {
        case BUTTON_EVENT_SHORT_PRESS: Serial.println("Button SHORT press"); break;
        case BUTTON_EVENT_MIDDLE_PRESS: Serial.println("Button MIDDLE press"); break;
        case BUTTON_EVENT_LONG_PRESS: Serial.println("Button LONG press"); config_setWiFiSettings("",""); break;
        case BUTTON_EVENT_LONG_LONG_PRESS: Serial.println("Button LONG LONG press"); break;
    }

}