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

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

volatile bool softreset = false;
bool rtc_require_update = false;
bool time_sync_with_ntp = false;
bool time_to_update_from_ntp = false;

Adafruit_BMP280 bmp; // I2C
bool bmp_sensor_present = false;
float temperature = 0;
float pressure = 0;
float altitude = 0;

#define PRESSURE_HISTORY_SIZE   96
float pressure_history[PRESSURE_HISTORY_SIZE] = {0}; // 2 days every 30min
uint16_t pressure_history_size = 0;
uint16_t pressure_history_start = 0;
uint16_t pressure_history_end = 0;

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
    display_printstarting();

    // load config
    config_read();

    rtc_Init();

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


    if( config.wifi.valid ) {
        WiFi.begin( config.wifi.name, config.wifi.password );
        if (testWifi()) {
            launchWeb(WEB_PAGES_NORMAL);

            Serial.printf("Reading time offset... %d hours %02d minute(s)\r\n", config.clock.hour_offset, config.clock.minute_offset);
            if( config.clock.hour_offset > 12 || config.clock.hour_offset < -12 || config.clock.minute_offset < 0 || config.clock.minute_offset > 59 ) {
                config_settimeoffset(0, 0);
            }
            
            timeClient.begin();
            rtc_require_update = true;
            time_sync_with_ntp = true;
            time_to_update_from_ntp = true; // update RTC module time with time from network each start
            return;
        } 
        else 
        {
            Serial.println("WiFi connection wasn't established. Switch to AP.");
        }
    }
    else
    {
        Serial.println("EEPROM doesn't contain WiFi connection information.");
        Serial.println("Switch to AP mode immediately.");
    }
    time_sync_with_ntp = false;
    setupAP();
}

//-----------------------------------------------------------------------------------------------------------
enum dispays_en {
    DISPLAY_CLOCK = 0,
    DISPLAY_TEMPERATURE,
    DISPLAY_PRESSURE
};

uint8_t show_display = DISPLAY_CLOCK;
uint8_t last_shown_display = DISPLAY_CLOCK;

