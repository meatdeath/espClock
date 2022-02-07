#include <Arduino.h>

// #include <ESP8266WiFi.h>
// #include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

#include "display.h"
#include "config.h"
#include "rtc.h"
#include "web.h"

#include <NTPClient.h>
#include <extEEPROM.h>

class myNTPClient : public NTPClient {

  public:
    myNTPClient(UDP& udp): NTPClient{udp} {};
    unsigned long getRawEpochTime();
};

// TODO: reimplement timeClient.getRawEpochTime()
// TODO: reimplement ledMatrix.setTextOffset(...)
// TODO: reimplement ledMatrix.Rotate90()

extEEPROM eeprom(kbits_32, 1, 64, 0x57);         //device size, number of devices, page size
uint8_t eepStatus;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

volatile bool softreset = false;
bool time_sync_with_ntp_enabled = false;

Adafruit_BMP280 bmp; // I2C
bool bmp_sensor_present = false;
float temperature = 0;
float pressure = 0;
float altitude = 0;

#define PRESSURE_HISTORY_SIZE   24
uint16_t pressure_history_size = 0;
uint16_t pressure_history_start = 0;
uint16_t pressure_history_end = 0;

const int analogInPin = A0;  // ESP8266 Analog Pin ADC0 = A0

String dbg_text = "";

typedef struct pressure_history_st {
    unsigned long time;
    float pressure;
} pressure_history_t;

#define EEPROM_HISTORY_START_ADDR   128
#define EEPROM_HISTORY_ITEM_SIZE    sizeof(pressure_history_t)

pressure_history_t pressure_history_item[PRESSURE_HISTORY_SIZE] = {0};

void eeprom_add_history_item( unsigned long time, float pressure );
void eeprom_restore_pressure_history(unsigned long time);
void generate_pressure_history(void);
void clear_log(void);

//--------------------------------------------------------------------------------------------------------

void read_bmp_sensor() {
    if(bmp_sensor_present) {
        temperature = bmp.readTemperature();
        pressure = bmp.readPressure();
        altitude = bmp.readAltitude(1013.25); /* Adjusted to local forecast! */
        pressure *= 0.00750062;
    }
}

void setup() {
    
    pinMode(RTC_SQW_PIN, INPUT);
    delay(1000);
    Serial.begin(115200);
    delay(10);
    Serial.println();
    Serial.println();
    Serial.println("Startup");

    //-----------

    config_init();

    // Init led display
    Serial.println("Init MAX7219");
    display_init();
    display_brightness(20);

    // Print loading on led screen
    Serial.println("Print start screen");
    display_printstarting();

    // Init RTC module
    Serial.println("Init RTC module");
    rtc_Init();

    eepStatus = eeprom.begin(eeprom.twiClock400kHz);
    if (eepStatus) {
        Serial.print(F("extEEPROM.begin() failed, status = "));
        Serial.println(eepStatus);
        while (1);
        // eeprom.write(addr, *data);
        // eeprom.write(addr, data, len);
        // *data = eeprom.read(addr);
        // eeprom.read(addr, data, len);
    }

    // load config
    Serial.println("Load config");
    config_read();

    Serial.printf("Time offset... %d hours %02d minute(s)\r\n", config.clock.hour_offset, config.clock.minute_offset);
    if( config.clock.hour_offset > 12 || config.clock.hour_offset < -12 || config.clock.minute_offset < 0 || config.clock.minute_offset > 59 ) {
        // Reset time offset if invalid
        config_settimeoffset(0, 0);
    }

    rtc_GetDT( &rtc_dt );
    Serial.printf("RTC time: %02d:%02d:%02d\r\n", rtc_dt.hour(), rtc_dt.minute(), rtc_dt.second());

    if (!bmp.begin(BMP280_ADDRESS_ALT)) {
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                        "try a different address!"));
    } else {
            /* Default settings from datasheet. */
        bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                        Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                        Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                        Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                        Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

        temperature = bmp.readTemperature();
        pressure = bmp.readPressure();
        altitude = bmp.readAltitude(1013.25); /* Adjusted to local forecast! */
        pressure *= 0.00750062;
        Serial.print(F("Temperature = "));
        Serial.print(temperature);
        Serial.println(" *C");

        Serial.print(F("Pressure = "));
        Serial.print(pressure);
        Serial.print(" Pa = ");
        Serial.print(0.00750062*pressure);
        Serial.println("mm Hg");

        Serial.print(F("Approx altitude = "));
        Serial.print(altitude); 
        Serial.println(" m");

        Serial.println(); 
        bmp_sensor_present = true;                 
    }

    WifiState = STATE_WIFI_IDLE;

    Serial.println("Restore history...");
    unsigned long time;
    if(time_sync_with_ntp_enabled) {
        time = 
            timeClient.getRawEpochTime() + 
            // config.clock.hour_offset*3600 + 
            // config.clock.minute_offset*60 + 
            rtc_SecondsSinceUpdate;
        Serial.printf("GMT time synched with NTP: %lu\r\n", time);
    } else {
        time = EPOCH_2000_01_01__12_00_AM + rtc_dt.secondstime() + rtc_SecondsSinceUpdate;
        Serial.printf("GMT time synched with RTC module: %lu\r\n", time);
    }
    eeprom_restore_pressure_history(time);

    time_sync_with_ntp_enabled = false;
}

