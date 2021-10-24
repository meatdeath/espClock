#include <Arduino.h>

// #include <ESP8266WiFi.h>
// #include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <DNSServer.h>
#include <Hash.h>
#include "ESPAsyncWebServer.h"
#include <EEPROM.h>
#include <google-tts.h>

#include <ESP8266HTTPClient.h>
// #include <SD.h>
// #include "AudioFileSourcePROGMEM.h"
// #include "AudioGeneratorTalkie.h"
// #include "AudioOutputI2S.h"

#include "display.h"

#include <NTPClient.h>
#include <WiFiUdp.h>


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
int8_t timeOffset = 0;





int x = 0;

// uint8_t spGOOD[]      PROGMEM = {0x0A,0x28,0xCD,0x34,0x20,0xD9,0x1A,0x45,0x74,0xE4,0x66,0x24,0xAD,0xBA,0xB1,0x8C,0x9B,0x91,0xA5,0x64,0xE6,0x98,0x21,0x16,0x0B,0x96,0x9B,0x4C,0xE5,0xFF,0x01};
// uint8_t spMORNING[]   PROGMEM = {0xCE,0x08,0x52,0x2A,0x35,0x5D,0x39,0x53,0x29,0x5B,0xB7,0x0A,0x15,0x0C,0xEE,0x2A,0x42,0x56,0x66,0xD2,0x55,0x2E,0x37,0x2F,0xD9,0x45,0xB3,0xD3,0xC5,0xCA,0x6D,0x27,0xD5,0xEE,0x50,0xF5,0x50,0x94,0x14,0x77,0x2D,0xD8,0x5D,0x49,0x92,0xFD,0xB1,0x64,0x2F,0xA9,0x49,0x0C,0x93,0x4B,0xAD,0x19,0x17,0x3E,0x66,0x1E,0xF1,0xA2,0x5B,0x84,0xE2,0x29,0x8F,0x8B,0x72,0x10,0xB5,0xB1,0x2E,0x4B,0xD4,0x45,0x89,0x4A,0xEC,0x5C,0x95,0x14,0x2B,0x8A,0x9C,0x34,0x52,0x5D,0xBC,0xCC,0xB5,0x3B,0x49,0x69,0x89,0x87,0xC1,0x98,0x56,0x3A,0x21,0x2B,0x82,0x67,0xCC,0x5C,0x85,0xB5,0x4A,0x8A,0xF6,0x64,0xA9,0x96,0xC4,0x69,0x3C,0x52,0x81,0x58,0x1C,0x97,0xF6,0x0E,0x1B,0xCC,0x0D,0x42,0x32,0xAA,0x65,0x12,0x67,0xD4,0x6A,0x61,0x52,0xFC,0xFF};
// uint8_t spAFTERNOON[] PROGMEM = {0xC7,0xCE,0xCE,0x3A,0xCB,0x58,0x1F,0x3B,0x07,0x9D,0x28,0x71,0xB4,0xAC,0x9C,0x74,0x5A,0x42,0x55,0x33,0xB2,0x93,0x0A,0x09,0xD4,0xC5,0x9A,0xD6,0x44,0x45,0xE3,0x38,0x60,0x9A,0x32,0x05,0xF4,0x18,0x01,0x09,0xD8,0xA9,0xC2,0x00,0x5E,0xCA,0x24,0xD5,0x5B,0x9D,0x4A,0x95,0xEA,0x34,0xEE,0x63,0x92,0x5C,0x4D,0xD0,0xA4,0xEE,0x58,0x0C,0xB9,0x4D,0xCD,0x42,0xA2,0x3A,0x24,0x37,0x25,0x8A,0xA8,0x8E,0xA0,0x53,0xE4,0x28,0x23,0x26,0x13,0x72,0x91,0xA2,0x76,0xBB,0x72,0x38,0x45,0x0A,0x46,0x63,0xCA,0x69,0x27,0x39,0x58,0xB1,0x8D,0x60,0x1C,0x34,0x1B,0x34,0xC3,0x55,0x8E,0x73,0x45,0x2D,0x4F,0x4A,0x3A,0x26,0x10,0xA1,0xCA,0x2D,0xE9,0x98,0x24,0x0A,0x1E,0x6D,0x97,0x29,0xD2,0xCC,0x71,0xA2,0xDC,0x86,0xC8,0x12,0xA7,0x8E,0x08,0x85,0x22,0x8D,0x9C,0x43,0xA7,0x12,0xB2,0x2E,0x50,0x09,0xEF,0x51,0xC5,0xBA,0x28,0x58,0xAD,0xDB,0xE1,0xFF,0x03};
// uint8_t spEVENING[]   PROGMEM = {0xCD,0x6D,0x98,0x73,0x47,0x65,0x0D,0x6D,0x10,0xB2,0x5D,0x93,0x35,0x94,0xC1,0xD0,0x76,0x4D,0x66,0x93,0xA7,0x04,0xBD,0x71,0xD9,0x45,0xAE,0x92,0xD5,0xAC,0x53,0x07,0x6D,0xA5,0x76,0x63,0x51,0x92,0xD4,0xA1,0x83,0xD4,0xCB,0xB2,0x51,0x88,0xCD,0xF5,0x50,0x45,0xCE,0xA2,0x2E,0x27,0x28,0x54,0x15,0x37,0x0A,0xCF,0x75,0x61,0x5D,0xA2,0xC4,0xB5,0xC7,0x44,0x55,0x8A,0x0B,0xA3,0x6E,0x17,0x95,0x21,0xA9,0x0C,0x37,0xCD,0x15,0xBA,0xD4,0x2B,0x6F,0xB3,0x54,0xE4,0xD2,0xC8,0x64,0xBC,0x4C,0x91,0x49,0x12,0xE7,0xB2,0xB1,0xD0,0x22,0x0D,0x9C,0xDD,0xAB,0x62,0xA9,0x38,0x53,0x11,0xA9,0x74,0x2C,0xD2,0xCA,0x59,0x34,0xA3,0xE5,0xFF,0x03};


