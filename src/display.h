#ifndef __DISPLAY
#define __DISPLAY

#include <Arduino.h>
#include <SPI.h>
#include "LedMatrix.h"

#define NUMBER_OF_DEVICES 4

#define CS_PIN 2

typedef struct _digits_st {
    byte array[8];
    byte size;
} digits_t;

const digits_t digits [] PROGMEM = {
    { { 0b01111110, 0b11111111, 0b10000001, 0b10000001, 0b11111111, 0b01111110 }, 6 },   //0
    { { 0b00000100, 0b10000010, 0b11111111, 0b11111111, 0b10000000, 0b00000000 }, 6 },   //1
    { { 0b11100010, 0b11110011, 0b10010001, 0b10010001, 0b10011111, 0b10001110 }, 6 },   //2
    { { 0b01000010, 0b11000011, 0b10001001, 0b10001001, 0b11111111, 0b01110110 }, 6 },   //3
    { { 0b00111000, 0b00100100, 0b10100010, 0b11111111, 0b11111111, 0b10100000 }, 6 },   //4
    { { 0b01000111, 0b11000111, 0b10000101, 0b10000101, 0b11111101, 0b01111001 }, 6 },   //5
    { { 0b01111110, 0b11111111, 0b10001001, 0b10001001, 0b11111011, 0b01110010 }, 6 },   //6
    { { 0b00000011, 0b00000011, 0b11100001, 0b11111001, 0b00011111, 0b00000111 }, 6 },   //7
    { { 0b01110110, 0b11111111, 0b10001001, 0b10001001, 0b11111111, 0b01110110 }, 6 },   //8
    { { 0b01001110, 0b11011111, 0b10010001, 0b10010001, 0b11111111, 0b01111110 }, 6 },   //9
    { { 0b01100110, 0b01100110, }, 2 },   //:
    { { 0b00000011, 0b00000011, }, 2 },   //.
};

enum __display_symbols {
    DISPLAY_SYMBOL_COLON = 10,
    DISPLAY_SYMBOL_DOT
};

#define DISPLAY_FORMAT_24H  0
#define DISPLAY_FORMAT_12H  1

void display_init( void );
void display_printtime( byte hours, byte minutes, byte seconds, byte format );

#endif // __DISPLAY
