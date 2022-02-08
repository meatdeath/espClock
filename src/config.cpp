#include "config.h"
//#include <EEPROM.h>
#include <extEEPROM.h>

config_t config;
extern extEEPROM eeprom;

void config_init(void) {
    //EEPROM.begin(512);
    memset(&config, 0, sizeof(config));
}

int8_t config_gettimeoffset(void) {
    config_read();
    return config.clock.hour_offset;
}

void config_writeeeprom(void) {
    for (uint16_t i = 0; i < sizeof(config); i++) {
        eeprom.write( i, ((byte*)&config)[i] );
        // EEPROM.write( i, ((byte*)&config)[i] );
    }
    
    //EEPROM.commit();
}

void config_settimeoffset(int8_t hours_offset, int8_t minutes_offset) {
    config.clock.hour_offset = hours_offset;
    config.clock.minute_offset = minutes_offset;
    config_writeeeprom();
}

void config_read(void) {
    Serial.println("   read config bytes from EEPROM");
    for (uint16_t i = 0; i < sizeof(config); i++) {
        uint8_t *addr = &(((byte*)&config)[i]);
        Serial.printf("   read [%d]: addr=%p", i, addr);
        uint8_t data = char(eeprom.read(i));
        Serial.printf(" data=0x%02x\r\n", data);
        *addr = data;
        // ((byte*)&config)[i] = char(EEPROM.read(i));
    }

    Serial.println("   check config for existing wifi configuration");
    if(config.wifi.name[0] < 'A' || config.wifi.name[0] > 'z' || strlen(config.wifi.name) == 0 )
    {
        Serial.println("EEPROM doesn't contain WiFi connection information.");
        config.wifi.valid = false;
        memset(config.wifi.name, 0, sizeof(config.wifi.name));
        memset(config.wifi.password, 0, sizeof(config.wifi.password));
    } else {
        config.wifi.valid = true;
    }

    Serial.println("   validate time offset in configuration");
    if( config.clock.hour_offset < -12 || config.clock.hour_offset > 12 ) {
        config.clock.hour_offset = 0;
    }

    if( config.clock.minute_offset < -30 || config.clock.minute_offset > 30 ) {
        config.clock.minute_offset = 0;
    }
}

void config_set(String *wifi_name, String *wifi_password, String *auth_username, String *auth_password)
{
    memset( config.wifi.name, 0, sizeof(config.wifi.name) );
    memset( config.wifi.password, 0, sizeof(config.wifi.password) );
    memset( config.auth.username, 0, sizeof(config.auth.username) );
    memset( config.auth.password, 0, sizeof(config.auth.password) );

    strcpy( config.wifi.name, wifi_name->c_str() );
    strcpy( config.wifi.password, wifi_password->c_str() );
    strcpy( config.auth.username, auth_username->c_str() );
    strcpy( config.auth.password, auth_password->c_str() );

    Serial.println("Set config:");
    Serial.println(*wifi_name);
    Serial.println(*wifi_password);
    Serial.println(*auth_username);
    Serial.println(*auth_password);
    Serial.println("");
    
    config_writeeeprom();
}

void config_clearwifi(void) {
    Serial.print("Clearing wifi settings... ");
    
    memset( config.wifi.name, 0xff, sizeof(config.wifi.name) );
    memset( config.wifi.password, 0xff, sizeof(config.wifi.password) );
    
    config_writeeeprom();
    Serial.println("Done");
}