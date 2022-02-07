#include "web.h"
#include "config.h"
#include "rtc.h"
#include <NTPClient.h>
#include <AsyncElegantOTA.h>

#include <ESP8266mDNS.h>        // Include the mDNS library
#include <LittleFS.h>

extern String dbg_text;

const byte DNS_PORT = 53;
extern volatile bool softreset;

IPAddress apIP(192,168,1,1);
IPAddress gateway(192,168,1,9);
IPAddress subnet(255,255,255,0);


int statusCode;
String content;
String htmlRadio_NetworksList;
String html_PressureHistory;
String pressureLabelsStr;
String pressureValuesStr;
extern float pressure;

AsyncWebServer server(80);
const char* ssid = "ESPClock";
const char* passphrase = "1290test$#A";

//extern WiFiUDP ntpUDP;
extern NTPClient timeClient;
extern bool time_sync_with_ntp_enabled;

StateWifi WifiState;

char http_username[20] = "admin";
char http_password[20] = "ruivon";

void web_init(void) {
    if(!LittleFS.begin()){
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }
}

void wifi_processing(void) {
    switch (WifiState)
    {
    case STATE_WIFI_IDLE:
        if( !swTimerIsTriggered(SW_TIMER_WIFI_CONNECTING) ) {
            if( config.wifi.valid ) {
                Serial.print("Network name: ");
                Serial.println(config.wifi.name);
                Serial.print("Password: ");
                Serial.println(config.wifi.password);
                Serial.print("Waiting for Wifi to connect ");
                WiFi.hostname("espclock"); 
                WiFi.begin( config.wifi.name, config.wifi.password );
                WifiState = STATE_WIFI_CONNECTING;
                swTimerStart(SW_TIMER_WIFI_CONNECTING);
            } else {
                Serial.println("EEPROM doesn't contain WiFi connection information.");
                Serial.println("Switch to Access Point AP mode immediately.");
                time_sync_with_ntp_enabled = false;
                setupAP();
                WifiState = STATE_WIFI_AP;
            }
        }
        break;
    case STATE_WIFI_CONNECTING:
        if (WiFi.status() == WL_CONNECTED) { 
            Serial.println("WiFi connected");
            if (!MDNS.begin("espclock")) {             // Start the mDNS responder for espclock.local
                Serial.println("Error setting up MDNS responder!");
            }
            Serial.println("mDNS responder started");
            swTimerStop(SW_TIMER_WIFI_CONNECTING);
            WifiState = STATE_WIFI_CONNECTED;
            launchWeb(WEB_PAGES_NORMAL);
            timeClient.begin();
            time_sync_with_ntp_enabled = true; 
        } else {
            //Serial.print(".");
            if( swTimerIsTriggered(SW_TIMER_WIFI_CONNECTING) ) {
                Serial.println("WiFi connection wasn't established");
                Serial.println("Switch to Access Point (AP) mode.");
                time_sync_with_ntp_enabled = false;
                setupAP();
                swTimerStop(SW_TIMER_WIFI_CONNECTING);
                WifiState = STATE_WIFI_IDLE;
            }  
        }
        break;
    case STATE_WIFI_CONNECTED:
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.disconnect();
            WifiState = STATE_WIFI_IDLE;
        }
        /* code */
        break;
    case STATE_WIFI_AP:
        /* code */
        break;
    
    default:
        WifiState = STATE_WIFI_IDLE;
        break;
    }
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
    Serial.println("WiFi connection parameters:");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("SoftAP IP: ");
    Serial.println(WiFi.softAPIP());
    createWebServer(webtype);
    // Start the server
    Serial.println("Start OTA server");
    Serial.println("Start web server");
    server.begin();
    AsyncElegantOTA.begin(&server);    // Start ElegantOTA
    Serial.println("Server started"); 
}

typedef struct ap_desc_st {
    String ssid;
    int32_t rssi;
    uint8_t encryption;
} ap_desc_t;

#define AP_LIST_MAX     10
uint8_t ap_list_num = 0;
ap_desc_t ap_list[AP_LIST_MAX];

