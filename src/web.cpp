#include "web.h"
#include "config.h"
#include "rtc.h"
#include "pressure_history.h"

#define OTA

#ifdef OTA
#include <AsyncElegantOTA.h>
#endif
#include <NTPClient.h>
#include <LittleFS.h>

extern volatile bool softreset;
extern unsigned long ntp_time;

IPAddress apIP(192,168,1,1);
IPAddress gateway(192,168,1,9);
IPAddress subnet(255,255,255,0);

#ifdef ENABLE_DNS
AsyncDNSServer dnsServer;
const byte DNS_PORT = 53;
#endif


int statusCode;
String content;
String htmlRadio_NetworksList;

AsyncWebServer server(80);
const char* ssid = "ESPClock";
const char* passphrase = "1290test$#A";

//extern WiFiUDP ntpUDP;
extern NTPClient timeClient;
extern bool time_sync_with_ntp_enabled;
extern bool time_in_sync_with_ntp;

extern float temperature;
extern float pressure;

StateWifi WifiState;

uint8_t ap_list_num = 0;
typedef struct ap_list_st {
    String ssid;
    int32_t rssi;
    uint8_t encryption;
} ap_list_t;

#define AP_LIST_MAX 20

ap_list_t ap_list[AP_LIST_MAX];

void web_init(void) {
    if(!LittleFS.begin()){
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }
    Serial.println("Check filesystem...");
    File file = LittleFS.open("/index-ap.html", "r");
    if(!file){
        Serial.println("Failed to open file \"index-ap.html\" for reading");
        file.close();
        return;
    }
    if(!file.available()){
        Serial.println("Failed to read file \"index-ap.html\"");
        file.close();
        return;
    }
    file.close();
    Serial.println("Filesystem OK");
}

void wifi_processing(void) {
    switch (WifiState)
    {
    case STATE_WIFI_IDLE:
        if( config_wifi.valid_marker == VALID_CONFIG_MARKER )
        {
            Serial.print("Network name: ");
            Serial.println(config_wifi.name);
            Serial.print("Password: ");
            Serial.println(config_wifi.password);
            Serial.print("Waiting for Wifi to connect "); 
            WiFi.begin( config_wifi.name, config_wifi.password );
            WifiState = STATE_WIFI_CONNECTING;
        } else {
            Serial.println("EEPROM doesn't contain WiFi connection information.");
            Serial.println("Switch to AP mode immediately.");
            time_sync_with_ntp_enabled = false;
            time_in_sync_with_ntp = false;
            setupAP();
            WifiState = STATE_WIFI_AP;
        }
        break;
    case STATE_WIFI_CONNECTING:
        if (WiFi.status() == WL_CONNECTED) { 
            Serial.println("WiFi connected");
            WifiState = STATE_WIFI_CONNECTED;
            launchWeb(WEB_PAGES_NORMAL);
            timeClient.begin();
            time_sync_with_ntp_enabled = true; 
        } else {
            static int counter = 0;
            if( counter == 500 ) {
                Serial.print(".");
                counter = 0;
            }
            counter++;
        }
        break;
    case STATE_WIFI_CONNECTED:
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.disconnect();
            WifiState = STATE_WIFI_IDLE;
            time_sync_with_ntp_enabled = false;
            time_in_sync_with_ntp = false;
        } else {
#ifdef ENABLE_MDNS
            MDNS.update();
#endif // ENABLE_MDNS
        }
        break;
    case STATE_WIFI_AP:
        /* code */
        break;
    
    default:
        WifiState = STATE_WIFI_IDLE;
        break;
    }
}

void launchWeb(int webtype) {

    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("SoftAP IP: ");
    Serial.println(WiFi.softAPIP());
    createWebServer(webtype);
    // Start the server
#ifdef OTA
    AsyncElegantOTA.begin(&server);    // Start ElegantOTA
#endif

    server.begin();
    Serial.println("Server started"); 

#ifdef ENABLE_MDNS
    Serial.print("Starting mDNS... ");
    bool mdns_result = MDNS.begin("espclock", WiFi.localIP());
    if(!mdns_result) Serial.println("FAILED !");
    else Serial.println("done");
#endif
}

