#include <Arduino.h>
#include <extEEPROM.h>
#include "pressure_history.h"
#include "config.h"
#include "rtc.h"

//-----------------------------------------------------------------------------

uint16_t pressure_history_size = 0;
uint16_t pressure_history_start = 0;
uint16_t pressure_history_end = 0;

String pressureLabelsStr;
String pressureValuesStr;
String pressure_json_history;
String pressure_html_history;

extern extEEPROM eeprom;
extern float pressure;

pressure_history_t pressure_history_item[PRESSURE_HISTORY_SIZE] = {0};

//-----------------------------------------------------------------------------

void pressureHistory_printDumpFromEeprom() {
    for( uint16_t i = 0; i < (PRESSURE_HISTORY_SIZE * sizeof(pressure_history_t)); i++ ) {
        if( (i%sizeof(pressure_history_t)) == 0 ) {
            Serial.printf("\r\n%04x: ", EEPROM_HISTORY_ADDR+i);
        }
        Serial.printf("%02x ", eeprom.read(EEPROM_HISTORY_ADDR+i));
    }
    Serial.println();
}

void generate_pressure_history(void) {
    pressureLabelsStr = "";
    pressureValuesStr = "";
    pressure_html_history = "";

    //Serial.println("----- Pressure history -----");
    pressure_json_history = "{\"current\":";
    pressure_json_history += pressure;
    pressure_json_history += ",\"history\":[";
    uint16_t time = pressure_history_size-1;
    uint16_t i = pressure_history_start;
    for(uint16_t j = 0; j < pressure_history_size; j++ )
    {
        // Serial.printf("   [%d] time:%lu   pressure:%3.1f\r\n", i, pressure_history_item[i].time, pressure_history_item[i].pressure);
        if( i != pressure_history_start ) 
            pressure_json_history += ",";
        pressure_json_history += "{\"time\":";
        pressure_json_history += pressure_history_item[i].time;
        pressure_json_history += ",\"value\":";
        if( pressure_history_item[i].pressure == 800 ) {
            pressure_json_history += "null";
        } else {
            pressure_json_history += pressure_history_item[i].pressure;
        }
        pressure_json_history += "}";

        uint32_t time_sec = pressure_history_item[i].time + config_clock.hour_offset*3600 + config_clock.minute_offset*60;
        uint8_t hour = (time_sec/3600 ) % 24;
        uint8_t minute = (time_sec/60) % 60;
        pressure_html_history += (String)"<tr><td>" + hour + (String)"h " + minute + (String)"min</td>";
        pressure_html_history += (String)"<td>" + pressure_history_item[i].pressure + (String)"mmm</td></tr>";
        pressureLabelsStr += "\"";
        if((time&1) == 0) {
            pressureLabelsStr += time*2;
            pressureLabelsStr += "h";
        }
        pressureLabelsStr += "\"";

        if( pressure_history_item[i].pressure != 800 )
        {
            pressureValuesStr += pressure_history_item[i].pressure;
        }
        else
        {
            pressureValuesStr += "null";
        }

        i++;
        if( i == PRESSURE_HISTORY_SIZE )
            i = 0; 
        if( time ) {
            pressureLabelsStr += ",";
            pressureValuesStr += ",";
        }
        time--;
    }

    pressure_json_history += "]}";
    //Serial.println("----------------------------");
}


void eeprom_add_history_item( unsigned long time, float pressure ) {
    unsigned long addr = EEPROM_HISTORY_ADDR + pressure_history_end*EEPROM_HISTORY_ITEM_SIZE;
    Serial.printf("Adding history item: time=%lu, pressure=%3.1f\r\n", time, (double)pressure );
    pressure_history_item[pressure_history_end].time = time;
    pressure_history_item[pressure_history_end].pressure = pressure;

    uint8_t *ptr = (uint8_t*)&(pressure_history_item[pressure_history_end]);
    for( uint16_t i = 0; i < EEPROM_HISTORY_ITEM_SIZE; i++ ) 
        eeprom.write(addr+i, ptr[i]);

    if( pressure_history_size == PRESSURE_HISTORY_SIZE ) {
        pressure_history_start++;
    } else {
        pressure_history_size++;
    }
    pressure_history_end++;
    if( pressure_history_end == PRESSURE_HISTORY_SIZE ) {
        pressure_history_end = 0;
    }
    if( pressure_history_start == PRESSURE_HISTORY_SIZE ) {
        pressure_history_start = 0;
    }
}