void setupAP(void) {
    WiFi.mode(WIFI_AP);
    WiFi.disconnect();
    delay(100);
    ap_list_num = 0;
    int ap_list_num = WiFi.scanNetworks();
    Serial.println("scan done");
    if (ap_list_num == 0)
        Serial.println("no networks found");
    else
    {
        Serial.print(ap_list_num);
        Serial.println(" networks found");
        for (int i = 0; i < ap_list_num; ++i)
        {
            // Print SSID and RSSI for each network found
            ap_list[i].ssid = WiFi.SSID(i);
            ap_list[i].rssi = WiFi.RSSI(i);
            ap_list[i].encryption = WiFi.encryptionType(i);

            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(ap_list[i].ssid);
            Serial.print(" (");
            Serial.print(ap_list[i].rssi);
            Serial.print(")");
            Serial.println((ap_list[i].encryption == ENC_TYPE_NONE)?String(" "):String("*"));
            //delay(10);
        }
    }
    Serial.println(""); 

    Serial.print("Setting soft-AP configuration: ");
    Serial.println(WiFi.softAPConfig(apIP, gateway, subnet) ? "Done" : "Failed!");

    Serial.printf("Setting soft-AP \"%s\": ", ssid);
    Serial.println(WiFi.softAP(ssid, passphrase) ? "Done" : "Failed!");

    Serial.print("Soft-AP IP address: ");
    Serial.println(WiFi.softAPIP());

    WiFi.config(0, 0, 0);

    if (!MDNS.begin("espclock")) {             // Start the mDNS responder for esp8266.local
        Serial.println("Error setting up MDNS responder!");
    }
    Serial.println("mDNS responder started");
    // WiFi.softAP(ssid, passphrase, 6);
    // Serial.println("softap");
    launchWeb(WEB_PAGES_FOR_AP);
    Serial.println("over");
}



extern float temperature;
extern float pressure;

String processor(const String& var){
    Serial.print("processor: ");
    Serial.println(var);
    if (var == "AP_LIST") {
        String APList = "<div>";
        if(ap_list_num == 0) {
            APList += "<p>No available network</p>";
        } else {
            for(int i = 0; i < ap_list_num; ++i)
            {
                APList += "<div>";
                APList +=   "<input type='radio' id='" + ap_list[i].ssid + "' name='ssid' value='" + ap_list[i].ssid + "'>";
                APList +=   "<label for='" + ap_list[i].ssid + "'>";
                APList +=       ap_list[i].ssid + " (" + ap_list[i].rssi + ")" + (ap_list[i].encryption == ENC_TYPE_NONE)?String(""):String("*");
                APList +=   "</label>";
                APList += "</div>";
            }
        }
        APList += "</div>";
        return APList;
    }
    else if (var == "IP_ADDRESS"){
        IPAddress ip = WiFi.softAPIP();
        return ip.toString();
    }
    else if (var == "HOURS_OFFSET"){
        return String(config.clock.hour_offset);
    }
    else if (var == "MINUTES_OFFSET"){
        return String(config.clock.minute_offset);
    }
    else if (var == "TEMPERATURE"){
        return String(temperature);
    }
    else if (var == "PRESSURE"){
        return String(pressure);
    }  
    return String("");
}

bool loadFromLittleFS(AsyncWebServerRequest *request){
    String path = request->url();
    String dataType = "text/plain";
    bool download = false;
    if(path.endsWith("/")) path += "index.htm";

    if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
    else if(path.endsWith(".html")) dataType = "text/html";
    else if(path.endsWith(".htm")) dataType = "text/html";
    else if(path.endsWith(".css")) dataType = "text/css";
    else if(path.endsWith(".js")) dataType = "application/javascript";
    else if(path.endsWith(".png")) dataType = "image/png";
    else if(path.endsWith(".gif")) dataType = "image/gif";
    else if(path.endsWith(".jpg")) dataType = "image/jpeg";
    else if(path.endsWith(".ico")) dataType = "image/x-icon";
    else if(path.endsWith(".xml")) dataType = "text/xml";
    else if(path.endsWith(".pdf")) dataType = "application/pdf";
    else if(path.endsWith(".zip")) dataType = "application/zip";
    //File dataFile = LittleFS.open(path.c_str(), "r");
    if (request->hasArg("download")) {
        dataType = "application/octet-stream";
        download = true;
    }

    request->send(LittleFS, path, dataType, download, processor);

    return true;
}

