#include "display.h"

// ----------------------------------------------------------------------------
typedef enum display_orientation_en {
    DISPLAY_ORIENTATION_0 = 0,
    DISPLAY_ORIENTATION_CW90,
    DISPLAY_ORIENTATION_CW180,
    DISPLAY_ORIENTATION_CCW90
} display_orientation_t;

// ----------------------------------------------------------------------------

LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CS_PIN);

display_orientation_t display_orientation = DISPLAY_ORIENTATION_CCW90;

// ----------------------------------------------------------------------------

void display_Init(void) {
    ledMatrix.init();
    ledMatrix.setText("23:47");
    ledMatrix.setTextOffset(32);
}

void display_SetIntensity(byte intensity) {
    ledMatrix.setIntensity(intensity);
}

void display_FixRotation(display_orientation_t orientation) {
    switch(orientation) {
        case DISPLAY_ORIENTATION_0: break;
        case DISPLAY_ORIENTATION_CW90: ledMatrix.Rotate90(); break;
        case DISPLAY_ORIENTATION_CW180: ledMatrix.Rotate90(); ledMatrix.Rotate90(); break;
        case DISPLAY_ORIENTATION_CCW90: ledMatrix.Rotate90(); ledMatrix.Rotate90(); ledMatrix.Rotate90(); break;
    }
}

void display_SetBrightness(uint8_t percentage) {
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

void display_Pressure(uint16_t pressure) {
    uint8_t i, offset = 0;
    uint8_t symbol[] = {
        (uint8_t)((pressure/100)%10),
        (uint8_t)((pressure/10)%10),
        (uint8_t)(pressure%10),
        DISPLAY_MM
    };
    uint8_t size;
    
    ledMatrix.clear();

    for(int j = 0; j < 4; j++) {
        size = pgm_read_byte(&(digits[symbol[j]].size));
        for( i = 0; i < size; i++ ) {
            ledMatrix.setColumn(offset+i, pgm_read_byte(&(digits[symbol[j]].array[i])) );
        }
        offset += size + 2;        
    }
    
    display_FixRotation(display_orientation);
    ledMatrix.commit();
}

void display_Temperature(int temperature) {
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
    display_FixRotation(display_orientation);
    ledMatrix.commit();

}

void display_Time(byte hours, byte minutes, byte seconds, byte format) {
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
    display_FixRotation(display_orientation);
    ledMatrix.commit();
}

void display_PrintStarting(void) {
    for( byte i = 0; i < pgm_read_byte(&(digits[DISPLAY_STARTING].size)); i++ ) {
        ledMatrix.setColumn(i, pgm_read_byte(&(digits[DISPLAY_STARTING].array[i])) );
    }
    display_FixRotation(display_orientation);
    ledMatrix.commit();
}

void display_ClockString(void) {
    for( byte i = 0; i < pgm_read_byte(&(digits[DISPLAY_CLOCK_STR].size)); i++ ) {
        ledMatrix.setColumn(i, pgm_read_byte(&(digits[DISPLAY_CLOCK_STR].array[i])) );
    }
    display_FixRotation(display_orientation);
    ledMatrix.commit();
}

void display_Version(void) {
    const byte v_dot[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,0x94,0x94,0x94,0x68,0x00,0x80,0x00};
    byte j = 0;
    for( byte i = 0; i < sizeof(v_dot); i++, j++ ) {
        ledMatrix.setColumn(j, v_dot[i]);
    }
    for( byte i = 0; i < pgm_read_byte(&(digits[VERSION_MAJOR].size)); i++, j++ ) {
        ledMatrix.setColumn(j, pgm_read_byte(&(digits[VERSION_MAJOR].array[i])) );
    }
    ledMatrix.setColumn(j++, 0x00);
    ledMatrix.setColumn(j++, 0x80);
    ledMatrix.setColumn(j++, 0x00);
    
    for( byte i = 0; i < pgm_read_byte(&(digits[VERSION_MINOR].size)); i++, j++ ) {
        ledMatrix.setColumn(j, pgm_read_byte(&(digits[VERSION_MINOR].array[i])) );
    }

    while( j < 32 ) {
        ledMatrix.setColumn(j++, 0x00);
    }
    display_FixRotation(display_orientation);
    ledMatrix.commit();
}
