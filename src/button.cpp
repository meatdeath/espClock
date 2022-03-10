#include "button.h"

#define BUTTON_PIN  8


class Button {
    private:
        uint8_t pin;
        uint8_t state;
        uint8_t old_state;
        uint8_t event;
    public:
        Button(uint8_t pin);
        void Init(uint8_t pin);
        uint8_t GetState();
        void UpdateTick();
};

Button::Button(uint8_t pin) {
    pinMode( pin, INPUT_PULLUP );
    pin = pin;
    state = BUTTON_STATE_UP;
    old_state = BUTTON_STATE_UP;
    event = 0;
}

void Button::Init(uint8_t pin) {
    pinMode( pin, INPUT_PULLUP );
    pin = pin;
    state = BUTTON_STATE_UP;
    old_state = BUTTON_STATE_UP;
    event = 0;
}

uint8_t Button::GetState() {
    return state;
}

void Button::UpdateTick() {
    old_state = state;
    state = digitalRead(pin);
    if( state == BUTTON_STATE_DOWN && old_state == BUTTON_STATE_UP ) {
        event |= BUTTON_EVENT_PRESSED;

    }
}