void loop() {
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

    if(time_sync_with_ntp) {
        if( rtc_wasUpdated() && rtc_SecondsSinceUpdate > 0 && (rtc_SecondsSinceUpdate%60) == 0) // It's time to update from time server
        {
            Serial.println("It's time to update from time server");
            if( timeClient.forceUpdate() )
            {
                Serial.println("Time updated");
                rtc_SecondsSinceUpdate = 0;
            }
            time_to_update_from_ntp = false;
        }
        if( rtc_require_update ) {
            Serial.println("Updating RTC module");
            uint32_t epoch_time = timeClient.getRawEpochTime() + rtc_SecondsSinceUpdate;
            Serial.printf("Updating RTC with epoch time %u... ", epoch_time);
            rtc_SetEpoch(epoch_time);
            rtc_require_update = false;
            Serial.println("done");
        }
    } else {
        if( rtc_wasUpdated() && rtc_SecondsSinceUpdate > 0 && (rtc_SecondsSinceUpdate%600) == 0 ) {
            rtc_GetDT(&rtc_dt);
        }
    }

    if( sw_timer[SW_TIMER_COLLECT_PRESSURE_HISTORY].triggered && pressure != 0 ) {
        sw_timer[SW_TIMER_COLLECT_PRESSURE_HISTORY].triggered = false;
        pressure_history[pressure_history_end++] = pressure;
        if( pressure_history_end == PRESSURE_HISTORY_SIZE )
            pressure_history_end = 0;
        if( pressure_history_end == pressure_history_start)
        {
            pressure_history_end++;
            if( pressure_history_start == PRESSURE_HISTORY_SIZE )
                pressure_history_start = 0;
        }
        else
        {
            pressure_history_size++;
        }
        pressureLabelsStr = "";
        pressureValuesStr = "";
        Serial.println("----- Pressure history -----");
        String html_PressureHistory_line1 = "|     Time |";
        String html_PressureHistory_line2 = "+----------+";
        String html_PressureHistory_line3 = "| Pressure |";
        uint16_t time = pressure_history_size-1;
        for(uint16_t i = pressure_history_start; i != pressure_history_end; )
        {
            char line[20];
            sprintf( line, " %dh%02dmin |", time/2, ((time&1)?30:0));
            pressureLabelsStr += "\"";
            pressureLabelsStr += time/2;
            pressureLabelsStr += "h";
            if(time&1) {
                pressureLabelsStr += "30min";
            }

            pressureLabelsStr += "\"";
            pressureValuesStr += (int)pressure_history[i];

            html_PressureHistory_line1 += line;
            html_PressureHistory_line2 += "---------+";
            sprintf( line, "   %03dmm |", (int)pressure_history[i]);
            html_PressureHistory_line3 += line;
            Serial.println(pressure_history[i]);
            i++;
            if( i == PRESSURE_HISTORY_SIZE )
                i = 0;
            if( time ) {
                pressureLabelsStr += ",";
                pressureValuesStr += ",";
            }
            time--;
        }
        html_PressureHistory =  
                                "<p>" + html_PressureHistory_line1 + "</p>" +
                                "<p>" + html_PressureHistory_line2 + "</p>" +
                                "<p>" + html_PressureHistory_line3 + "</p>";
        Serial.println("----------------------------");
    }

    switch(show_display) {
        case DISPLAY_CLOCK:
            if( rtc_wasUpdated() )
            {
                int8_t hours = 0;
                int8_t minutes = 0;
                int8_t seconds = 0;
                if(time_sync_with_ntp) {
                    unsigned long time = timeClient.getRawEpochTime() + config.clock.hour_offset*3600 + config.clock.minute_offset*60 + rtc_SecondsSinceUpdate;
                    hours = (time % 86400L) / 3600;
                    minutes = (time % 3600) / 60;
                    seconds = time % 60;
                    if( hours == 3 && minutes == 0 && seconds == 0 ) // update time in RTC module each day at 3:00am
                    {
                        rtc_require_update = true;
                    }
                } else {
                    DateTime dt = rtc_dt + TimeSpan( rtc_SecondsSinceUpdate/(3600*24), config.clock.hour_offset + (rtc_SecondsSinceUpdate/3600)%24, config.clock.minute_offset + (rtc_SecondsSinceUpdate/60)%60, rtc_SecondsSinceUpdate%60 );
                    hours = dt.hour();
                    minutes = dt.minute();
                }
                display_printtime( hours, minutes, digitalRead(RTC_SQW_PIN), DISPLAY_FORMAT_24H );
                rtc_updated();
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
                display_printpressure((uint16_t)pressure);
            last_shown_display = DISPLAY_PRESSURE;
            break;
    }

    if( sw_timer[SW_TIMER_SENSOR_UPDATE].triggered ) {
        sw_timer[SW_TIMER_SENSOR_UPDATE].triggered = false;
        read_bmp_sensor();
    }

    if( sw_timer[SW_TIMER_SWITCH_DISPLAY].triggered ) {
        //Serial.printf("Time to switch display %d\r\n", show_display);
        switch(show_display) {
            case DISPLAY_CLOCK:
                show_display = DISPLAY_TEMPERATURE;
                sw_timer[SW_TIMER_SWITCH_DISPLAY].update_time = PRESSURE_SHOW_TIME;
                break;
            case DISPLAY_TEMPERATURE:
                show_display = DISPLAY_PRESSURE;
                sw_timer[SW_TIMER_SWITCH_DISPLAY].update_time = CLOCK_SHOW_TIME;
                break;
            case DISPLAY_PRESSURE:
                show_display = DISPLAY_CLOCK;
                sw_timer[SW_TIMER_SWITCH_DISPLAY].update_time = TEMPERATURE_SHOW_TIME;
                break;
        }
        sw_timer[SW_TIMER_SWITCH_DISPLAY].triggered = false;
    }
    delay(1);
}