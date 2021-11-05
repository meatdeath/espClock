#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <Arduino.h>

typedef struct _config_st {
    struct _wifi_config_st {
        bool valid;
        char name[32];
        char password[32];
    } wifi;
    struct _clock_config_ {
        int8_t hour_offset;
        int8_t minute_offset;
    } clock;
} config_t;

extern config_t config;

void config_init(void);
int8_t config_gettimeoffset(void);
void config_settimeoffset(int8_t hours_offset, int8_t minutes_offset);
void config_read(void);
void config_setwifi(String *name, String *password);
void config_clearwifi(void);

#endif // __CONFIG_H__