#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <Arduino.h>

#define CREDENTIALS_SIZE    32

#pragma pack(push,1)
typedef struct _config_wifi_st {
    char name[CREDENTIALS_SIZE];
    char password[CREDENTIALS_SIZE];
    uint8_t valid_marker;
} config_wifi_t;
typedef struct _config_auth_st {
    char username[CREDENTIALS_SIZE];
    char password[CREDENTIALS_SIZE];
    uint8_t valid_marker;
} config_auth_t;
typedef struct _config_clock_st {
    int8_t hour_offset;
    uint8_t minute_offset;
    uint8_t valid_marker;
} config_clock_t;
#pragma pack(pop)

#define VALID_CONFIG_MARKER         0x5A

#define EEPROM_CONFIG_WIFI_ADDR     0
#define EEPROM_CONFIG_WIFI_SIZE     sizeof(config_wifi_t)

#define EEPROM_CONFIG_AUTH_ADDR     (EEPROM_CONFIG_WIFI_ADDR+EEPROM_CONFIG_WIFI_SIZE)
#define EEPROM_CONFIG_AUTH_SIZE     sizeof(config_auth_t)

#define EEPROM_CONFIG_CLOCK_ADDR    (EEPROM_CONFIG_AUTH_ADDR+EEPROM_CONFIG_AUTH_SIZE)
#define EEPROM_CONFIG_CLOCK_SIZE    sizeof(config_clock_t)

extern config_wifi_t config_wifi;
extern config_auth_t config_auth;
extern config_clock_t config_clock;

void config_init(void);
bool config_SetTimeSettings(int8_t h_offset, int8_t m_offset);
void config_setWiFiSettings(String ssid, String pass);
void config_setAuthSettings(String user, String pass);

#endif // __CONFIG_H__