void eeprom_restore_pressure_history(unsigned long time) {
    int i, j;
    pressure_history_start = 0;
    pressure_history_end = 0;
    pressure_history_size = 0;

    Serial.printf("Restore history time: 0x%08lx / %lu\r\n", time, time);

    Serial.println("Read history from EEPROM");
    uint8_t eepStatus;
    for( int i = 0; i < PRESSURE_HISTORY_SIZE; i++ ) {
        eepStatus = eeprom.read( EEPROM_HISTORY_ADDR + i*EEPROM_HISTORY_ITEM_SIZE, 
                                (uint8_t*)(&pressure_history_item[i]), 
                                EEPROM_HISTORY_ITEM_SIZE );
    }
    Serial.printf("EEPROM status: %d\r\n", eepStatus);
pressureHistory_printDumpFromEeprom();
    // Remove invalid and outdated items
    pressure_history_t empty_item;
    memset((uint8_t*)(&empty_item), 0xFF, sizeof(pressure_history_t));
    Serial.println("1. Remove invalid and outdated items");
    for( i = 0; i < PRESSURE_HISTORY_SIZE; i++ ) 
    {
        if(memcmp((uint8_t*)&pressure_history_item[i], (uint8_t*)(&empty_item), sizeof(pressure_history_t)) == 0 ) 
        {
            //Serial.printf("Skip empty item [%d].\r\n", i);
        }
        // else if(i==0||i==1) {
        //     // clear item
        //     memset((uint8_t*)&(pressure_history_item[i]), 0xFF, sizeof(pressure_history_t) );
        //     Serial.printf("Temporary code: Remove hole[%d]\r\n", i);
        // } 
        else
        if( ( (pressure_history_item[i].time > time || // invalid item time
               pressure_history_item[i].time < (time - PRESSURE_HISTORY_SIZE*COLLECT_PRESSURE_HISTORY_PERIOD))) || // outdated item
            !(pressure_history_item[i].pressure >= 700 && pressure_history_item[i].pressure < 801) )  // invalid item pressure
        {
            //Serial.printf("Item [%d] time : %lu\r\n", i, pressure_history_item[i].time);
            if( pressure_history_item[i].time > time ) Serial.printf("Delete item [%d], it has invalid big time %lu\r\n", i, pressure_history_item[i].time);
            else if( pressure_history_item[i].time < (time - PRESSURE_HISTORY_SIZE*COLLECT_PRESSURE_HISTORY_PERIOD) )  Serial.printf("Delete item [%d], it has invalid old time %lu\r\n", i, pressure_history_item[i].time);
            else if( pressure_history_item[i].pressure < 700 ) Serial.printf("Delete item [%d], it has invalid low pressure %f\r\n", i, pressure_history_item[i].pressure);
            else if( pressure_history_item[i].pressure > 800 ) Serial.printf("Delete item [%d], it has invalid high pressure %f\r\n", i, pressure_history_item[i].pressure);
            else 
                Serial.printf("Unknown reason to delete item [%d], pressure=%f, time=%lu\r\n", 
                    i,
                    pressure_history_item[i].pressure, 
                    pressure_history_item[i].time);
            // clear item
            memset((uint8_t*)&(pressure_history_item[i]), 0xFF, sizeof(pressure_history_t) );
        } 
        else 
        {
            Serial.printf("Keep item [%d], pressure: %f mm, time: %lu sec\r\n", 
                i,
                pressure_history_item[i].pressure, 
                pressure_history_item[i].time);
        }
    }
                
    Serial.println("2. Sort pressure history by time");
    for( i = 1; i < PRESSURE_HISTORY_SIZE; i++ ) 
    {
        //Serial.printf("Move item from %d ", i);
        for( j = i; j > 0 && pressure_history_item[j-1].time > pressure_history_item[j].time; j-- )
        {
            pressure_history_t tmp;
            tmp.time = pressure_history_item[j-1].time;
            tmp.pressure = pressure_history_item[j-1].pressure;
            
            pressure_history_item[j-1].time = pressure_history_item[j].time;
            pressure_history_item[j-1].pressure = pressure_history_item[j].pressure;

            pressure_history_item[j].time = tmp.time;
            pressure_history_item[j].pressure = tmp.pressure;
        }
        
        //Serial.printf("to %d\r\n", j);
    }

    // Determine the size of the history
    Serial.println("3. Determine the size of the history");
    while( pressure_history_size < PRESSURE_HISTORY_SIZE && pressure_history_item[pressure_history_size].time != (unsigned long)-1 )
    {
        pressure_history_size++;
    }
    pressure_history_end = pressure_history_size;
    if( pressure_history_end == PRESSURE_HISTORY_SIZE )
    {
        pressure_history_end = 0;
    }
    Serial.printf("History: start=%d, end=%d, size=%d\r\n", pressure_history_start, pressure_history_end, pressure_history_size);

    // Update pressure history in eeprom
    for( uint16_t i = 0; i < (PRESSURE_HISTORY_SIZE*EEPROM_HISTORY_ITEM_SIZE); i++) {
        eepStatus = eeprom.write( EEPROM_HISTORY_ADDR+i, ((uint8_t*)pressure_history_item)[i] );
    }

    // Add holes
    Serial.println("4. Add holes");
    if( pressure_history_size > 0 )
    {
        int last_item_index;
        if( pressure_history_end == 0 ) {
            last_item_index = PRESSURE_HISTORY_SIZE-1;
        } else {
            last_item_index = pressure_history_end-1;
        }
        unsigned long last_item_time = 
            (pressure_history_item[last_item_index].time + COLLECT_PRESSURE_HISTORY_PERIOD/2) / 
            COLLECT_PRESSURE_HISTORY_PERIOD;
        last_item_time *= COLLECT_PRESSURE_HISTORY_PERIOD;

        Serial.printf("Last item [%d] time %lu sec\r\n", last_item_index, last_item_time);

        if( last_item_time > (time-COLLECT_PRESSURE_HISTORY_PERIOD*PRESSURE_HISTORY_SIZE) ) {
            while( last_item_time < (time-COLLECT_PRESSURE_HISTORY_PERIOD) )
            {
                last_item_time += COLLECT_PRESSURE_HISTORY_PERIOD;
                eeprom_add_history_item( last_item_time, 800 );
                Serial.printf("Add hole item with time %lu sec\r\n", last_item_time);
            }
        } else {
            memset( &pressure_history_item, 0xff, PRESSURE_HISTORY_SIZE*EEPROM_HISTORY_ITEM_SIZE );
            pressure_history_start = 0;
            pressure_history_end = 0;
            pressure_history_size = 0;
        }
    }

    // calculate timeout
    UpdatePressureCollectionTimer(time);

    generate_pressure_history();
}

void UpdatePressureCollectionTimer(unsigned long time)
{
    uint16_t downcounter = COLLECT_PRESSURE_HISTORY_PERIOD - (time%COLLECT_PRESSURE_HISTORY_PERIOD);
    swTimer[SW_TIMER_COLLECT_PRESSURE_HISTORY].SetDowncounter(downcounter);
    swTimer[SW_TIMER_COLLECT_PRESSURE_HISTORY].SetTriggered(false);
    Serial.printf("Time until next pressure collection: %u sec\r\nTime of next collection: %lu sec\r\n", downcounter, time+downcounter );

}

void clear_log(void) {
    for( uint16_t i = 0; i < (PRESSURE_HISTORY_SIZE*sizeof(pressure_history_t)); i++ ) {
        eeprom.write( EEPROM_HISTORY_ADDR + i, 0xff );
    }
}