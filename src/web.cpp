#include "web.h"
#include "config.h"



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
String html_PressureHistory;
String pressureLabelsStr;
String pressureValuesStr;

AsyncWebServer server(80);
const char* ssid = "ESPClock";
const char* passphrase = "1290test$#A";

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
        htmlRadio_NetworksList = "<div>";
        for (int i = 0; i < n; ++i)
        {
            // Print SSID and RSSI for each network found
            String ssid = WiFi.SSID(i);
            int32_t rssi = WiFi.RSSI(i);
            String encryption = (WiFi.encryptionType(i) == ENC_TYPE_NONE)?String(" "):String("*");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(ssid);
            Serial.print(" (");
            Serial.print(rssi);
            Serial.print(")");
            Serial.println(encryption);
            //delay(10);
            htmlRadio_NetworksList += "<div>";
            htmlRadio_NetworksList +=   "<input type='radio' id='" + ssid + "' name='ssid' value='" + ssid + "'>";
            htmlRadio_NetworksList +=   "<label for='" + ssid + "'>";
            htmlRadio_NetworksList +=       ssid + " (" + rssi + ")" + encryption;
            htmlRadio_NetworksList +=   "</label>";
            htmlRadio_NetworksList += "</div>";
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



void createWebServer(int webtype)
{
    if( webtype == WEB_PAGES_FOR_AP ) 
    {
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            IPAddress ip = WiFi.softAPIP();
            content = 
"<!DOCTYPE HTML>"
"   <html>"
"       <head>"
"			<style>"
"				input[type='radio'] { margin: 0 10px 0 0; }"
"			</style>"
"           <meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>"
"           <link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css' integrity='sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm' crossorigin='anonymous'>"
"       </head>"
"       <body>"
"           <div style='font-size:3em; padding:30px;'>"
"               <h1>ESPClock</h1>"
"               <p>IP address: " + ip.toString() + "</p>"
"               <hr>" +
                html_PressureHistory +
"               <hr>"
"               <p>"
"                   <label for='hour_offset'>Hours offset</label>"
"                   <input type='number' id='hour_offset' name='hour_offset' value='" + ((String)config.clock.hour_offset) + "' min='-12' max='12' onchange='setOffset(this);'>"
"               </p>"
"               <p>"
"                   <label for='minutes_offset'>Minutes offset</label>"
"                   <input type='number' id='minutes_offset' name='minutes_offset' value='" + ((String)config.clock.minute_offset) + "' min='0' max='59' onchange='setOffset(this);'>"
"               </p>"
"               <form method='get' action='setting'>"
                    "<hr>"
"                   <p style='margin:0 0 5px 0'>Choose a network to connect</p>" +
                    htmlRadio_NetworksList +
                    "<br>"
                    "<p>"
"                       <label for='pass'>Password</label>"
"                       <input name='pass' type='password' length=64>"
                    "</p>"
"                   <hr>"
"                   <button type='submit' class='btn btn-primary'>Save WiFi settings</button>"
"               </form>"
"           </div>"
"       </body>"
"       <script src='https://code.jquery.com/jquery-3.2.1.slim.min.js' integrity='sha384-KJ3o2DKtIkvYIK3UENzmM7KCkRr/rE9/Qpg6aAZGJwFDMVNA/GpGFF93hXpG5KkN' crossorigin='anonymous'></script>"
"       <script src='https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.12.9/umd/popper.min.js' integrity='sha384-ApNbgh9B+Y1QKtv3Rn7W3mgPxhU9K/ScQsAP7hUibX39j7fakFPskvXusvfa0b4Q' crossorigin='anonymous'></script>"
"       <script src='https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/js/bootstrap.min.js' integrity='sha384-JZR6Spejh4U02d8jOt6vLEHfe/JQGiRRSQQxSfFWpi1MquVdAyjUar5+76PVCmYl' crossorigin='anonymous'></script>"
"       <script>"
"           function setOffset() {"
                "var xhttp = new XMLHttpRequest();"
                "xhttp.onreadystatechange = function() {"
                "};"
                "param='?hour_offset=' + document.getElementById('hour_offset').value + '&minutes_offset=' + document.getElementById('minutes_offset').value;"
                "xhttp.open('GET', '/time_offset'+param, true);"
                "xhttp.send();"
"           }"
"       </script>"
"   </html>";

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
            content = "<!DOCTYPE HTML><html>";
            content += "       <script src='https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.5.0/Chart.min.js'></script>";
            content += "<body><h1>ESPClock</h1><p>IP address: ";
            //content += ipStr;
            content += ip.toString();
            content += 
"               <p>"
"                   <canvas id='pressureChart' style='width:100%;max-width:600px'></canvas>"
"               </p>";
            content += "</p><br><hr><br><label for='hour_offset'>Hour offset</label><input type='number' id='hour_offset' name='hour_offset' value='";
            content += ((String)config.clock.hour_offset).c_str();
            content += "' min='-12' max='12' onchange='setOffset(this);'>";
            content += "<label for='minutes_offset'>Minute offset</label><input type='number' id='minutes_offset' name='minutes_offset' value='";
            content += ((String)config.clock.minute_offset).c_str();
            content += "' min='0' max='59' onchange='setOffset(this);'>";
            content += "<br><hr><br><a href='/clear_wifi_settings' class='btn btn-primary'>Clear WiFi settings and restart in AP mode</a>";
            content += "</body>";
            
            content += "<script>"
"            new Chart('pressureChart', {"
"                type: 'line',"
"                data: {"
"                    labels: [" + pressureLabelsStr + "],"
"                    datasets: [{" 
"                       data: [" + pressureValuesStr + "],"
"                       borderColor: 'red',"
"                       fill: false"
"                    }]"
"                },"
"                options: {"
"                    legend: {display: false}"
"                }"
"            });"
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