// AudioGeneratorTalkie *talkie;
// AudioOutputI2S *out;

// void sayTime(AudioGeneratorTalkie *talkie)
// {
//     talkie->say(spGOOD, sizeof(spGOOD));
//     talkie->say(spMORNING, sizeof(spMORNING));
//     talkie->say(spGOOD, sizeof(spGOOD));
//     talkie->say(spAFTERNOON, sizeof(spAFTERNOON));
//     talkie->say(spGOOD, sizeof(spGOOD));
//     talkie->say(spEVENING, sizeof(spEVENING));
// }

// ESP8266WebServer server(80);

enum _web_pages_en { 
    WEB_PAGES_NORMAL = 0,
    WEB_PAGES_FOR_AP  
};

IPAddress apIP(192,168,4,22);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);


DNSServer dnsServer;
AsyncWebServer server(80);
const char* ssid = "ESPtest";
const char* passphrase = "1290test$#A";
volatile bool softreset = false;
int statusCode;
String content;
String st;

bool testWifi(void);
void launchWeb(int webtype);
void setupAP(void);
void createWebServer(int webtype);


typedef struct _config_st {
    struct _wifi_config_st {
        char wifi_name[32];
        char wifi_password[32];
    } wifi;
    struct _clock_config_ {
        int8_t hour_offset;
        int8_t minute_offset;
    } clock;
} config;


void setup() {
    Serial.begin(115200);
    EEPROM.begin(512);
    delay(10);
    Serial.println();
    Serial.println();
    Serial.println("Startup");


    // read eeprom for ssid and pass
    Serial.println("Reading EEPROM ssid");
    String esid;
    for (int i = 0; i < 32; ++i)
    {
        esid += char(EEPROM.read(i));
    }
    if(esid[0] == 0 || esid[0] == 0xff)
    {
        Serial.println("EEPROM doesn't contain WiFi connection information.");
        Serial.println("Switch to AP mode immediately.");
        setupAP();
        return;
    }
    Serial.print("SSID: ");
    Serial.println(esid);
    Serial.println("Reading EEPROM pass");
    String epass = "";
    for (int i = 32; i < 96; ++i)
    {
        epass += char(EEPROM.read(i));
    }
    Serial.print("PASS: ");
    Serial.println(epass);  
    if ( esid.length() > 1 ) {
        WiFi.begin(esid.c_str(), epass.c_str());
        if (testWifi()) {

            dnsServer.setTTL(300);
            dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
            dnsServer.start(53, "clock.esp", WiFi.localIP());
            launchWeb(WEB_PAGES_NORMAL);


            //-----------

            Serial.println("Init MAX7219");
            display_init();

            Serial.print("Reading time offset... ");
            int8_t time_offset = int8_t(EEPROM.read(96));
            Serial.printf("Reading time offset... 0x%02x (%d)", time_offset, time_offset);
            if( time_offset > 12 || time_offset < -12 ) {
                time_offset = timeOffset;
                EEPROM.write(96, 0);
                delay(100);
                Serial.print("Set time offset: ");
                Serial.println((int)timeOffset);
                EEPROM.write(96, timeOffset);

                Serial.print("Reading time offset... ");
                time_offset = 0xaa;
                time_offset = int8_t(EEPROM.read(96));
                Serial.println((int)time_offset);
                time_offset = 0;
                
                EEPROM.commit();
            }
            timeOffset = time_offset;
            

            // ledMatrix.setText("MAX7219 Animation Demo");
            // ledMatrix.setNextText("Second text");
            
            //ledMatrix.setTextAlignmentOffset(4*8);
            
            

            timeClient.begin();

            return;
        } 
    }

    setupAP();
}