//-----------------------------------------------------------------------------------------------------------
enum dispays_en {
    DISPLAY_CLOCK = 0,
    DISPLAY_TEMPERATURE,
    DISPLAY_PRESSURE
};

uint8_t show_display = DISPLAY_CLOCK;
uint8_t last_shown_display = DISPLAY_CLOCK;
uint8_t intensity = 15;
uint8_t measured_intensity = 1;
extern StateWifi WifiState;
uint8_t wifi_sqw_state = 0xff;

void loop() {
    int ambianceValue = 0;  // value abiance read from the port
    unsigned long epoch_time;
    wifi_processing();

    if(softreset==true) {
        Serial.println("The board will reset in 10s ");
        for(int i = 0; i < 10; i++) {
            Serial.print(".");
            delay(1000);
        }
        Serial.println(" reset");
        delay(100);
        ESP.reset();
    }

    if( swTimerIsActive(SW_TIMER_WIFI_CONNECTING) ) {
            //Serial.print(".");
        uint8_t new_state = digitalRead(RTC_SQW_PIN);
        if( wifi_sqw_state != new_state )
            Serial.print(".");
        wifi_sqw_state = new_state;
    }

    if(time_sync_with_ntp_enabled) {
        if( swTimerIsTriggered(SW_TIMER_NTP_TIME_UPDATE, true) ) // It's time to update from time server
        {
            //Serial.println("It's time to update from time server");
            if( timeClient.forceUpdate() )
            {
                Serial.printf("Local time updated from NTP with %lus.\r\n", timeClient.getRawEpochTime());
                rtc_SecondsSinceUpdate = 0;
            }
        }
        if( swTimerIsTriggered(SW_TIMER_RTC_MODULE_UPDATE, true) ) {
            epoch_time = timeClient.getRawEpochTime() + rtc_SecondsSinceUpdate;
            Serial.printf("Updating RTC module with epoch time %lu... ", epoch_time);
            rtc_SetEpoch(epoch_time);
            Serial.println("done");
        }
    } else {
        if( swTimerIsTriggered(SW_TIMER_GET_TIME_FROM_RTC_MODULE, true) ) {
            rtc_GetDT(&rtc_dt);
        }
    }

    if( swTimerIsTriggered(SW_TIMER_GET_AMBIANCE, true) && pressure != 0 ) {
        ambianceValue = analogRead(analogInPin);
        // 3.3v -> 1024
        // 2.4v -> 740
        // 0.8v -> 250
        const int lower_light = 250;
        const int higher_light = 740;
        measured_intensity = 1;
        if( ambianceValue > higher_light ) {
            measured_intensity = 15;
        } else if (ambianceValue > lower_light ) {
            measured_intensity = ((double)(ambianceValue-lower_light)/(higher_light-lower_light))*15 + 1;
        }
        // Serial.printf("ambiance : %d\r\n", ambianceValue);
        // Serial.printf("lightness : %d\r\n", 16-measured_intensity);
    }
    if( intensity != measured_intensity ) {
        if( intensity > measured_intensity ) {
            intensity--;
        } else if( intensity < measured_intensity ) {
            intensity++;
        }
        display_intensity(16-intensity);
    }

    if( swTimerIsTriggered(SW_TIMER_COLLECT_PRESSURE_HISTORY, true) && pressure != 0 ) {
        unsigned long timeinsec;
        Serial.print("Time to collect pressure history: ");
        if(time_sync_with_ntp_enabled) {
            timeinsec = 
                timeClient.getRawEpochTime() + 
                //config.clock.hour_offset*3600 + 
                //config.clock.minute_offset*60 + 
                rtc_SecondsSinceUpdate;
            Serial.printf("Time from ntp %lu, pressure %3.1f\r\n", timeinsec, pressure);
        } else {
            timeinsec = EPOCH_2000_01_01__12_00_AM + rtc_dt.secondstime() + rtc_SecondsSinceUpdate;
            Serial.printf("Time from rtc module %lu, pressure %3.1f\r\n", timeinsec, pressure);
        }
        eeprom_add_history_item( timeinsec, pressure );
        generate_pressure_history();
    }

    switch(show_display) {
        case DISPLAY_CLOCK:
            if( rtc_LocalTimeRequireProcessing() )
            {
                int8_t hours = 0;
                int8_t minutes = 0;
                //int8_t seconds = 0;
                if(time_sync_with_ntp_enabled) {
                    unsigned long time = timeClient.getRawEpochTime() + config.clock.hour_offset*3600 + config.clock.minute_offset*60 + rtc_SecondsSinceUpdate;
                    hours = (time % 86400L) / 3600;
                    minutes = (time % 3600) / 60;
                    //seconds = time % 60;
                } else {
                    DateTime dt = rtc_dt + 
                        TimeSpan( rtc_SecondsSinceUpdate/(3600*24), 
                                    config.clock.hour_offset + (rtc_SecondsSinceUpdate/3600)%24, 
                                    config.clock.minute_offset + (rtc_SecondsSinceUpdate/60)%60, 
                                    rtc_SecondsSinceUpdate%60 );
                    hours = dt.hour();
                    minutes = dt.minute();
                }
                display_printtime( hours, minutes, digitalRead(RTC_SQW_PIN), DISPLAY_FORMAT_24H );
                rtc_SetLocalTimeProcessed();
                last_shown_display = DISPLAY_CLOCK;
            }
            break;
        case DISPLAY_TEMPERATURE:
            if(last_shown_display != DISPLAY_TEMPERATURE)
                display_printtemperature((int)temperature);
            last_shown_display = DISPLAY_TEMPERATURE;
            break;
        case DISPLAY_PRESSURE:
            if(last_shown_display != DISPLAY_PRESSURE)
                display_printpressure((uint16_t)(pressure+.5));
            last_shown_display = DISPLAY_PRESSURE;
            break;
    }

    if( swTimerIsTriggered(SW_TIMER_SENSOR_UPDATE,true) ) {
        read_bmp_sensor();
    }

    if( swTimerIsTriggered(SW_TIMER_SWITCH_DISPLAY,true) ) {
        //Serial.printf("Time to switch display %d\r\n", show_display);
        switch(show_display) {
            case DISPLAY_CLOCK:
                show_display = DISPLAY_TEMPERATURE;
                sw_timer[SW_TIMER_SWITCH_DISPLAY].updatetime = PRESSURE_SHOW_TIME;
                break;
            case DISPLAY_TEMPERATURE:
                show_display = DISPLAY_PRESSURE;
                sw_timer[SW_TIMER_SWITCH_DISPLAY].updatetime = CLOCK_SHOW_TIME;
                break;
            case DISPLAY_PRESSURE:
                show_display = DISPLAY_CLOCK;
                sw_timer[SW_TIMER_SWITCH_DISPLAY].updatetime = TEMPERATURE_SHOW_TIME;
                break;
        }
    }
    delay(1);
}

