#include <Arduino.h>

//#define ENABLE_DNS

// #include <ESP8266WiFi.h>
// #include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#ifdef ENABLE_DNS
#include <DNSServer.h>
#endif
#include <Hash.h>
#include "ESPAsyncWebServer.h"

#include <ESP8266HTTPClient.h>

#include "display.h"
#include "config.h"
#include "rtc.h"

#include <NTPClient.h>
#include <WiFiUdp.h>

#define DNS_PORT    53

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
//int8_t timeOffset = 0;


enum _web_pages_en { 
    WEB_PAGES_NORMAL = 0,
    WEB_PAGES_FOR_AP  
};

IPAddress apIP(192,168,4,22);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);

#ifdef ENABLE_DNS
DNSServer dnsServer;
#endif
AsyncWebServer server(80);
const char* ssid = "ESPClock";
const char* passphrase = "1290test$#A";
volatile bool softreset = false;
int statusCode;
String content;
String st;
bool rtc_require_update = false;
bool time_sync_with_ntp = false;

bool testWifi(void);
void launchWeb(int webtype);
void setupAP(void);
void createWebServer(int webtype);





void setup() {
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

    DateTime dt;
    rtc_GetDT( config.clock.hour_offset, config.clock.minute_offset, &dt );
    Serial.printf("RTC time: %02d:%02d:%02d\r\n", dt.hour(), dt.minute(), dt.second());


    if( config.wifi.valid ) {
        WiFi.begin( config.wifi.name, config.wifi.password );
        if (testWifi()) {
// #ifdef ENABLE_DNS
//             dnsServer.setTTL(300);
//             dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
//             dnsServer.start(DNS_PORT, "*", WiFi.localIP());
//             Serial.print("Start DNS server with IP:");
//             Serial.println(WiFi.localIP());
// #endif
            launchWeb(WEB_PAGES_NORMAL);

            Serial.printf("Reading time offset... %d hours %02d minute(s)\r\n", config.clock.hour_offset, config.clock.minute_offset);
            if( config.clock.hour_offset > 12 || config.clock.hour_offset < -12 || config.clock.minute_offset < 0 || config.clock.minute_offset > 59 ) {
                config_settimeoffset(0, 0);
            }
            
            timeClient.begin();
            rtc_require_update = true;
            time_sync_with_ntp = true;
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

bool testWifi(void) {
    int c = 0;
    Serial.print("Network name: ");
    Serial.println(config.wifi.name);
    Serial.print("Password: ");
    Serial.println(config.wifi.password);

    Serial.print("Waiting for Wifi to connect ");  
    while ( c < 20 ) {
        if (WiFi.status() == WL_CONNECTED) { 
            Serial.println(" connected");
            return true; 
        } 
        delay(500);
        Serial.print("."); 
        //Serial.print(WiFi.status());    
        c++;
    }
    Serial.println(" timeout");
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
    WiFi.mode(WIFI_AP);
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

    Serial.printf("Setting soft-AP \"%s\": ", ssid);
    Serial.println(WiFi.softAP(ssid, passphrase) ? "Done" : "Failed!");

    Serial.print("Soft-AP IP address: ");
    Serial.println(WiFi.softAPIP());

    WiFi.config(0, 0, 0);

#ifdef ENABLE_DNS
    Serial.print("DNS server start ... ");

    // dnsServer.setTTL(600);
    // dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
    Serial.println(dnsServer.start(DNS_PORT, "*", WiFi.softAPIP())? "Ready" : "Failed!");
    Serial.print("DNS server started with IP: ");
    Serial.println(WiFi.softAPIP());
#endif

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
            content = "<!DOCTYPE HTML><html><body><h1>EspClock at ";
            content += ipStr;
            content += "</h1><br><label for='hour_offset'>Time offset</label><input type='number' id='hour_offset' name='hour_offset' value='";
            content += ((String)config.clock.hour_offset).c_str();
            content += "' min='-12' max='12' onchange='setOffset(this);'>";
            content += "<label for='minutes_offset'>Time offset</label><input type='number' id='minutes_offset' name='minutes_offset' value='";
            content += ((String)config.clock.minute_offset).c_str();
            content += "' min='0' max='59' onchange='setOffset(this);'>";
            content += "<form method='get' action='setting'>";
            content += st;
            content += "<input name='pass' type='password' length=64><input type='submit'></form>";
                        content += "</body>";
            
            content += "<script>"
                            "function setOffset() {"
                                "var xhttp = new XMLHttpRequest();"
                                "xhttp.onreadystatechange = function() {"
                                "};"
                                "param='?hour_offset=' + document.getElementById('hour_offset').value + '&minutes_offset=' + document.getElementById('minutes_offset').value;"
                                "xhttp.open('GET', '/time_offset'+param, true);"
                                "xhttp.send();"
                            "}"
                        "</script>";
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

            if ( qsid.length() > 0 ) 
            {
                // Serial.println("clearing eeprom");
                // for (int i = 0; i < 96; ++i) 
                // { 
                //     EEPROM.write(i, 0);
                // }
                // Serial.println(qsid);
                // Serial.println("");
                // Serial.println(qpass);
                // Serial.println("");
                  
                // Serial.println("writing eeprom ssid:");
                // for (unsigned int i = 0; i < qsid.length(); ++i)
                // {
                //     EEPROM.write(i, qsid[i]);
                //     Serial.print("Wrote: ");
                //     Serial.println(qsid[i]); 
                // }
                // Serial.println("writing eeprom pass:"); 
                // for (unsigned int i = 0; i < qpass.length(); ++i)
                // {
                //     EEPROM.write(32+i, qpass[i]);
                //     Serial.print("Wrote: ");
                //     Serial.println(qpass[i]); 
                // }    
                // EEPROM.commit();
                config_setwifi(&qsid, &qpass);

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

            if(p->name() != "hour_offset" || p_value < -12 || p_value > 12 ) {
                request->send_P(200, "text/plain", "Error. Wrong parameter [0]"); //TODO replace status 200
                return;
            }
            Serial.println("----------------------------------------");
            int8_t hour_offset = p_value;
            // EEPROM.write(96, 0);
            // delay(100);
            Serial.print("Set hour offset: ");
            Serial.println(hour_offset);

            //------------------------------------

            p = request->getParam(1);
            Serial.print("Param name: ");
            Serial.println(p->name());

            Serial.print("Param value: ");
            Serial.println(p->value());

            p_value = p->value().toInt();

            Serial.print("Intager value");
            Serial.println(p_value);

            if(p->name() != "minutes_offset" || p_value < 0 || p_value > 59 ) {
                request->send_P(200, "text/plain", "Error. Wrong parameter [1]"); //TODO replace status 200
                return;
            }
            Serial.println("----------------------------------------");
            int8_t minutes_offset = p_value;
            // EEPROM.write(96, 0);
            // delay(100);
            Serial.print("Set minutes offset: ");
            Serial.println(minutes_offset);
            config_settimeoffset(hour_offset, minutes_offset);
            request->send_P(200, "text/plain", "OK");
        });
        server.on("/clear_wifi_settings", HTTP_GET, [](AsyncWebServerRequest *request) {
            content = "<!DOCTYPE HTML>\r\n<html>";
            content += "<p>Clear WiFi settings done.</p><p>The board will automatically restart after 10s... After restart you can connect to AP '";
            content += ssid;
            content += "' and open configuration page by address '";
            content += apIP.toString();
            content += "'.</p></html>";
            request->send_P(200, "text/html", content.c_str());
            config_clearwifi();
            softreset = true;
        });
    } 
    else if (webtype == WEB_PAGES_NORMAL) 
    {
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            IPAddress ip = WiFi.localIP();
            String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
            // request->send_P(200, "application/json", ("{\"IP\":\"" + ipStr + "\"}").c_str());
            content = "<!DOCTYPE HTML><html><body><h1>Hello from ESP8266 at ";
            //content += ipStr;
            content += ip.toString();
            content += "</h1><br><label for='hour_offset'>Time offset</label><input type='number' id='hour_offset' name='hour_offset' value='";
            content += ((String)config.clock.hour_offset).c_str();
            content += "' min='-12' max='12' onchange='setOffset(this);'>";
            content += "<label for='minutes_offset'>Time offset</label><input type='number' id='minutes_offset' name='minutes_offset' value='";
            content += ((String)config.clock.minute_offset).c_str();
            content += "' min='0' max='59' onchange='setOffset(this);'>";
            content += "<br><hr><br><a href='/clear_wifi_settings' class='btn btn-primary'>Clear WiFi settings and restart in AP mode</a>";
            content += "</body>";
            
            content += "<script>"
                            "function setOffset() {"
                                "var xhttp = new XMLHttpRequest();"
                                "xhttp.onreadystatechange = function() {"
                                "};"
                                "param='?hour_offset=' + document.getElementById('hour_offset').value + '&minutes_offset=' + document.getElementById('minutes_offset').value;"
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

            if(p->name() != "hour_offset" || p_value < -12 || p_value > 12 ) {
                request->send_P(200, "text/plain", "Error. Wrong parameter [0]"); //TODO replace status 200
                return;
            }
            Serial.println("----------------------------------------");
            int8_t hour_offset = p_value;
            // EEPROM.write(96, 0);
            // delay(100);
            Serial.print("Set hour offset: ");
            Serial.println(hour_offset);

            //------------------------------------

            p = request->getParam(1);
            Serial.print("Param name: ");
            Serial.println(p->name());

            Serial.print("Param value: ");
            Serial.println(p->value());

            p_value = p->value().toInt();

            Serial.print("Intager value");
            Serial.println(p_value);

            if(p->name() != "minutes_offset" || p_value < 0 || p_value > 59 ) {
                request->send_P(200, "text/plain", "Error. Wrong parameter [1]"); //TODO replace status 200
                return;
            }
            Serial.println("----------------------------------------");
            int8_t minutes_offset = p_value;
            // EEPROM.write(96, 0);
            // delay(100);
            Serial.print("Set minutes offset: ");
            Serial.println(minutes_offset);
            config_settimeoffset(hour_offset, minutes_offset);
            request->send_P(200, "text/plain", "OK");
        });
        server.on("/clear_wifi_settings", HTTP_GET, [](AsyncWebServerRequest *request) {
            content = "<!DOCTYPE HTML>\r\n<html>";
            content += "<p>Clear WiFi settings done.</p><p>The board will automatically restart after 10s... After restart you can connect to AP '";
            content += ssid;
            content += "' and open configuration page by address '";
            content += apIP.toString();
            content += "'.</p></html>";
            request->send_P(200, "text/html", content.c_str());
            config_clearwifi();
            softreset = true;
        });
    }
}