bool testWifi(void) {
    int c = 0;
    Serial.println("Waiting for Wifi to connect");  
    while ( c < 20 ) {
        if (WiFi.status() == WL_CONNECTED) { return true; } 
        delay(500);
        Serial.print(WiFi.status());    
        c++;
    }
    Serial.println("");
    Serial.println("Connect timed out, opening AP");
    return false;
} 

void launchWeb(int webtype) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("SoftAP IP: ");
    Serial.println(WiFi.softAPIP());
    createWebServer(webtype);
    // Start the server
    server.begin();
    Serial.println("Server started"); 
}

void setupAP(void) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0)
        Serial.println("no networks found");
    else
    {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i)
        {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
            delay(10);
        }
    }
    Serial.println(""); 
    st = "";
    for (int i = 0; i < n; ++i)
    {
        // Print SSID and RSSI for each network found
        st += "<input type='radio' id='";
        st += WiFi.SSID(i);
        st += "' name='ssid' value='";
        st += WiFi.SSID(i);
        st += "'><label for='";
        st += WiFi.SSID(i);
        st += "'>";
        st += WiFi.SSID(i);
        st += " (";
        st += WiFi.RSSI(i);
        st += ")";
        st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
        st += "<label><br>";
    }
    delay(100);

    Serial.print("Setting soft-AP configuration: ");
    Serial.println(WiFi.softAPConfig(apIP, gateway, subnet) ? "Done" : "Failed!");

    Serial.print("Setting soft-AP: ");
    Serial.println(WiFi.softAP(ssid, passphrase) ? "Done" : "Failed!");

    Serial.print("Soft-AP IP address: ");
    Serial.println(WiFi.softAPIP());

    Serial.print("DNS server start ... ");

    dnsServer.setTTL(600);
    dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
    Serial.println(dnsServer.start(53, "www.device-ap.esp", WiFi.softAPIP())? "Ready" : "Failed!");

    // WiFi.softAP(ssid, passphrase, 6);
    // Serial.println("softap");
    launchWeb(WEB_PAGES_FOR_AP);
    Serial.println("over");
}



