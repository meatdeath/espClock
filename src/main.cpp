#include <Arduino.h>

// #include <ESP8266WiFi.h>
// #include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

#include "display.h"
#include "config.h"
#include "rtc.h"
#include "web.h"
#include "pressure_history.h"

#include <NTPClient.h>
#include <extEEPROM.h>

extEEPROM eeprom(kbits_32, 1, 64, 0x57); // device size, number of devices, page size

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

volatile bool softreset = false;
bool time_sync_with_ntp_enabled = false;
bool time_in_sync_with_ntp = false;

Adafruit_BMP280 bmp; // I2C
bool bmp_sensor_present = false;
float temperature = 0;
float pressure = 0;
float altitude = 0;

const int analogInPin = A0; // ESP8266 Analog Pin ADC0 = A0

//--------------------------------------------------------------------------------------------------------

void read_bmp_sensor()
{
    if (bmp_sensor_present)
    {
        temperature = bmp.readTemperature();
        pressure = bmp.readPressure();
        altitude = bmp.readAltitude(1013.25); /* Adjusted to local forecast! */
        pressure *= 0.00750062;
    }
}

void setup()
{

    pinMode(RTC_SQW_PIN, INPUT);
    delay(1000);
    Serial.begin(115200);
    delay(10);
    Serial.println();
    Serial.println();
    Serial.println("Startup");
    delay(1000);

    //-----------

    uint8_t eepStatus;
    Serial.println("Init EEPROM...");
    eepStatus = eeprom.begin(eeprom.twiClock400kHz);
    if (eepStatus)
    {
        Serial.print(F("extEEPROM.begin() failed, status = "));
        Serial.println(eepStatus);
        while (1)
            ;
    }

    // Init led display
    Serial.println("Init MAX7219...");
    display_Init();
    display_SetBrightness(20);

    // Print loading on led screen
    display_PrintStarting();

    // load config
    config_init();

    Serial.printf("Time offset... %d hours %02d minute(s)\r\n", config_clock.hour_offset, config_clock.minute_offset);

    rtc_Init();

    rtc_GetDT(&rtc_dt);
    Serial.printf("RTC time: %02d:%02d:%02d\r\n", rtc_dt.hour(), rtc_dt.minute(), rtc_dt.second());

    UpdatePressureCollectionTimer(rtc_dt.secondstime());

    if (!bmp.begin(BMP280_ADDRESS_ALT))
    {
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                         "try a different address!"));
    }
    else
    {
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
        Serial.print(0.00750062 * pressure);
        Serial.println("mm Hg");

        Serial.print(F("Approx altitude = "));
        Serial.print(altitude);
        Serial.println(" m");

        Serial.println();
        bmp_sensor_present = true;
    }

    WifiState = STATE_WIFI_IDLE;

    time_sync_with_ntp_enabled = false;
    time_in_sync_with_ntp = false;

    Serial.println("Restore history...");
    unsigned long time;
    time = rtc_dt.secondstime() + RTC_SECONDS_2000_01_01 + rtc_SecondsSinceUpdate;
    Serial.printf("GMT time synched with RTC module: %lu\r\n", time);
    eeprom_restore_pressure_history(time);

    web_init();
}

//-----------------------------------------------------------------------------------------------------------
enum dispays_en
{
    DISPLAY_CLOCK = 0,
    DISPLAY_TEMPERATURE,
    DISPLAY_PRESSURE
};

uint8_t show_display = DISPLAY_CLOCK;
uint8_t last_shown_display = DISPLAY_CLOCK;
uint8_t intensity = 15;
uint8_t measured_intensity = 1;
unsigned long ntp_time = 0;