void generate_pressure_history(void) {
    pressureLabelsStr = "";
    pressureValuesStr = "";
    Serial.println("----- Pressure history -----");
    //html_PressureHistory = "<table><tr><td>Time ago</td><td>Pressure, mm</td></tr>";
    html_PressureHistory = "{\"current\":";
    html_PressureHistory += pressure;
    html_PressureHistory += "\"history\":[";
    uint16_t time = pressure_history_size-1;
    dbg_text = "";
    for(uint16_t i = pressure_history_start; i != pressure_history_end; )
    {
        if( i != pressure_history_start ) 
            html_PressureHistory += ",";
        html_PressureHistory += "{\"time\":";
        html_PressureHistory += pressure_history_item[i].time;
        html_PressureHistory += ",\"value\":";
        html_PressureHistory += pressure_history_item[i].pressure;
        html_PressureHistory += "}";
        //html_PressureHistory += "<tr><td>";
        pressureLabelsStr += "\"";
        //html_PressureHistory += time*2;
        //html_PressureHistory += "h";
        if((time&1) == 0) {
            pressureLabelsStr += time*2;
            pressureLabelsStr += "h";
        }
        pressureLabelsStr += "\"";

        //html_PressureHistory += "</td><td>";

        if( pressure_history_item[i].pressure != 800 )
        {
            pressureValuesStr += pressure_history_item[i].pressure;
            //html_PressureHistory += pressure_history_item[i].pressure;
        }
        else
        {
            pressureValuesStr += "null";
            //html_PressureHistory += '-';
        }

        //html_PressureHistory += "</td></tr>";

        char dbgstr[50];
        sprintf(dbgstr,"T: %lu  P: %3.1f\r\n", pressure_history_item[i].time, pressure_history_item[i].pressure);
        Serial.print(dbgstr);
        dbg_text += "<p>"+(String)dbgstr+"</p>";
        i++;
        if( i == PRESSURE_HISTORY_SIZE )
            i = 0; 
        if( time ) {
            pressureLabelsStr += ",";
            pressureValuesStr += ",";
        }
        time--;
    }
    //html_PressureHistory += "</table>";

    html_PressureHistory += "]}";
    Serial.println("----------------------------");
}