void setupAP(void) {
    WiFi.mode(WIFI_AP);
    WiFi.disconnect();
    delay(100);
    Serial.println("Scan WiFi network... ");
    int n = WiFi.scanNetworks();
    
    Serial.println("done");
    if (n == 0)
        Serial.println("No networks found");
    else
    {
        uint8_t i;
        Serial.printf("Found %d networks.\r\n", n);
        ap_list_num = 0;
        for (int ap_index = 0; ap_index < n && ap_list_num < AP_LIST_MAX; ap_list_num++, ap_index++)
        {
            // Print SSID and RSSI for each network found
            String ssid = WiFi.SSID(ap_index);
            int32_t rssi = WiFi.RSSI(ap_index);
            uint8_t encryption = WiFi.encryptionType(ap_index);
            
            // remove duplicates
            for(i = 0; i < ap_list_num; i++)
            {
                if( ssid == ap_list[i].ssid )
                {
                    Serial.printf("Skipping duplicated network %s... ", ssid.c_str());
                    if( rssi > ap_list[i].rssi ) {
                        ap_list[i].rssi = rssi;
                    }
                    break;
                }
            }
            if( i < ap_list_num ) {
                Serial.println("network skipped");
                ap_list_num--;
                continue;
            }

            // Sort by rssi
            for( i = ap_list_num; i > 0; i-- ) {
                if( ap_list[i-1].rssi < rssi ) {
                    ap_list[i].ssid = ap_list[i-1].ssid;
                    ap_list[i].rssi = ap_list[i-1].rssi;
                    ap_list[i].encryption = ap_list[i-1].encryption;
                } else {
                    break;
                }
            }
            ap_list[i].ssid = ssid;
            ap_list[i].rssi = rssi;
            ap_list[i].encryption = encryption;

            Serial.print(ap_list_num + 1);
            Serial.print(": ");
            Serial.print(ssid);
            Serial.print(" (");
            Serial.print(rssi);
            Serial.print(")");
            Serial.println((encryption == ENC_TYPE_NONE)?String(" "):String("*"));
        }

        Serial.println();
        Serial.printf("Sorted network list (%d items):\r\n", ap_list_num);
        for( i = 0; i < ap_list_num; i++ )
        {
            String lock="&#128275;";
            String ssid = ap_list[i].ssid;
            int32_t rssi = ap_list[i].rssi;
            String encryption = (ap_list[i].encryption==ENC_TYPE_NONE)?String(" "):lock;
            htmlRadio_NetworksList += "<tr>";
            htmlRadio_NetworksList +=   "<td><input type='radio' id='" + ssid + "' name='ssid' value='" + ssid + "'></td>";
            htmlRadio_NetworksList +=   "<td><label for='" + ssid + "'>" + ssid + "</label></td>";
            htmlRadio_NetworksList +=   "<td><div>" + (String)rssi + "</div></td>";
            htmlRadio_NetworksList +=   "<td><div>" + encryption + "</div></td>";
            htmlRadio_NetworksList += "</tr>";
            Serial.printf("  %d: %-25s %d\r\n", i+1, ssid.c_str(), rssi);
        }
    }
    Serial.println(""); 

    Serial.print("Setting soft-AP configuration: ");
    Serial.println(WiFi.softAPConfig(apIP, gateway, subnet) ? "Done" : "Failed!");

    Serial.printf("Setting soft-AP \"%s\": ", ssid);
    Serial.println(WiFi.softAP(ssid, passphrase) ? "Done" : "Failed!");

    Serial.print("Soft-AP IP address: ");
    Serial.println(WiFi.softAPIP());

    //WiFi.config(0, 0, 0);

#ifdef ENABLE_DNS
    Serial.print("DNS server start ... ");

    // modify TTL associated  with the domain name (in seconds)
    // default is 60 seconds
    dnsServer.setTTL(300);
    // set which return code will be used for all other domains (e.g. sending
    // ServerFailure instead of NonExistentDomain will reduce number of queries
    // sent by clients)
    // default is AsyncDNSReplyCode::NonExistentDomain
    dnsServer.setErrorReplyCode(AsyncDNSReplyCode::ServerFailure);

    // start DNS server for a specific domain name
    //dnsServer.start(DNS_PORT, "*", apIP);

    Serial.println(dnsServer.start(DNS_PORT, "*", apIP)? "Ready" : "Failed!");
    Serial.print("DNS server started with IP: ");
    Serial.println(WiFi.softAPIP());
#endif

    // WiFi.softAP(ssid, passphrase, 6);
    // Serial.println("softap");
    launchWeb(WEB_PAGES_FOR_AP);
    Serial.println("Access Point started.");
}

