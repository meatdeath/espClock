#include "config.h"
#include <EEPROM.h>

config_t config;

void config_init(void) {
    EEPROM.begin(512);
    memset(&config, 0, sizeof(config));
}

int8_t config_gettimeoffset(void) {
    config_read();
    return config.clock.hour_offset;
}

void config_writeeeprom(void) {
    for (uint16_t i = 0; i < sizeof(config); i++) {
         EEPROM.write( i, ((byte*)&config)[i] );
    }
    EEPROM.commit();
}

void config_settimeoffset(int8_t hours_offset, int8_t minutes_offset) {
    config.clock.hour_offset = hours_offset;
    config.clock.minute_offset = minutes_offset;
    //long int address = (long int)&(config.clock.hour_offset) - (long int)&config;
    //EEPROM.write(address, config.clock.hour_offset);
    config_writeeeprom();
}

void config_read(void) {
    for (uint16_t i = 0; i < sizeof(config); i++) {
        ((byte*)&config)[i] = char(EEPROM.read(i));
    }

    if(config.wifi.name[0] == 0 || config.wifi.name[0] == 0xff || strlen(config.wifi.name) == 0 )
    {
        Serial.println("EEPROM doesn't contain WiFi connection information.");
        config.wifi.valid = false;
        memset(config.wifi.name, 0, sizeof(config.wifi.name));
        memset(config.wifi.password, 0, sizeof(config.wifi.password));
    } else {
        config.wifi.valid = true;
    }

    if( config.clock.hour_offset < -12 || config.clock.hour_offset > 12 ) {
        config.clock.hour_offset = 0;
    }

    if( config.clock.minute_offset < -30 || config.clock.minute_offset > 30 ) {
        config.clock.minute_offset = 0;
    }

    // Serial.println("Reading EEPROM ssid");
    // String esid;
    // for (int i = 0; i < 32; ++i)
    // {
    //     esid += char(EEPROM.read(i));
    // }
    // if(esid[0] == 0 || esid[0] == 0xff || esid.length() == 0 )
    // {
    //     Serial.println("EEPROM doesn't contain WiFi connection information.");
    //     config.wifi.valid = false;
    // } else {
    //     config.wifi.valid = true;
    // }
    // Serial.print("SSID: ");
    // Serial.println(esid);
    // Serial.println("Reading EEPROM pass");
    // String epass = "";
    // for (int i = 32; i < 96; ++i)
    // {
    //     epass += char(EEPROM.read(i));
    // }
    // Serial.print("PASS: ");
    // Serial.println(epass);
    // strcpy( config.wifi.name, esid.c_str() );
    // strcpy( config.wifi.password, epass.c_str() );
}

void config_setwifi(String *name, String *password)
{
    // Serial.println("clearing eeprom");
    // for (int i = 0; i < 96; ++i) 
    // { 
    //     EEPROM.write(i, 0);
    // }

    memset( config.wifi.name, 0, sizeof(config.wifi.name) );
    memset( config.wifi.password, 0, sizeof(config.wifi.password) );

    strcpy( config.wifi.name, name->c_str() );
    strcpy( config.wifi.password, password->c_str() );

    Serial.println("Set wifi config:");
    Serial.println(*name);
    Serial.println(*password);
    Serial.println("");
        
    // Serial.print("writing eeprom ssid: ");
    // for (unsigned int i = 0; i < sizeof(config.wifi.name); ++i)
    // {
    //     EEPROM.write(i, config.wifi.name[i]); 
    //     Serial.print(config.wifi.name[i]); 
    // }
    // Serial.println();
    // Serial.print("writing eeprom pass: "); 
    // for (unsigned int i = 0; i < sizeof(config.wifi.password); ++i)
    // {
    //     EEPROM.write(32+i, config.wifi.password[i]);
    //     Serial.print(config.wifi.password[i]); 
    // }    
    
    // Serial.println();
    config_writeeeprom();
}

void config_clearwifi(void) {
    Serial.print("Clearing wifi settings... ");
    
    memset( config.wifi.name, 0, sizeof(config.wifi.name) );
    memset( config.wifi.password, 0, sizeof(config.wifi.password) );
    // for (unsigned int i = 0; i < sizeof(config.wifi.name); ++i)
    // {
    //     EEPROM.write(i, config.wifi.name[i]); 
    // }
    // for (unsigned int i = 0; i < sizeof(config.wifi.password); ++i)
    // {
    //     EEPROM.write(32+i, config.wifi.password[i]);
    // }
    config_writeeeprom();
    Serial.println("Done");
}