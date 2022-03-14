#include "config.h"
#include <extEEPROM.h>

config_wifi_t config_wifi;
config_auth_t config_auth;
config_clock_t config_clock;

extern extEEPROM eeprom;

void eeprom_ReadBlock(unsigned long addr, uint8_t *buf, uint16_t size){
    for( uint16_t i = 0; i < size; i++ ) {
        buf[i] = eeprom.read(addr+i);
    }
}
void eeprom_WriteBlock(unsigned long addr, uint8_t *buf, uint16_t size){
    for( uint16_t i = 0; i < size; i++ ) {
        eeprom.write(addr+i,buf[i]);
    }
}

void config_validate(void);

void config_printDumpFromEeprom() {
    uint16_t size = sizeof(config_wifi_t) + sizeof(config_auth_t) + sizeof(config_clock_t);
    for( uint16_t i = 0; i < size; i++ ) {
        if( (i%16) == 0 ) {
            Serial.printf("\r\n%04x: ", i);
        }
        Serial.printf("%02x ", eeprom.read(i));
    }
    Serial.println();
}

void config_init(void) {
    memset(&config_wifi, 0xFF, sizeof(config_wifi_t));
    memset(&config_auth, 0xFF, sizeof(config_auth_t));
    memset(&config_clock, 0xFF, sizeof(config_clock_t));
    eeprom_ReadBlock(EEPROM_CONFIG_WIFI_ADDR, (uint8_t*)&config_wifi, sizeof(config_wifi_t));
    eeprom_ReadBlock(EEPROM_CONFIG_AUTH_ADDR, (uint8_t*)&config_auth, sizeof(config_auth_t));
    eeprom_ReadBlock(EEPROM_CONFIG_CLOCK_ADDR, (uint8_t*)&config_clock, sizeof(config_clock_t));
    config_validate();
}

void config_store(void) {
    Serial.println("Store config -> EEPROM full update.");
    eeprom_WriteBlock(EEPROM_CONFIG_WIFI_ADDR, (uint8_t*)&config_wifi, sizeof(config_wifi_t));
    eeprom_WriteBlock(EEPROM_CONFIG_AUTH_ADDR, (uint8_t*)&config_auth, sizeof(config_auth_t));
    eeprom_WriteBlock(EEPROM_CONFIG_CLOCK_ADDR, (uint8_t*)&config_clock, sizeof(config_clock_t));
}

bool config_SetTimeSettings(int8_t hour_offset, int8_t minute_offset) {
    if( hour_offset < (-12) || hour_offset > 12 || 
        minute_offset < 0 || minute_offset > 59 ) {
            return false;
    }
    config_clock.hour_offset = hour_offset;
    config_clock.minute_offset = minute_offset;

    eeprom_WriteBlock(EEPROM_CONFIG_CLOCK_ADDR, (uint8_t*)&config_clock, sizeof(config_clock_t));

    return true;
}

void config_validate(void) {
    Serial.println("Config validation");
    Serial.printf("config_wifi.name: %s\r\n",config_wifi.name);
    Serial.printf("config_wifi.password: %s\r\n",config_wifi.password);
    if( config_wifi.valid_marker != VALID_CONFIG_MARKER ) 
    {
        if( config_wifi.name[0] < 'A' || 
            config_wifi.name[0] > 'z' || 
            strlen(config_wifi.name) > CREDENTIALS_SIZE || 
            strlen(config_wifi.password) > CREDENTIALS_SIZE )
        {
            Serial.println("WiFi config is not valid");
        } else {
            config_wifi.valid_marker = VALID_CONFIG_MARKER;
            eeprom_WriteBlock(EEPROM_CONFIG_WIFI_ADDR, (uint8_t*)&config_wifi, sizeof(config_wifi_t));
        }
    }

    bool changed = false;

    if( config_clock.valid_marker != VALID_CONFIG_MARKER ) 
    {
        changed = true;
        config_clock.hour_offset = 0;
        config_clock.minute_offset = 0;
        config_clock.valid_marker = VALID_CONFIG_MARKER;
    } 
    else 
    {
        if( config_clock.hour_offset < -12 || config_clock.hour_offset > 12 ) {
            changed = true;
            config_clock.hour_offset = 0;
            Serial.println("Hour offset fixed");
        }
        if( config_clock.minute_offset > 59 ) {
            changed = true;
            config_clock.minute_offset = 0;
            Serial.println("Minute offset fixed");
        }
    }
    if( changed == true) {
        eeprom_WriteBlock(EEPROM_CONFIG_CLOCK_ADDR, (uint8_t*)&config_clock, sizeof(config_clock_t));
    }

    if( config_auth.valid_marker != VALID_CONFIG_MARKER ) 
    {
        if( config_auth.username[0] < 'A' || 
            config_auth.username[0] > 'z' || 
            strlen(config_auth.username) > CREDENTIALS_SIZE || 
            strlen(config_auth.password) > CREDENTIALS_SIZE )
        {
            Serial.println("Auth config is not valid");
        } else {
            config_auth.valid_marker = VALID_CONFIG_MARKER;
            eeprom_WriteBlock(EEPROM_CONFIG_AUTH_ADDR, (uint8_t*)&config_auth, sizeof(config_auth_t));
        }
    } 
}


