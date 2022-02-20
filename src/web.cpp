#include "web.h"
#include "config.h"
#include "rtc.h"
#include "pressure_history.h"

#include <NTPClient.h>
#include <AsyncElegantOTA.h>
#include <LittleFS.h>

const byte DNS_PORT = 53;
extern volatile bool softreset;

IPAddress apIP(192,168,1,1);
IPAddress gateway(192,168,1,9);
IPAddress subnet(255,255,255,0);

#ifdef ENABLE_DNS
AsyncDNSServer dnsServer;
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
            //delay(1);
            // TODO: if timeout then setup AP
            // if()
            // {
            //     Serial.println("WiFi connection wasn't established. Switch to AP.");
            //     time_sync_with_ntp_enabled = false;
            //     setupAP();
            // }

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

void launchWeb(int webtype) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("SoftAP IP: ");
    Serial.println(WiFi.softAPIP());
    createWebServer(webtype);
    // Start the server
    AsyncElegantOTA.begin(&server);    // Start ElegantOTA

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
        uint8_t i;
        Serial.print(n);
        Serial.println(" networks found");
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
    Serial.println("over");
}

String processor(const String& var){
    Serial.print("processor: ");
    Serial.println(var);
    if (var == "PRESSURE_HISTORY_TABLE") {
        return html_PressureHistory;
    }
    else if (var == "AP_LIST") {
        return htmlRadio_NetworksList;
    }
    else if (var == "IP_ADDRESS"){
        IPAddress ip = WiFi.softAPIP();
        return ip.toString();
    }
    else if (var == "HOURS_OFFSET"){
        return String(config_clock.hour_offset);
    }
    else if (var == "MINUTES_OFFSET"){
        return String(config_clock.minute_offset);
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
    Serial.print("WEB message: ");
    Serial.println(message);
    request->send(404, "text/plain", message);
}

void createWebServer(int webtype)
{
    if( webtype == WEB_PAGES_FOR_AP ) 
    {
        Serial.print("Creating AP server ...");
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {

            Serial.println("Accessing root page...");
            //request->send(LittleFS, "/index-ap.html", "text/html");
            request->send(LittleFS, "/index-ap.html", "text/html", false, processor);
        });
        Serial.print(".");
        server.onNotFound([](AsyncWebServerRequest *request) {
            //request->send_P(404,"text/plain", "");
            Serial.println("Handle not found page...");
            handleWebRequests(request);
        }); // Set server all paths are not found so we can handle as per URI 

        server.on("/getTime", HTTP_GET, [](AsyncWebServerRequest *request){
            // if( !request->authenticate(http_username,http_password) )
            //     return request->requestAuthentication();
            char time_s[15];
            sprintf(time_s, "%lld", (long long)rtc_dt.secondstime()+RTC_SECONDS_2000_01_01 + rtc_SecondsSinceUpdate);
            // Serial.printf("Web request getTime: %s\r\n", time_s);
            request->send_P(200, "text/plain", time_s);
        });

        server.on("/getTimeOffset", HTTP_GET, [](AsyncWebServerRequest *request){
            // if( !request->authenticate(http_username,http_password) )
            //     return request->requestAuthentication();
            char offset_min[10];
            sprintf(offset_min, "%d", config_clock.hour_offset*60+config_clock.minute_offset);
            request->send_P(200, "text/plain", offset_min);
        });

        server.on("/getPressure", HTTP_GET, [](AsyncWebServerRequest *request){
            // if( !request->authenticate(http_username,http_password) )
            //     return request->requestAuthentication();
            char pressure_str[10];
            sprintf(pressure_str, "%3.1f", pressure);
            request->send_P(200, "text/plain", pressure_str);
        });

        server.on("/getTemperature", HTTP_GET, [](AsyncWebServerRequest *request){
            // if( !request->authenticate(http_username,http_password) )
            //     return request->requestAuthentication();
            char temperature_str[10];
            sprintf(temperature_str, "%3.1f", temperature);
            request->send_P(200, "text/plain", temperature_str);
        });

        server.on("/set_time_offset", HTTP_GET, [](AsyncWebServerRequest *request){
            Serial.println("Set time_offset----------------------------------------");

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

            Serial.print("Intager value: ");
            Serial.println(p_value);

            if(p->name() != "hour_offset" || p_value < -12 || p_value > 12 ) {
                request->send_P(200, "text/plain", "Error. Wrong parameter [0]"); //TODO replace status 200
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

            if(p->name() != "minutes_offset" || p_value < 0 || p_value > 59 ) {
                request->send_P(200, "text/plain", "Error. Wrong parameter [1]"); //TODO replace status 200
                return;
            }
            Serial.println("----------------------------------------");
            int8_t minutes_offset = p_value;
            Serial.print("Set minutes offset: ");
            Serial.println(minutes_offset);
            config_settimeoffset(hour_offset, minutes_offset);
            request->send_P(200, "text/plain", "OK");
        });

//         server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
//             IPAddress ip = WiFi.softAPIP();
//             content = 
// "<!DOCTYPE HTML>"
// "   <html>"
// "       <head>"
// "			<style>"
// "				input[type='radio'] { margin: 0 10px 0 0; }"
// "			</style>"
// "           <meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>"
// "           <link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css' integrity='sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm' crossorigin='anonymous'>"
// "       </head>"
// "       <body>"
// "           <div style='font-size:3em; padding:30px;'>"
// "               <h1>ESPClock</h1>"
// "               <p>IP address: " + ip.toString() + "</p>"
// "               <hr>" +
//                 json_PressureHistory +
// "               <hr>"
// "               <p>"
// "                   <label for='hour_offset'>Hours offset</label>"
// "                   <input type='number' id='hour_offset' name='hour_offset' value='" + ((String)config.clock.hour_offset) + "' min='-12' max='12' onchange='setOffset(this);'>"
// "               </p>"
// "               <p>"
// "                   <label for='minutes_offset'>Minutes offset</label>"
// "                   <input type='number' id='minutes_offset' name='minutes_offset' value='" + ((String)config.clock.minute_offset) + "' min='0' max='59' onchange='setOffset(this);'>"
// "               </p>"
// "               <form method='get' action='setting'>"
//                     "<hr>"
// "                   <p style='margin:0 0 5px 0'>Choose a network to connect</p>" +
//                     htmlRadio_NetworksList +
//                     "<br>"
//                     "<p>"
// "                       <label for='pass'>Password</label>"
// "                       <input name='pass' type='password' length=64>"
//                     "</p>"
// "                   <hr>"
// "                   <button type='submit' class='btn btn-primary'>Save WiFi settings</button>"
// "               </form>"
// "           </div>"
// "       </body>"
// "       <script src='https://code.jquery.com/jquery-3.2.1.slim.min.js' integrity='sha384-KJ3o2DKtIkvYIK3UENzmM7KCkRr/rE9/Qpg6aAZGJwFDMVNA/GpGFF93hXpG5KkN' crossorigin='anonymous'></script>"
// "       <script src='https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.12.9/umd/popper.min.js' integrity='sha384-ApNbgh9B+Y1QKtv3Rn7W3mgPxhU9K/ScQsAP7hUibX39j7fakFPskvXusvfa0b4Q' crossorigin='anonymous'></script>"
// "       <script src='https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/js/bootstrap.min.js' integrity='sha384-JZR6Spejh4U02d8jOt6vLEHfe/JQGiRRSQQxSfFWpi1MquVdAyjUar5+76PVCmYl' crossorigin='anonymous'></script>"
// "       <script>"
// "           function setOffset() {"
//                 "var xhttp = new XMLHttpRequest();"
//                 "xhttp.onreadystatechange = function() {"
//                 "};"
//                 "param='?hour_offset=' + document.getElementById('hour_offset').value + '&minutes_offset=' + document.getElementById('minutes_offset').value;"
//                 "xhttp.open('GET', '/time_offset'+param, true);"
//                 "xhttp.send();"
// "           }"
// "       </script>"
// "   </html>";

//             request->send_P(200, "text/html", content.c_str());  
//         });
        server.on("/setting", HTTP_GET, [](AsyncWebServerRequest *request) {
            Serial.println("Store network params...");
            AsyncWebParameter *p;
            if(!request->hasParam("ssid")) return;
            if(!request->hasParam("pass")) return;
            if(!request->hasParam("auth-username")) return;
            if(!request->hasParam("auth-pass")) return;
            
            // wifi ssid
            Serial.println("Get wifi ssid...");
            p = request->getParam(0);
            Serial.print(p->name());
            Serial.print(" : ");
            Serial.println(p->value());

            if(p->name() != "ssid") return;
            String qsid = p->value();
            
            // wifi password
            Serial.println("Get wifi password");
            p = request->getParam(1);
            Serial.print(p->name());
            Serial.print(" : ");
            Serial.println(p->value());

            if(p->name() != "pass") return;
            String qpass = p->value();
            
            // auth username
            Serial.println("Get auth username");
            p = request->getParam(2);
            Serial.print(p->name());
            Serial.print(" : ");
            Serial.println(p->value());

            if(p->name() != "auth-username") return;
            String auth_username = p->value();
            
            // auth password
            Serial.println("Get auth password");
            p = request->getParam(3);
            Serial.print(p->name());
            Serial.print(" : ");
            Serial.println(p->value());

            if(p->name() != "auth-pass") return;
            String auth_pass = p->value();


            Serial.println("Validate wifi ssid");
            if ( qsid.length() > 0 ) 
            {
                Serial.printf("Save network settings: \"%s\", \"%s\", \"%s\", \"%s\"\r\n", 
                    qsid.c_str(),
                    qpass.c_str(),
                    auth_username.c_str(),
                    auth_pass.c_str());
                config_setNetSettings(&qsid, &qpass, &auth_username, &auth_pass);
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
//         server.on("/time_offset", HTTP_GET, [](AsyncWebServerRequest *request){
//             Serial.println("Set time_offset----------------------------------------");

//             Serial.println("debug 0");

//             int param_n = request->params();
            
//             Serial.print("param N=");
//             Serial.println(param_n);
//             if(param_n == 0) {
//                 request->send_P(200, "text/plain", "Error. No parameter"); //TODO replace status 200
//                 return;
//             }

//             AsyncWebParameter* p = request->getParam(0);
//             Serial.print("Param name: ");
//             Serial.println(p->name());

//             Serial.print("Param value: ");
//             Serial.println(p->value());

//             long p_value = p->value().toInt();

//             Serial.print("Intager value");
//             Serial.println(p_value);

//             if(p->name() != "hour_offset" || p_value < -12 || p_value > 12 ) {
//                 request->send_P(200, "text/plain", "Error. Wrong parameter [0]"); //TODO replace status 200
//                 return;
//             }
//             Serial.println("----------------------------------------");
//             int8_t hour_offset = p_value;
//             // EEPROM.write(96, 0);
//             // delay(100);
//             Serial.print("Set hour offset: ");
//             Serial.println(hour_offset);

//             //------------------------------------

//             p = request->getParam(1);
//             Serial.print("Param name: ");
//             Serial.println(p->name());

//             Serial.print("Param value: ");
//             Serial.println(p->value());

//             p_value = p->value().toInt();

//             Serial.print("Intager value");
//             Serial.println(p_value);

//             if(p->name() != "minutes_offset" || p_value < 0 || p_value > 59 ) {
//                 request->send_P(200, "text/plain", "Error. Wrong parameter [1]"); //TODO replace status 200
//                 return;
//             }
//             Serial.println("----------------------------------------");
//             int8_t minutes_offset = p_value;
//             // EEPROM.write(96, 0);
//             // delay(100);
//             Serial.print("Set minutes offset: ");
//             Serial.println(minutes_offset);
//             config_settimeoffset(hour_offset, minutes_offset);
//             request->send_P(200, "text/plain", "OK");
//         });
        server.on("/reset_network", HTTP_GET, [](AsyncWebServerRequest *request) {
            content = "<!DOCTYPE HTML>\r\n<html>";
            content += "<p>Clear WiFi settings done.</p><p>The board will automatically restart after 10s... After restart you can connect to AP '";
            content += ssid;
            content += "' and open configuration page by address '";
            content += apIP.toString();
            content += "'.</p></html>";
            request->send_P(200, "text/html", content.c_str());
            config_resetNetSettings();
            softreset = true;
        });
    } 
    else if (webtype == WEB_PAGES_NORMAL) 
    {
        Serial.print("Creating Device server ...");
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            IPAddress ip = WiFi.localIP();
            String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
            String updateDateTime = (String)rtc_dt.year() + "/" + 
                                    (String)rtc_dt.month() + "/" + 
                                    (String)rtc_dt.day() + " " +
                                    (String)rtc_dt.hour() + ":" + 
                                    (String)rtc_dt.minute()+ ":" + 
                                    (String)rtc_dt.second() ;
        // request->send_P(200, "application/json", ("{\"IP\":\"" + ipStr + "\"}").c_str());
            content = 
                "<!DOCTYPE HTML>"
                "<html>"
                    "<script src='https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.5.0/Chart.min.js'></script>"
                    "<style>"
                        "body { padding: 8px 16px; font-family: system-ui; }"
                        "h1 { text-align: center; }"
                        "h3 {"
                            "background-color: #555555;"
                            "color: white;"
                            "padding:  8px;"
                            "text-align:  center;"
                        "}"
                        "canvas {"
                            "display:block;"
                        "}"
                        "p.time_offset_section>label {"
                            "display: inline-block;"
                            "min-width: 70px;"
                        "}"
                        "p.time_offset_section>input {"
                            "display: inline-block;"
                            "min-width: 50px;"
                        "}"
                        "button {"
                            "padding: 8px;"
                        "}"
                    "</style>"
                    "<body>"
                        "<h1>ESPClock</h1>"
                        "<p>IP address: ";
            content += ip.toString();
            content += "</p>"
                        "<h3>Pressure history</h3>"
                        "<p>"
                            "<canvas id='pressureChart'></canvas>"
                        "</p>"
                        "<h3>Time offset</h3>"
                        "<p class='time_offset_section'>"
                            "<label for='hour_offset'>Hours</label>"
                            "<input type='number' id='hour_offset' name='hour_offset' value='";
            content += ((String)config_clock.hour_offset).c_str();
            content += "' min='-12' max='12'>"
                        "</p>"
                        "<p class='time_offset_section'>"
                            "<label for='minutes_offset'>Minutes</label>"
                            "<input type='number' id='minutes_offset' name='minutes_offset' value='";
            content += ((String)config_clock.minute_offset).c_str();
            content += "' min='0' max='59'>"
                        "</p>"
                        "<p class='time_offset_section'>"
                            "<button onclick='setOffset(this);'>Update time offset</button>"
                        "</p>"
                        "<hr>"
                        "<br>"
                        "<a href='/clear_wifi_settings' class='btn btn-primary'>Clear WiFi settings and restart in AP mode</a>"
                        "<br>"
                        "<hr>"
                        "<p>" + updateDateTime + "</p>"
                    "</body>"
                    "<script>"
                        "new Chart('pressureChart', {"
                            "type: 'line',"
                            "data: {"
                                "labels: [" + pressureLabelsStr + "],"
                                "datasets: [{" 
                                    "data: [" + pressureValuesStr + "],"
                                    "borderColor: 'red',"
                                    "fill: false"
                                "}]"
                            "},"
                            "options: {"
                            "   legend: {display: false}"
                            "}"
                        "});"
                        "function setOffset() {"
                            "var xhttp = new XMLHttpRequest();"
                            "xhttp.onreadystatechange = function() {"
                            "};"
                            "param='?hour_offset=' + document.getElementById('hour_offset').value + '&minutes_offset=' + document.getElementById('minutes_offset').value;"
                            "xhttp.open('GET', '/time_offset'+param, true);"
                            "xhttp.send();"
                        "}"
                        "const myTimeout = setTimeout (reloadPage, 60*60*1000);"
                        "function reloadPage() {"
                            "location.reload();"
                        "}"
                    "</script>"
                "</html>";
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

    Serial.println(" done");
}