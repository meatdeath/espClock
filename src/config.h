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
    struct _auth_st {
        char username[16];
        char password[16];
    } auth;
} config_t;

#define EEPROM_CONFIG_START_ADDR    0
#define EEPROM_CONFIG_SIZE          sizeof(config_t)

extern config_t config;

void config_init(void);
int8_t config_gettimeoffset(void);
void config_settimeoffset(int8_t hours_offset, int8_t minutes_offset);
void config_read(void);
void config_set(String *name, String *password, String *auth_username, String *auth_password);
void config_clearwifi(void);

#endif // __CONFIG_H__