// String new_time = "";
// String old_time = "";

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

#ifdef ENABLE_DNS
    dnsServer.processNextRequest();
#endif
    if(time_sync_with_ntp) {
        timeClient.update();

        if( rtc_require_update ) {
            uint32_t epoch_time = timeClient.getEpochTime();
            Serial.printf("Updating RTC with epoch time %u... ", epoch_time);
            rtc_SetEpoch(epoch_time);
            rtc_require_update = false;
            Serial.println("done");
        }
    }

    // new_time = timeClient.getFormattedTime();
    // if( new_time != old_time ) {
    //     Serial.println(new_time);
    //     old_time = new_time;
    // }

    static int cnt = 0;
    
    cnt++;
    if( cnt == 100 ) {
        int8_t hours = 0;
        int8_t minutes = 0;
        int8_t seconds = 0;
        if(time_sync_with_ntp) {
            hours = timeClient.getHours();
            minutes = timeClient.getMinutes() + config.clock.minute_offset;
            seconds = timeClient.getSeconds();
            if( minutes > 59 ) { minutes -= 60; hours++; }
            hours += config.clock.hour_offset;
            if( hours > 23 ) hours -= 24;
            if( hours > 23 ) hours -= 24;
            if( hours < 0 ) hours += 24;
        } else {
            DateTime dt;
            rtc_GetDT(config.clock.hour_offset, config.clock.minute_offset, &dt);
            hours = dt.hour();
            minutes = dt.minute();
            seconds = dt.second();
        }
        display_printtime( hours, minutes, seconds, DISPLAY_FORMAT_24H );
        cnt = 0;
    }

    delay(1);
  
}