void handleWebRequests(AsyncWebServerRequest *request){
    if(loadFromLittleFS(request)) return;
    String message = "File Not Detected\n\n";
    message += "URI: ";
    message += request->url();
    message += "\nMethod: ";
    message += (request->method() == HTTP_GET)?"GET":"POST";
    message += "\nArguments: ";
    message += request->args();
    message += "\n";
    for (uint8_t i=0; i<request->args(); i++){
        message += " NAME:"+request->argName(i) + "\n VALUE:" + request->arg(i) + "\n";
    }
    request->send(404, "text/plain", message);
    Serial.println(message);
}

void clear_log(void);

void createWebServer(int webtype)
{
    if( webtype == WEB_PAGES_FOR_AP ) 
    {
        Serial.print("Creating AP server ...");
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(LittleFS, "/index-ap.html", "text/html", false, processor);
            //request->send_P(200, "text/html", content.c_str());  
        });
        Serial.print(".");
        server.on("/setting", HTTP_GET, [](AsyncWebServerRequest *request) {
            if(!request->hasParam("ssid")) return;
            if(!request->hasParam("pass")) return;
            if(!request->hasParam("auth-username")) return;
            if(!request->hasParam("auth-password")) return;
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
            
            p = request->getParam(2);
            Serial.print(p->name());
            Serial.print(" : ");
            Serial.println(p->value());

            if(p->name() != "auth-username") return;
            String auth_username = p->value();
            
            p = request->getParam(3);
            Serial.print(p->name());
            Serial.print(" : ");
            Serial.println(p->value());

            if(p->name() != "auth-password") return;
            String auth_password = p->value();

            if ( qsid.length() > 0 ) 
            {
                config_set(&qsid, &qpass, &auth_username, &auth_password);
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
        Serial.print(".");
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
        Serial.print(".");
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
        Serial.print(".");
        server.on("/clear_logs", HTTP_GET, [](AsyncWebServerRequest *request) {
                clear_log();
                content = 
                    "<html>"
                        "<head>"
                            "<meta http-equiv='refresh' content='3;url=espclock.local' />"
                        "</head>"
                        "<body>"
                            "<h1>EspClock</h1>"
                            "<p>Log cleaned. Redirecting in 3 seconds...</p>"
                        "</body>"
                    "</html>";
                statusCode = 200;
            //request->send_P(statusCode, "application/json", content.c_str());

            request->send_P(200, "text/html", content.c_str());
        });
    } 
    else if (webtype == WEB_PAGES_NORMAL) 
    {
        Serial.print("Creating Device server ...");
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            if( !request->authenticate(http_username,http_password) )
                return request->requestAuthentication();
            request->send(LittleFS, "/index-dev.html", "text/html", false, processor);
        });
        Serial.print(".");
        server.onNotFound([](AsyncWebServerRequest *request) {
            handleWebRequests(request);
        }); // Set server all paths are not found so we can handle as per URI

        Serial.print(".");
        server.on("/readPressure", HTTP_GET, [](AsyncWebServerRequest *request){
            if( !request->authenticate(http_username,http_password) )
                return request->requestAuthentication();
            char pressure_str[10];
            sprintf(pressure_str, "%3.1f", pressure);
            request->send_P(200, "text/plain", pressure_str);
        });
        Serial.print(".");
        server.on("/time_offset", HTTP_GET, [](AsyncWebServerRequest *request){
            if( !request->authenticate(http_username,http_password) )
                return request->requestAuthentication();
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
        Serial.print(".");
        server.on("/clear_wifi_settings", HTTP_GET, [](AsyncWebServerRequest *request) {
            if( !request->authenticate(http_username,http_password) )
                return request->requestAuthentication();
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
    Serial.println(" done");
}