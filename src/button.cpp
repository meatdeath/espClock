#include "button.h"

#define BUTTON_PIN  8

uint8_t button_state = BUTTON_STATE_UP;
uint8_t button_oldState = BUTTON_STATE_UP;
uint8_t button_event = 0;

void button_init() {
    pinMode( BUTTON_PIN, INPUT_PULLUP );
    button_state = BUTTON_STATE_UP;
    button_oldState = BUTTON_STATE_UP;
    button_event = 0;
}

uint8_t button_getState() {
    return button_state;
}

void button_updateTick() {
    button_oldState = button_state;
    button_state = digitalRead(BUTTON_PIN);
    if( button_state == BUTTON_STATE_DOWN && button_oldState == BUTTON_STATE_UP ) {
        button_event |= BUTTON_EVENT_PRESSED;
    }
}