void loop()
{
    int ambianceValue = 0; // value abiance read from the port
    wifi_processing();

    if (softreset == true)
    {
        Serial.println("The board will reset in 10s ");
        for (int i = 0; i < 10; i++)
        {
            Serial.print(".");
            delay(1000);
        }
        Serial.println(" reset");
        delay(100);
        ESP.reset();
    }

    if (time_sync_with_ntp_enabled)
    {
        if (swTimer[SW_TIMER_NTP_TIME_UPDATE].IsTriggered(true)) // It's time to update from time server
        {
            if (timeClient.forceUpdate())
            {
                static uint16_t read_time_lock_count = 0;
                static uint16_t read_time_unlock_count = 0;
                if (read_time_lock_count < 100)
                {
                    if (read_time_lock_count > 0)
                    {
                        if ((ntp_time + rtc_SecondsSinceUpdate + 30) < timeClient.getRawEpochTime() ||
                            (ntp_time + rtc_SecondsSinceUpdate - 30) > timeClient.getRawEpochTime())
                        {
                            read_time_lock_count = 0;
                        }
                    }
                    ntp_time = timeClient.getRawEpochTime();
                    time_in_sync_with_ntp = true;
                    read_time_lock_count++;
                    rtc_SecondsSinceUpdate = 0;
                }
                else
                {
                    if ((ntp_time + rtc_SecondsSinceUpdate + 30) < timeClient.getRawEpochTime() ||
                        (ntp_time + rtc_SecondsSinceUpdate - 30) > timeClient.getRawEpochTime())
                    {
                        read_time_unlock_count++;
                        if (read_time_unlock_count == 100)
                        {
                            read_time_lock_count = 0;
                            read_time_unlock_count = 0;
                        }
                    }
                    else
                    {
                        ntp_time = timeClient.getRawEpochTime();
                        time_in_sync_with_ntp = true;
                        read_time_unlock_count = 0;
                        rtc_SecondsSinceUpdate = 0;
                    }
                }
                Serial.printf("Time after update from NTP server %02d:%02d:%02d (%lu)... \r\n",
                              (ntp_time / (60 * 60)) % 24,
                              (ntp_time / 60) % 60,
                              ntp_time % 60,
                              ntp_time);
            }
        }
        if (swTimer[SW_TIMER_RTC_MODULE_UPDATE].IsTriggered(true))
        {
            uint32_t epoch_time = /*timeClient.getRawEpochTime()*/ ntp_time + rtc_SecondsSinceUpdate;
            // Serial.printf("Updating RTC module with epoch time %u... ", epoch_time);

            rtc_SetEpoch(epoch_time);
            UpdatePressureCollectionTimer(epoch_time);
            // Serial.println("done");
        }
    }
    else
    {
        if (swTimer[SW_TIMER_GET_TIME_FROM_RTC_MODULE].IsTriggered(true))
        {
            rtc_GetDT(&rtc_dt);
        }
    }

    if (swTimer[SW_TIMER_GET_AMBIANCE].IsTriggered(true) && pressure != 0)
    {
        ambianceValue = analogRead(analogInPin);
        // 3.3v -> 1024
        // 2.4v -> 740
        // 0.8v -> 250
        const int lower_light = 250;
        const int higher_light = 740;
        measured_intensity = 1;
        if (ambianceValue > higher_light)
        {
            measured_intensity = 15;
        }
        else if (ambianceValue > lower_light)
        {
            measured_intensity = ((double)(ambianceValue - lower_light) / (higher_light - lower_light)) * 15 + 1;
        }
        // Serial.printf("ambiance : %d\r\n", ambianceValue);
        // Serial.printf("lightness : %d\r\n", 16-measured_intensity);
    }
    if (intensity != measured_intensity)
    {
        if (intensity > measured_intensity)
        {
            intensity--;
        }
        else if (intensity < measured_intensity)
        {
            intensity++;
        }
        display_SetIntensity(16 - intensity);
    }

    if (swTimer[SW_TIMER_COLLECT_PRESSURE_HISTORY].IsTriggered(true) && pressure != 0)
    {
        unsigned long timeinsec;
        Serial.print("Time to collect pressure history: ");
        if (time_in_sync_with_ntp)
        {
            timeinsec = /*timeClient.getRawEpochTime()*/ ntp_time + rtc_SecondsSinceUpdate;
            Serial.printf("Time from ntp %lu, pressure %3.1f\r\n", timeinsec, pressure);
        }
        else
        {
            timeinsec = rtc_dt.secondstime() + RTC_SECONDS_2000_01_01 + rtc_SecondsSinceUpdate;
            Serial.printf("Time from rtc module %lu, pressure %3.1f\r\n", timeinsec, pressure);
        }
        eeprom_add_history_item(timeinsec, pressure);
        generate_pressure_history();
    }

    switch (show_display)
    {
    case DISPLAY_CLOCK:
        if (rtc_LocalTimeRequireProcessing())
        {
            int8_t hours = 0;
            int8_t minutes = 0;
            // int8_t seconds = 0;
            if (time_in_sync_with_ntp)
            {
                unsigned long time = /*timeClient.getRawEpochTime()*/ 
                    (unsigned long)ntp_time + 
                    (unsigned long)config_clock.hour_offset * 3600 + 
                    (unsigned long)config_clock.minute_offset * 60 + 
                    (unsigned long)rtc_SecondsSinceUpdate;
                hours = (time / (60 * 60)) % 24;
                minutes = (time / 60) % 60;
                // seconds = time % 60;
                //Serial.printf("Time from NTP: %lu => %d:%d (%lu)\r\n", ntp_time, hours, minutes, time);
            }
            else
            {
                DateTime dt = rtc_dt +
                              TimeSpan(rtc_SecondsSinceUpdate / (60 * 60 * 24),
                                       config_clock.hour_offset + (rtc_SecondsSinceUpdate / 3600) % 24,
                                       config_clock.minute_offset + (rtc_SecondsSinceUpdate / 60) % 60,
                                       rtc_SecondsSinceUpdate % 60);
                hours = dt.hour();
                minutes = dt.minute();
                //Serial.printf("Time from RTC: %d:%d\r\n", hours, minutes);
            }
            display_Time(hours, minutes, digitalRead(RTC_SQW_PIN), DISPLAY_FORMAT_24H);
            rtc_SetLocalTimeProcessed();
            last_shown_display = DISPLAY_CLOCK;
        }
        break;
    case DISPLAY_TEMPERATURE:
        if (last_shown_display != DISPLAY_TEMPERATURE)
            display_Temperature((int)temperature);
        last_shown_display = DISPLAY_TEMPERATURE;
        break;
    case DISPLAY_PRESSURE:
        if (last_shown_display != DISPLAY_PRESSURE)
            display_Pressure((uint16_t)(pressure + .5));
        last_shown_display = DISPLAY_PRESSURE;
        break;
    }

    if (swTimer[SW_TIMER_SENSOR_UPDATE].IsTriggered(true))
    {
        read_bmp_sensor();
    }

    if (swTimer[SW_TIMER_SWITCH_DISPLAY].IsTriggered(true))
    {
        // Serial.printf("Time to switch display %d\r\n", show_display);
        switch (show_display)
        {
        case DISPLAY_CLOCK:
            show_display = DISPLAY_TEMPERATURE;
            swTimer[SW_TIMER_SWITCH_DISPLAY].SetUpdateTime(PRESSURE_SHOW_TIME);
            break;
        case DISPLAY_TEMPERATURE:
            show_display = DISPLAY_PRESSURE;
            swTimer[SW_TIMER_SWITCH_DISPLAY].SetUpdateTime(CLOCK_SHOW_TIME);
            break;
        case DISPLAY_PRESSURE:
            show_display = DISPLAY_CLOCK;
            swTimer[SW_TIMER_SWITCH_DISPLAY].SetUpdateTime(TEMPERATURE_SHOW_TIME);
            break;
        }
    }
    delay(1);
}

