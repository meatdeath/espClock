#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <Arduino.h>
#include <SPI.h>
#include "LedMatrix.h"

#define NUMBER_OF_DEVICES 4

#define CS_PIN 15
//#define HIDE_HOUR_LEADING_ZERO

#pragma pack(push,1)
typedef struct _digits_st {
    byte array[32];
    byte size;
} digits_t;
#pragma pack(pop)

// const digits_t digits [] PROGMEM = {
//     { { 0b01111110, 0b11111111, 0b10000001, 0b10000001, 0b11111111, 0b01111110 }, 6 },   //0
//     { { 0b00000100, 0b10000010, 0b11111111, 0b11111111, 0b10000000, 0b00000000 }, 6 },   //1
//     { { 0b11100010, 0b11110011, 0b10010001, 0b10010001, 0b10011111, 0b10001110 }, 6 },   //2
//     { { 0b01000010, 0b11000011, 0b10001001, 0b10001001, 0b11111111, 0b01110110 }, 6 },   //3
//     { { 0b00111000, 0b00100100, 0b10100010, 0b11111111, 0b11111111, 0b10100000 }, 6 },   //4
//     { { 0b01000111, 0b11000111, 0b10000101, 0b10000101, 0b11111101, 0b01111001 }, 6 },   //5
//     { { 0b01111110, 0b11111111, 0b10001001, 0b10001001, 0b11111011, 0b01110010 }, 6 },   //6
//     { { 0b00000011, 0b00000011, 0b11100001, 0b11111001, 0b00011111, 0b00000111 }, 6 },   //7
//     { { 0b01110110, 0b11111111, 0b10001001, 0b10001001, 0b11111111, 0b01110110 }, 6 },   //8
//     { { 0b01001110, 0b11011111, 0b10010001, 0b10010001, 0b11111111, 0b01111110 }, 6 },   //9
//     { { 0b01100110, 0b01100110, }, 2 },   //:
//     { { 0b00000011, 0b00000011, }, 2 },   //.
//     { { 
//         0x22, 0x49, 0x49, 0x36, 0x00, 0x78, 0x24, 0x24, 0x78, 0x00, 0x7c, 0x04, 0x04, 0x7c, 0x00, 0x0c,
//         0x50, 0x50, 0x3c, 0x00, 0x38, 0x44, 0x44, 0x28, 0x00, 0x7c, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00
//       }, 32 
//     },    // Zapusk
// };

#define VERSION_MAJOR   1
#define VERSION_MINOR   5

const digits_t digits [] PROGMEM = {
    { { 0b01111110, 0b10000001, 0b10000001, 0b10000001, 0b01111110 }, 5 },   //0
    { { 0b00000000, 0b00000100, 0b00000010, 0b11111111, 0b00000000 }, 5 },   //1
    { { 0b10000010, 0b11000001, 0b10100001, 0b10010001, 0b10001110 }, 5 },   //2
    { { 0b01000010, 0b10000001, 0b10001001, 0b10001001, 0b01110110 }, 5 },   //3
    { { 0b00110000, 0b00101000, 0b00100100, 0b00100010, 0b11111111 }, 5 },   //4
    { { 0b01001111, 0b10001001, 0b10001001, 0b10001001, 0b01110001 }, 5 },   //5
    { { 0b01111110, 0b10001001, 0b10001001, 0b10001001, 0b01110010 }, 5 },   //6
    { { 0b00000001, 0b11100001, 0b00010001, 0b00001001, 0b00000111 }, 5 },   //7
    { { 0b01110110, 0b10001001, 0b10001001, 0b10001001, 0b01110110 }, 5 },   //8
    { { 0b01001110, 0b10010001, 0b10010001, 0b10010001, 0b01111110 }, 5 },   //9
    { { 0b00100100, 0b00000000, }, 2 },   //:
    { { 0b00000011, 0b00000011, }, 2 },   //.
    { {     // Zapusk...
        0x22, 0x49, 0x49, 0x36, 0x00, 0x78, 0x24, 0x24, 0x78, 0x00, 0x7c, 0x04, 0x04, 0x7c, 0x00, 0x0c,
        0x50, 0x50, 0x3c, 0x00, 0x38, 0x44, 0x44, 0x28, 0x00, 0x7c, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00
      }, 32 
    },
    {     // Starting...
        {
            0x7E, 0x81, 0x81, 0x81, 0x62, 0x00, 0x04, 0xfC, 0x04, 0x00, 0xF8, 0x24, 0x24, 0xFC, 0x00, 0xFC, 
            0x24, 0x24, 0x18, 0x00, 0x04, 0xfC, 0x04, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80 
        },32
    },
    {       // Clock
        {
            0x00,0x00,0x00,0x00,0x00,0x0f,0x10,0x10,0x10,0xff,0x00,0xf8,0x24,0x24,0xfc,0x00,
            0x78,0x84,0x84,0x48,0x00,0xfc,0x90,0x90,0x60,0x00,0xfc,0x00,0x00,0x00,0x00,0x00
        }, 32

    },
    {   // -
        {
            0x10, 0x10, 0x10
        }, 3
    },
    {   // Celcius
        {
            0x06, 0x09, 0x09, 0x06, 0x00, 0x7E, 0x81, 0x81, 0x81, 0x42
        }, 10
    },
    {   // mm
        {
            0xF8, 0x30, 0x40, 0x30, 0xF8, 0x00, 0xF8, 0x30, 0x40, 0x30, 0xF8
        }, 11
    }
};

enum __display_symbols {
    DISPLAY_SYMBOL_COLON = 10,
    DISPLAY_SYMBOL_DOT,
    DISPLAY_SYMBOL_ZAPUSK,
    DISPLAY_STARTING,
    DISPLAY_CLOCK_STR,
    DISPLAY_MINUS,
    DISPLAY_CELCIUS,
    DISPLAY_MM
};

#define DISPLAY_FORMAT_24H  0
#define DISPLAY_FORMAT_12H  1

#define CLOCK_SHOW_TIME         6
#define TEMPERATURE_SHOW_TIME   3
#define PRESSURE_SHOW_TIME      3

void display_Init( void );
void display_PrintStarting(void);
void display_Time( byte hours, byte minutes, byte seconds, byte format );
void display_SetIntensity(byte intensity);
void display_SetBrightness(uint8_t percentage);

void display_Temperature(int temperature);
void display_Pressure(uint16_t pressure);

void display_ClockString(void);
void display_Version(void);

#endif // __DISPLAY_H__