String processor(const String& var){
    Serial.print("Web processing: ");
    Serial.println(var);
    if (var == "PRESSURE_HISTORY_TABLE") {
        return pressure_html_history;
    }
    else if (var == "AP_LIST") {
        return htmlRadio_NetworksList;
    }
    else if (var == "IP_ADDRESS"){
        IPAddress ip = WiFi.softAPIP();
        return ip.toString();
    }
    else if (var == "LOCAL_IP_ADDRESS"){
        IPAddress ip = WiFi.localIP();
        return ip.toString();
    }
    else if (var == "HOUR_OFFSET"){
        return String(config_clock.hour_offset);
    }
    else if (var == "MINUTE_OFFSET"){
        return String(config_clock.minute_offset);
    }
    else if (var == "TEMPERATURE"){
        return String(temperature);
    }
    else if (var == "PRESSURECOLLECTTIMELEFT"){
        return String(swTimer[SW_TIMER_COLLECT_PRESSURE_HISTORY].GetDowncounter());
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

    if(path.endsWith(".html")) dataType = "text/html";
    else if(path.endsWith(".htm")) dataType = "text/html";
    else if(path.endsWith(".css")) dataType = "text/css";
    else if(path.endsWith(".js")) dataType = "application/javascript";
    else if(path.endsWith(".png")) dataType = "image/png";
    else if(path.endsWith(".gif")) dataType = "image/gif";
    else if(path.endsWith(".jpg")) dataType = "image/jpeg";
    else if(path.endsWith(".ico")) dataType = "image/x-icon";
    // else if(path.endsWith(".xml")) dataType = "text/xml";
    // else if(path.endsWith(".pdf")) dataType = "application/pdf";
    // else if(path.endsWith(".zip")) dataType = "application/zip";
    // else if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
    
    // if (request->hasArg("download")) {
    //     dataType = "application/octet-stream";
    //     download = true;
    // }

    request->send(LittleFS, path, dataType, download, processor);

    return true;
}

void handleWebRequests(AsyncWebServerRequest *request){
    Serial.print("WEB request url: ");
    Serial.println(request->url());
    if( loadFromLittleFS(request) ) return;
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
    Serial.print("WEB message: ");
    Serial.println(message);
    request->send(404, "text/plain", message);
}

bool GetParamValue(AsyncWebServerRequest *request, const char *name, String& value) {
    Serial.print("GetParaValue: ");
    Serial.println(name);

    if(request->hasParam(name)) {
        Serial.print("Get parameter value: ");
        value = request->getParam(name)->value();
        Serial.println(value);
        return true;
    }
    Serial.println("Parameter not found");
    return false;
}

void AddServerFastTelemetry() {
    server.on("/getFastTelemetry", HTTP_GET, [](AsyncWebServerRequest *request){
        int8_t hours = 0;
        int8_t minutes = 0;
        int8_t seconds = 0;
        if (time_in_sync_with_ntp)
        {
            unsigned long time =
                (unsigned long)ntp_time + 
                (unsigned long)rtc_SecondsSinceUpdate;
            hours = (time / (60 * 60)) % 24;
            minutes = (time / 60) % 60;
            seconds = time % 60;
            //Serial.printf("Time from NTP: %lu => %d:%d (%lu)\r\n", ntp_time, hours, minutes, time);
        }
        else
        {
            DateTime dt = rtc_dt +
                            TimeSpan(rtc_SecondsSinceUpdate / (60*60*24),
                                    (rtc_SecondsSinceUpdate / (60*60)) % 24,
                                    (rtc_SecondsSinceUpdate / 60) % 60,
                                    rtc_SecondsSinceUpdate % 60);
            hours = dt.hour();
            minutes = dt.minute();
            seconds = dt.second();
            //Serial.printf("Time from RTC: %d:%d\r\n", hours, minutes);
        }
        String response_str = String();

        // Hours
        response_str = "{ \"Hours\":\"";
        response_str +=  hours;
        response_str += "\",";
        // Minutes
        response_str += "\"Minutes\":\"";
        response_str += minutes;
        response_str += "\",";
        // Seconds
        response_str += "\"Seconds\":\"";
        response_str += seconds;
        response_str += "\",";
        // Hous offset
        response_str += "\"HourOffset\":\"";
        response_str +=  config_clock.hour_offset;
        response_str += "\",";
        // Minute offset
        response_str += "\"MinuteOffset\":\"";
        response_str += config_clock.minute_offset;
        response_str += "\",";

        // Seconds until next pressure connection
        response_str += "\"SecondsUntilPressureCollection\":\"";
        response_str += swTimer[SW_TIMER_COLLECT_PRESSURE_HISTORY].GetDowncounter();
        response_str += "\",";
        // Pressure
        response_str += "\"Pressure\":\"";
        response_str += pressure;
        response_str += "\",";
        // Temperature
        response_str += "\"Temperature\":\"";
        response_str += temperature;
        response_str += "\"}";

        request->send_P(200, "text/plain", response_str.c_str());
    });
}

void createWebServer(int webtype)
{
    if( webtype == WEB_PAGES_FOR_AP ) 
    {
        Serial.print("Creating AP server ...");
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {

            Serial.println("Accessing root page...");
            request->send(LittleFS, "/index-ap.html", "text/html", false, processor);
        });

        Serial.print(".");
        server.onNotFound([](AsyncWebServerRequest *request) {
            Serial.println("Handle not found page...");
            handleWebRequests(request);
        }); // Set server all paths are not found so we can handle as per URI 

        Serial.print(".");
        AddServerFastTelemetry();

        server.on("/getPressureHistory", HTTP_GET, [](AsyncWebServerRequest *request){
            // if( !request->authenticate(http_username,http_password) )
            //     return request->requestAuthentication();
            char pressure_str[10];
            sprintf(pressure_str, "%3.1f", pressure);
            request->send_P(200, "text/plain", pressure_str);
        });

        server.on("/set_time_offset", HTTP_GET, [](AsyncWebServerRequest *request){
            Serial.println("Set time_offset----------------------------------------");
            int param_n = request->params();
            
            Serial.print("param N=");
            Serial.println(param_n);
            if(param_n == 0) {
                request->send_P(400, "text/plain", "Error. No parameter");
                return;
            }

            AsyncWebParameter* p = request->getParam(0);
            Serial.print("Param name: ");
            Serial.println(p->name());

            Serial.print("Param value: ");
            Serial.println(p->value());

            long p_value = p->value().toInt();

            Serial.print("Intager value: ");
            Serial.println(p_value);

            if(p->name() != "hour_offset" || p_value < -12 || p_value > 12 ) {
                request->send_P(400, "text/plain", "Error. Wrong or missing parameter.");
                return;
            }
            Serial.println("----------------------------------------");
            int8_t hour_offset = p_value;
            Serial.print("Set hour offset: ");
            Serial.println(hour_offset);

            //------------------------------------

            p = request->getParam(1);
            Serial.print("Param name: ");
            Serial.println(p->name());

            Serial.print("Param value: ");
            Serial.println(p->value());

            p_value = p->value().toInt();

            Serial.print("Intager value: ");
            Serial.println(p_value);

            if(p->name() != "minute_offset" || p_value < 0 || p_value > 59 ) {
                request->send_P(400, "text/plain", "Error. Wrong or missing parameter.");
                return;
            }
            Serial.println("----------------------------------------");
            int8_t minute_offset = p_value;
            Serial.print("Set minutes offset: ");
            Serial.println(minute_offset);
            config_SetTimeSettings(hour_offset, minute_offset);
            request->send_P(200, "text/plain", "OK");
        });

        server.on("/setting", HTTP_GET, [](AsyncWebServerRequest *request) {
            Serial.println("Store network params...");
            String param1, param2;
            bool param_saved = false;
            if( GetParamValue(request,"ssid",param1) && GetParamValue(request,"pass",param2) ) {
                if( param1.length() > 0 ) 
                {
                    Serial.printf(
                        "Save WiFi network settings: SSID:%s, Pass:%s\r\n", 
                        param1.c_str(),
                        param2.c_str() );
                    config_setWiFiSettings(param1, param2);
                    param_saved = true;
                }
            }
            if( GetParamValue(request,"auth-username",param1) && GetParamValue(request,"auth-pass",param2) ) {
                if( param1.length() > 0 && param2.length() > 0 ) 
                {
                    Serial.printf(
                        "Save Auth settings: username:%s, pass:%s\r\n", 
                        param1.c_str(),
                        param2.c_str() );
                    config_setAuthSettings(param1, param2);
                    param_saved = true;
                }
            }
            if( param_saved ){
                statusCode = 200;
            } else {
                statusCode = 500;
            }
            request->send_P(statusCode, "application/json", content.c_str());
        });
        server.on("/reset_network", HTTP_GET, [](AsyncWebServerRequest *request) {
            content = "<!DOCTYPE HTML>\r\n<html>";
            content += "<p>Clear WiFi settings done.</p><p>The board will automatically restart after 10s... After restart you can connect to AP '";
            content += ssid;
            content += "' and open configuration page by address '";
            content += apIP.toString();
            content += "'.</p></html>";
            request->send_P(200, "text/html", content.c_str());
            config_setWiFiSettings("","");
            config_setAuthSettings("","");
            softreset = true;
        });
    } 
    else if (webtype == WEB_PAGES_NORMAL) 
    {
        Serial.print("Creating Device server ...");
        
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {

            Serial.println("Accessing root page...");
            //request->send(LittleFS, "/index-ap.html", "text/html");
            request->send(LittleFS, "/index-dev.html", "text/html", false, processor);
        });
        
        Serial.print(".");
        server.onNotFound([](AsyncWebServerRequest *request) {
            //request->send_P(404,"text/plain", "");
            Serial.println("Handle not found page...");
            handleWebRequests(request);
        }); // Set server all paths are not found so we can handle as per URI 

        Serial.print(".");
        AddServerFastTelemetry();
       
        Serial.print(".");
        server.on("/getPressureHistory", HTTP_GET, [](AsyncWebServerRequest *request){
            // if( !request->authenticate(http_username,http_password) )
            //     return request->requestAuthentication();
            request->send_P( 200, "text/plain", pressure_json_history.c_str() );
        });
        
        Serial.print(".");
        server.on("/set_time_offset", HTTP_GET, [](AsyncWebServerRequest *request){
            String param1, param2;
            Serial.print("URL: ");
            Serial.println(request->url());
            if( GetParamValue(request, "hour_offset", param1) && GetParamValue(request, "minute_offset",param2)){
                Serial.printf("Set time offset: %02ld:%02ld\r\n",param1.toInt(), param2.toInt());
                if( !config_SetTimeSettings(param1.toInt(), param2.toInt()) ){
                    Serial.println("Time offset parameters are invalid");
                    request->send_P(400, "text/plain", "Error. Wrong or missing parameter.");
                    return;
                }
            } else {
                Serial.println("Parse time offset parameters error");
                request->send_P(400, "text/plain", "Error. Wrong or missing parameter.");
                return;
            }
            Serial.println("Time offset set successfully");
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
            config_setWiFiSettings("","");
            softreset = true;
        });
    }

    Serial.println(" done");
}