void config_setWiFiSettings(String ssid, String pass){
    if(ssid == ""){
        memset( (uint8_t*)&config_wifi, 0xFF, sizeof(config_wifi_t) );
        eeprom_WriteBlock(EEPROM_CONFIG_WIFI_ADDR, (uint8_t*)&config_wifi, sizeof(config_wifi_t));
    } else {
        memset( (uint8_t*)&config_wifi, 0xFF, sizeof(config_wifi_t) );
        strcpy( config_wifi.name, ssid.c_str() );
        strcpy( config_wifi.password, pass.c_str() );
        config_validate();
        Serial.printf("Set WiFi config:\r\nSSID:%s Pass:%s\r\n", 
            config_wifi.name, 
            config_wifi.password);
    }
}
void config_setAuthSettings(String user, String pass){
    if(user == ""){
        memset( (uint8_t*)&config_auth, 0xFF, sizeof(config_auth_t) );
        eeprom_WriteBlock(EEPROM_CONFIG_AUTH_ADDR, (uint8_t*)&config_auth, sizeof(config_auth_t));
    } else {
        memset( (uint8_t*)&config_auth, 0xFF, sizeof(config_auth_t) );
        strcpy( config_auth.username, user.c_str() );
        strcpy( config_auth.password, pass.c_str() );
        config_validate();
        Serial.printf("Set auth config:\r\nUser:%s Pass:%s\r\n",
            config_auth.username,
            config_auth.password);
    }
}

// void config_resetNetSettings() {
//     memset( (uint8_t*)&config_wifi, 0xFF, sizeof(config_wifi_t) );
//     memset( (uint8_t*)&config_auth, 0xFF, sizeof(config_auth_t) );

//     eeprom_WriteBlock(EEPROM_CONFIG_WIFI_ADDR, (uint8_t*)&config_wifi, sizeof(config_wifi_t));
//     eeprom_WriteBlock(EEPROM_CONFIG_AUTH_ADDR, (uint8_t*)&config_auth, sizeof(config_auth_t));
// }

// void config_setNetSettings(String *ssid, String *ssid_pass, String *auth_username, String *auth_pass)
// {
//     memset( (uint8_t*)&config_wifi, 0xFF, sizeof(config_wifi_t) );
//     strcpy( config_wifi.name, ssid->c_str() );
//     strcpy( config_wifi.password, ssid_pass->c_str() );

//     memset( (uint8_t*)&config_auth, 0xFF, sizeof(config_auth_t) );
//     strcpy( config_auth.username, auth_username->c_str() );
//     strcpy( config_auth.password, auth_pass->c_str() );

//     config_validate();

//     Serial.println("Set wifi config:");
//     Serial.println(config_wifi.name);
//     Serial.println(config_wifi.password);
//     Serial.println("");
//     Serial.println("Set auth config:");
//     Serial.println(config_auth.username);
//     Serial.println(config_auth.password);
//     Serial.println("");
// }

// void config_clearwifi(void) {
//     Serial.print("Clearing wifi settings... ");
//     memset( config_wifi.name, 0xff, sizeof(config_wifi.name) );
//     memset( config_wifi.password, 0xff, sizeof(config_wifi.password) );
//     config_wifi.valid_marker = 0xFF;
//     eeprom_WriteBlock(EEPROM_CONFIG_WIFI_ADDR, (uint8_t*)&config_wifi, sizeof(config_wifi_t));
//     Serial.println("Done");
// }