void createWebServer(int webtype)
{
    if( webtype == WEB_PAGES_FOR_AP ) 
    {
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            IPAddress ip = WiFi.softAPIP();
            String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
            content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
            content += ipStr;
            content += "<form method='get' action='setting'>";
            content += st;
            content += "<input name='pass' type='password' length=64><input type='submit'></form>";
            content += "</html>";
            request->send_P(200, "text/html", content.c_str());  
        });
        server.on("/setting", HTTP_GET, [](AsyncWebServerRequest *request) {
            if(!request->hasParam("ssid")) return;
            if(!request->hasParam("pass")) return;
            // String qsid = server.arg("ssid");
            // String qpass = server.arg("pass");
            AsyncWebParameter* p = request->getParam(0);
            Serial.print(p->name());
            Serial.print(" : ");
            Serial.println(p->value());

            if(p->name() != "ssid") return;
            String qsid = p->value();
            
            p = request->getParam(1);
            Serial.print(p->name());
            Serial.print(" : ");
            Serial.println(p->value());

            if(p->name() != "pass") return;
            String qpass = p->value();

            if (qsid.length() > 0 && qpass.length() > 0) 
            {
                Serial.println("clearing eeprom");
                for (int i = 0; i < 96; ++i) 
                { 
                    EEPROM.write(i, 0);
                }
                Serial.println(qsid);
                Serial.println("");
                Serial.println(qpass);
                Serial.println("");
                  
                Serial.println("writing eeprom ssid:");
                for (unsigned int i = 0; i < qsid.length(); ++i)
                {
                    EEPROM.write(i, qsid[i]);
                    Serial.print("Wrote: ");
                    Serial.println(qsid[i]); 
                }
                Serial.println("writing eeprom pass:"); 
                for (unsigned int i = 0; i < qpass.length(); ++i)
                {
                    EEPROM.write(32+i, qpass[i]);
                    Serial.print("Wrote: ");
                    Serial.println(qpass[i]); 
                }    
                EEPROM.commit();
                content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
                statusCode = 200;
                softreset = true;
            } 
            else 
            {
                content = "{\"Error\":\"404 not found\"}";
                statusCode = 404;
                Serial.println("Sending 404");
            }
            request->send_P(statusCode, "application/json", content.c_str());
        });
    } 
    else if (webtype == WEB_PAGES_NORMAL) 
    {
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            IPAddress ip = WiFi.localIP();
            String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
            // request->send_P(200, "application/json", ("{\"IP\":\"" + ipStr + "\"}").c_str());
            content = "<!DOCTYPE HTML>\r\n<html><body><h1>Hello from ESP8266 at ";
            //content += ipStr;
            content += ip.toString();
            content += "</h1><br><label for='time_offset'>Time offset</label><input type='number' id='time_offset' name='time_offset' value='0' min='-12' max='12' onchange='setOffset(this);'>";
            content += "<br><hr><br><a href='/cleareeprom' class='btn btn-primary'>Clear EEPROM</a>";
            content += "</body>";
            
                                //     if (this.readyState == 4 && this.status == 200) {
                                //         if(this.response == "") {
                                //             document.getElementById("time_offset").removeAttribute("checked");
                                //         }
                                //         else
                                //         {
                                //             if(document.getElementById("toggle_led_input").getAttribute("checked") == null)
                                //             document.getElementById("toggle_led_input").removeAttribute("checked");
                                //         }
                                //     }
            content += "<script>"
                            "function setOffset() {"
                                "var xhttp = new XMLHttpRequest();"
                                "xhttp.onreadystatechange = function() {"
                                "};"
                                "param='?time_offset=' + document.getElementById('time_offset').value;"
                                "xhttp.open('GET', '/time_offset'+param, true);"
                                "xhttp.send();"
                            "}"
                        "</script>";
            content += "</html>";
            request->send_P(200, "text/html", content.c_str());  
        });
        server.on("/time_offset", HTTP_GET, [](AsyncWebServerRequest *request){
            Serial.println("Set time_offset----------------------------------------");

            Serial.println("debug 0");

            int param_n = request->params();
            
            Serial.print("param N=");
            Serial.println(param_n);
            if(param_n == 0) {
                request->send_P(200, "text/plain", "Error. No parameter"); //TODO replace status 200
                return;
            }

            AsyncWebParameter* p = request->getParam(0);
            Serial.print("Param name: ");
            Serial.println(p->name());

            Serial.print("Param value: ");
            Serial.println(p->value());

            long p_value = p->value().toInt();

            Serial.print("Intager value");
            Serial.println(p_value);

            if(p_value < -12 && p_value > 12 ) {
                request->send_P(200, "text/plain", "Error. Wrong parameter value"); //TODO replace status 200
                return;
            }
            Serial.println("----------------------------------------");
            request->send_P(200, "text/plain", "OK");
            timeOffset = (int8_t)p_value;
            // EEPROM.write(96, 0);
            // delay(100);
            Serial.print("Set time offset: ");
            Serial.println((int)timeOffset);
            EEPROM.write(96, timeOffset);
            EEPROM.commit();
        });
        server.on("/cleareeprom", HTTP_GET, [](AsyncWebServerRequest *request) {
            content = "<!DOCTYPE HTML>\r\n<html>";
            content += "<p>Clear settings from EEPROM done.</p><p>The board will automatically restart after 10s... After restart you can connect to AP '";
            content += ssid;
            content += "' and open configuration page by address '";
            content += apIP.toString();
            content += "'.</p></html>";
            request->send_P(200, "text/html", content.c_str());
            Serial.println("clearing eeprom");
            for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
            EEPROM.commit();
            softreset = true;
        });
    }
}

void loop() {
  //server.handleClient();

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
  
    dnsServer.processNextRequest();

    timeClient.update();

    //Serial.println(timeClient.getFormattedTime());

    int8_t hours = timeClient.getHours()+timeOffset;
    if( hours > 23 ) hours -= 24;
    if( hours < 0 ) hours += 24;
    display_printtime( hours, timeClient.getMinutes(), timeClient.getSeconds(), DISPLAY_FORMAT_24H );

    delay(50);
  
}