#include "display.h"

LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CS_PIN);

void display_init(void) {
    ledMatrix.init();
    ledMatrix.setText("23:47");
    ledMatrix.setTextOffset(32);
}

void display_brightness(uint8_t percentage) {
    if( percentage <= 6)       ledMatrix.setIntensity(0);
    else if( percentage <= 12) ledMatrix.setIntensity(1);
    else if( percentage <= 18) ledMatrix.setIntensity(2);
    else if( percentage <= 25) ledMatrix.setIntensity(3);
    else if( percentage <= 31) ledMatrix.setIntensity(4);
    else if( percentage <= 37) ledMatrix.setIntensity(5);
    else if( percentage <= 43) ledMatrix.setIntensity(6);
    else if( percentage <= 50) ledMatrix.setIntensity(7);
    else if( percentage <= 56) ledMatrix.setIntensity(8);
    else if( percentage <= 62) ledMatrix.setIntensity(9);
    else if( percentage <= 68) ledMatrix.setIntensity(10);
    else if( percentage <= 75) ledMatrix.setIntensity(11);
    else if( percentage <= 81) ledMatrix.setIntensity(12);
    else if( percentage <= 87) ledMatrix.setIntensity(13);
    else if( percentage <= 93) ledMatrix.setIntensity(14);
    else                       ledMatrix.setIntensity(15);
}

void display_printpressure(uint16_t pressure) {
    uint8_t i, offset = 0;
    uint8_t d1 = (pressure/100)%10;
    uint8_t d2 = (pressure/10)%10;
    uint8_t d3 = pressure%10;
    uint8_t size;
    
    ledMatrix.clear();

    size = pgm_read_byte(&(digits[d1].size));
    for( i = 0; i < size; i++ ) {
        ledMatrix.setColumn(offset+i, pgm_read_byte(&(digits[d1].array[i])) );
    }
    offset += size + 2;

    size = pgm_read_byte(&(digits[d2].size));
    for( i = 0; i < size; i++ ) {
        ledMatrix.setColumn(offset+i, pgm_read_byte(&(digits[d2].array[i])) );
    }
    offset += size + 2;

    size = pgm_read_byte(&(digits[d3].size));
    for( i = 0; i < size; i++ ) {
        ledMatrix.setColumn(offset+i, pgm_read_byte(&(digits[d3].array[i])) );
    }
    offset += size + 2;
    size = pgm_read_byte(&(digits[DISPLAY_MM].size));
    for( i = 0; i < size; i++ ) {
        ledMatrix.setColumn(offset+i, pgm_read_byte(&(digits[DISPLAY_MM].array[i])) );
    }
    ledMatrix.Rotate90();
    ledMatrix.commit();
}

void display_printtemperature(int temperature) {
    uint8_t i, offset = 0;
    bool negative = false;


    ledMatrix.clear();

    uint8_t size;
    if( temperature < 0 ) {
        negative = true;
        temperature = -temperature;
    }

    uint8_t t1 = temperature / 10;
    uint8_t t2 = temperature % 10;
    if( t1 == 0 ) {
        size = pgm_read_byte(&(digits[t1].size));
        for( i = 0; i < size; i++ ) {
            ledMatrix.setColumn(offset+i, 0x00 );
        }
        offset += size + 2;
    }

    size = pgm_read_byte(&(digits[DISPLAY_MINUS].size));
    if( negative ) {
        for( i = 0; i < size; i++ ) {
            ledMatrix.setColumn(offset+i, pgm_read_byte(&(digits[DISPLAY_MINUS].array[i])) );
        }
    } else {
        for( i = 0; i < size; i++ ) {
            ledMatrix.setColumn(offset+i, 0x00);
        }
    }
    offset += size + 2;


    if( t1 ) {
        size = pgm_read_byte(&(digits[t1].size));
        for( i = 0; i < size; i++ ) {
            ledMatrix.setColumn(offset+i, pgm_read_byte(&(digits[t1].array[i])) );
        }
        offset += size + 2;
    }

    size = pgm_read_byte(&(digits[t2].size));
    for( i = 0; i < size; i++ ) {
        ledMatrix.setColumn(offset+i, pgm_read_byte(&(digits[t2].array[i])) );
    }
    offset += size + 2;
    size = pgm_read_byte(&(digits[DISPLAY_CELCIUS].size));
    for( i = 0; i < size; i++ ) {
        ledMatrix.setColumn(offset+i, pgm_read_byte(&(digits[DISPLAY_CELCIUS].array[i])) );
    }
    ledMatrix.Rotate90();
    ledMatrix.commit();

}

void display_printtime(byte hours, byte minutes, byte seconds, byte format) {
    byte i;

    if( format == DISPLAY_FORMAT_12H ) hours %= 12;
    else hours %= 24;

    byte h1 = hours/10;
    byte h2 = hours%10;
    byte m1 = minutes/10;
    byte m2 = minutes%10;
    
    ledMatrix.clear();
#ifndef HIDE_HOUR_LEADING_ZERO
    if( h1 != 0 ) {
#endif
        for( i = 0; i < pgm_read_byte(&(digits[h1].size)); i++ ) {
            ledMatrix.setColumn(i, pgm_read_byte(&(digits[h1].array[i])) );
        }
#ifndef HIDE_HOUR_LEADING_ZERO
    }else {
        for( i = 0; i < pgm_read_byte(&(digits[h1].size)); i++ ) {
            ledMatrix.setColumn(i, 0 );
        }
    }
#endif
    //ledMatrix.setColumn(i,0); // space
    //hours low
    for( i = 0; i < pgm_read_byte(&(digits[h2].size)); i++ ) {
        ledMatrix.setColumn(7+i, pgm_read_byte(&(digits[h2].array[i])) );
    }
    //ledMatrix.setColumn(7+i,0); // space
    //colon
    for( i = 0; i < digits[DISPLAY_SYMBOL_COLON].size; i++ ) {
        ledMatrix.setColumn(15+i, (seconds&1)?pgm_read_byte(&(digits[DISPLAY_SYMBOL_COLON].array[i])):0 );
    }
    //ledMatrix.setColumn(15+i,0); // space
    // minutes high
    for( i = 0; i < pgm_read_byte(&(digits[m1].size)); i++ ) {
        ledMatrix.setColumn(19+i, pgm_read_byte(&(digits[m1].array[i])) );
    }
    //ledMatrix.setColumn(19+i,0); // space
    // minutes low
    for( i = 0; i < pgm_read_byte(&(digits[m2].size)); i++ ) {
        ledMatrix.setColumn(26+i, pgm_read_byte(&(digits[m2].array[i])) );
    }
    ledMatrix.Rotate90();
    ledMatrix.commit();
}

void display_printstarting(void) {
    for( byte i = 0; i < pgm_read_byte(&(digits[DISPLAY_STARTING].size)); i++ ) {
        ledMatrix.setColumn(i, pgm_read_byte(&(digits[DISPLAY_STARTING].array[i])) );
    }
    ledMatrix.Rotate90();
    ledMatrix.commit();
}