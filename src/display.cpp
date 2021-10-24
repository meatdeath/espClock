#include "display.h"

LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CS_PIN);

void display_init(void) {
    ledMatrix.init();
    ledMatrix.setText("23:47");
    ledMatrix.setTextOffset(32);
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

    for( i = 0; i < pgm_read_byte(&(digits[h1].size)); i++ ) {
        ledMatrix.setColumn(i, pgm_read_byte(&(digits[h1].array[i])) );
    }
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