//---------------------------------------------------------

// LedMatrix.h
//  void setTextOffset(byte offset);
//  void Rotate90();

// LedMatrix.cpp
//  void LedMatrix::Rotate90() {
//      for( byte device = 0; device < myNumberOfDevices; device++ ) {
//          byte result[8] = {0};
//          for( byte i = 0; i < 8; i++ ) {
//              for( byte j = 0; j < 8; j++ ) {
//                  if( cols[i+device*8] & (1<<j) ) {
//                  //if( cols[i] & (1<<j) ) {
//                      result [7-j] |= 1<<i;
//                  } else {
//                      result [7-j] &= ~(1<<i);
//                  }
//              }
//          }
//          for( byte i = 0; i < 8; i++ ) {
//              cols[device*8+i] = result[i];
//              //cols[i] = result[i];
//          }
//      }
//  }

// void LedMatrix::setTextOffset(byte offset) {
//     myTextOffset = (myTextOffset-offset) % ((int)myText.length() * myCharWidth + myNumberOfDevices * 8);
//     if (myTextOffset == 0 && myNextText.length() > 0) {
//         myText = myNextText;
//         myNextText = "";
//         calculateTextAlignmentOffset();
//     }
// }

//----------------------------------------------------------

// NTPClient.h
//  unsigned long NTPClient::getRawEpochTime() {

// NTPClient.cpp
//  unsigned long NTPClient::getRawEpochTime() {
//    return this->_timeOffset + // User offset
//           this->_currentEpoc; // Epoc returned by the NTP server
//  }

//----------------------------------------------------------