#include "config.h"
#include <extEEPROM.h>

config_wifi_t config_wifi;
config_auth_t config_auth;
config_clock_t config_clock;

extern extEEPROM eeprom;

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
    eeprom.read(EEPROM_CONFIG_WIFI_ADDR, (uint8_t*)&config_wifi, sizeof(config_wifi_t));
    eeprom.read(EEPROM_CONFIG_AUTH_ADDR, (uint8_t*)&config_auth, sizeof(config_auth_t));
    eeprom.read(EEPROM_CONFIG_CLOCK_ADDR, (uint8_t*)&config_clock, sizeof(config_clock_t));
    config_validate();
    //config_printDumpFromEeprom();
}

int16_t config_gettimeoffset(int8_t *h_offset, int8_t *m_offset) {
    if(h_offset)    *h_offset = config_clock.hour_offset;
    if(m_offset)    *m_offset = config_clock.minute_offset;
    return config_clock.hour_offset*60 + config_clock.minute_offset;
}

void config_store(void) {
    Serial.println("Store config -> EEPROM full update.");
    uint8_t* ptr;

    ptr = (uint8_t*)&config_wifi;
    for( uint8_t i = 0; i < sizeof(config_wifi_t); i++ ) 
        eeprom.write(EEPROM_CONFIG_WIFI_ADDR+i, ptr[i]);

    ptr = (uint8_t*)&config_auth;
    for( uint8_t i = 0; i < sizeof(config_auth_t); i++ ) 
        eeprom.write(EEPROM_CONFIG_AUTH_ADDR+i, ptr[i]);

    ptr = (uint8_t*)&config_clock;
    for( uint8_t i = 0; i < sizeof(config_clock_t); i++ ) 
        eeprom.write(EEPROM_CONFIG_CLOCK_ADDR+i, ptr[i]);
}

void config_settimeoffset(int8_t hours_offset, int8_t minutes_offset) {
    config_clock.hour_offset = hours_offset;
    config_clock.minute_offset = minutes_offset;

    Serial.printf("Set offset: addr=0x%04x data: %02x %02x\r\n", EEPROM_CONFIG_CLOCK_ADDR, (int8_t)config_clock.hour_offset, (int8_t)config_clock.minute_offset);
    
    uint8_t *ptr = (uint8_t*)&config_clock;
    for( uint8_t i = 0; i < sizeof(config_clock_t); i++ ) 
        eeprom.write(EEPROM_CONFIG_CLOCK_ADDR+i, ptr[i]);

    config_printDumpFromEeprom();
}

void config_validate(void) {
    Serial.println("Config validation");
    config_printDumpFromEeprom();
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
            uint8_t *ptr = (uint8_t*)&config_wifi;
            for( uint8_t i = 0; i < sizeof(config_wifi_t); i++ ) 
                eeprom.write(EEPROM_CONFIG_WIFI_ADDR+i, ptr[i]);
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
        if( config_clock.minute_offset < 0 || config_clock.minute_offset > 59 ) {
            changed = true;
            config_clock.minute_offset = 0;
            Serial.println("Minute offset fixed");
        }
    }
    if( changed == true) {
        uint8_t *ptr = (uint8_t*)&config_clock;
        for( uint8_t i = 0; i < sizeof(config_clock_t); i++ ) 
            eeprom.write(EEPROM_CONFIG_CLOCK_ADDR+i, ptr[i]);
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
            uint8_t *ptr = (uint8_t*)&config_auth;
            for( uint8_t i = 0; i < sizeof(config_auth_t); i++ ) 
                eeprom.write(EEPROM_CONFIG_AUTH_ADDR+i, ptr[i]);
        }
    } 
}

void config_resetNetSettings() {
    memset( (uint8_t*)&config_wifi, 0xFF, sizeof(config_wifi_t) );
    memset( (uint8_t*)&config_auth, 0xFF, sizeof(config_auth_t) );

    uint8_t *ptr = (uint8_t*)&config_wifi;
    for( uint8_t i = 0; i < sizeof(config_wifi_t); i++ ) 
        eeprom.write(EEPROM_CONFIG_WIFI_ADDR+i, ptr[i]);
    ptr = (uint8_t*)&config_auth;
    for( uint8_t i = 0; i < sizeof(config_auth_t); i++ )
        eeprom.write(EEPROM_CONFIG_AUTH_ADDR+i, ptr[i]);
}

void config_setNetSettings(String *ssid, String *ssid_pass, String *auth_username, String *auth_pass)
{
    memset( (uint8_t*)&config_wifi, 0xFF, sizeof(config_wifi_t) );
    strcpy( config_wifi.name, ssid->c_str() );
    strcpy( config_wifi.password, ssid_pass->c_str() );

    memset( (uint8_t*)&config_auth, 0xFF, sizeof(config_auth_t) );
    strcpy( config_auth.username, auth_username->c_str() );
    strcpy( config_auth.password, auth_pass->c_str() );

    config_validate();

    Serial.println("Set wifi config:");
    Serial.println(config_wifi.name);
    Serial.println(config_wifi.password);
    Serial.println("");

    Serial.println("Set auth config:");
    Serial.println(config_auth.username);
    Serial.println(config_auth.password);
    Serial.println("");

    // uint8_t *ptr = (uint8_t*)&config_wifi;
    // for( uint8_t i = 0; i < sizeof(config_wifi_t); i++ ) 
    //     eeprom.write(EEPROM_CONFIG_WIFI_ADDR+i, ptr[i]);
    // ptr = (uint8_t*)&config_auth;
    // for( uint8_t i = 0; i < sizeof(config_auth_t); i++ )
    //     eeprom.write(EEPROM_CONFIG_AUTH_ADDR+i, ptr[i]);
}

void config_clearwifi(void) {
    Serial.print("Clearing wifi settings... ");
    
    memset( config_wifi.name, 0xff, sizeof(config_wifi.name) );
    memset( config_wifi.password, 0xff, sizeof(config_wifi.password) );

    config_wifi.valid_marker = 0xFF;
    
    uint8_t *ptr = (uint8_t*)&config_wifi;
    for( uint8_t i = 0; i < sizeof(config_wifi_t); i++ ) 
        eeprom.write(EEPROM_CONFIG_WIFI_ADDR+i, ptr[i]);
    Serial.println("Done");
}