void eeprom_add_history_item( unsigned long time, float pressure ) {
    unsigned long addr = EEPROM_HISTORY_START_ADDR + pressure_history_end*EEPROM_HISTORY_ITEM_SIZE;
    Serial.printf("Adding history item: time=%lu, pressure=%3.1f\r\n", time, (double)pressure );
    pressure_history_item[pressure_history_end].time = time;
    pressure_history_item[pressure_history_end].pressure = pressure;
    eepStatus = eeprom.write( addr, (uint8_t*)&pressure_history_item[pressure_history_end], EEPROM_HISTORY_ITEM_SIZE );
    Serial.printf("EEPROM status: %d\r\n", eepStatus);
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

    Serial.println("Read history from EEPROM");
    eepStatus = eeprom.read( EEPROM_HISTORY_START_ADDR, 
                            (uint8_t*)(&pressure_history_item[0]), 
                            PRESSURE_HISTORY_SIZE*EEPROM_HISTORY_ITEM_SIZE );
    Serial.printf("EEPROM status: %d\r\n", eepStatus);

    // Remove invalid and outdated items
    Serial.println("1. Remove invalid and outdated items");
    for( i = 1; i < PRESSURE_HISTORY_SIZE; i++ ) 
    {
        if( pressure_history_item[i].time > time || // invalid item time
            pressure_history_item[i].time < (time - PRESSURE_HISTORY_SIZE*COLLECT_PRESSURE_HISTORY_PERIOD) || // outdated item
            pressure_history_item[i].pressure < 700 || // invalid item pressure
            pressure_history_item[i].pressure > 800 )  // invalid item pressure
        {
            // clear item
            memset((uint8_t*)&(pressure_history_item[i]), 0xFF, sizeof(pressure_history_t) );
        } 
        else 
        {
            Serial.printf("Keep item on index=%d, pressure=%f, time=%lu\r\n", 
                i,
                pressure_history_item[i].pressure, 
                pressure_history_item[i].time);
        }
    }
                
    Serial.println("2. Sort pressure history by time");
    for( i = 1; i < PRESSURE_HISTORY_SIZE; i++ ) 
    {
        Serial.printf("Move item from %d ", i);
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
        
        Serial.printf("to %d\r\n", j);
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
    eepStatus = eeprom.write( EEPROM_HISTORY_START_ADDR, 
                                (uint8_t*)(&pressure_history_item[0]), 
                                PRESSURE_HISTORY_SIZE*EEPROM_HISTORY_ITEM_SIZE );
    Serial.printf("EEPROM status: %d\r\n", eepStatus);

    // Add holes
    Serial.println("4. Add holes");
    if( pressure_history_size > 0 )
    {
        int last_item_index;
        if( pressure_history_end == 0 ) {
            last_item_index = PRESSURE_HISTORY_SIZE-1;
        } else {
            last_item_index == pressure_history_end-1;
        }
        unsigned long last_item_time = pressure_history_item[last_item_index].time;
        Serial.printf("Last item time %lu\r\n", last_item_time);

        if( last_item_time > (time-COLLECT_PRESSURE_HISTORY_PERIOD*PRESSURE_HISTORY_SIZE) ) {
            while( last_item_time < (time-COLLECT_PRESSURE_HISTORY_PERIOD) )
            {
                last_item_time += COLLECT_PRESSURE_HISTORY_PERIOD;
                eeprom_add_history_item( last_item_time, 800 );
                Serial.printf("Add hole item with time %lu\r\n", last_item_time);
            }
        } else {
            memset( &pressure_history_item, 0xff, PRESSURE_HISTORY_SIZE*EEPROM_HISTORY_ITEM_SIZE );
            pressure_history_start = 0;
            pressure_history_end = 0;
            pressure_history_size = 0;
        }

        // calculate timeout
        sw_timer[SW_TIMER_COLLECT_PRESSURE_HISTORY].triggered = false;
        sw_timer[SW_TIMER_COLLECT_PRESSURE_HISTORY].downcounter = 
                    (COLLECT_PRESSURE_HISTORY_PERIOD - (last_item_time%COLLECT_PRESSURE_HISTORY_PERIOD) + 5);
        Serial.printf("Time until next pressure collection: %lus\r\n", sw_timer[SW_TIMER_COLLECT_PRESSURE_HISTORY].downcounter);
    } else {
        // calculate timeout
        sw_timer[SW_TIMER_COLLECT_PRESSURE_HISTORY].triggered = false;
        sw_timer[SW_TIMER_COLLECT_PRESSURE_HISTORY].downcounter = 
                    (COLLECT_PRESSURE_HISTORY_PERIOD - (time%COLLECT_PRESSURE_HISTORY_PERIOD) + 5);

    }

    generate_pressure_history();
}

void clear_log(void) {
    for( int i = 0; i < (PRESSURE_HISTORY_SIZE*sizeof(pressure_history_t)); i++ ) {
        eeprom.write( EEPROM_HISTORY_START_ADDR + i, 0xff );
    }
}
