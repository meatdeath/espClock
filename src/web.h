#ifndef __WEB_H__
#define __WEB_H__
#include <Arduino.h>

#define ENABLE_DNS

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <WiFiUdp.h>

#ifdef ENABLE_DNS
#include "ESPAsyncUDP.h"
#include <ESPAsyncDNSServer.h>
#endif

#include <Hash.h>
#include "ESPAsyncWebServer.h"
#include <ESP8266HTTPClient.h>

enum _web_pages_en { 
    WEB_PAGES_NORMAL = 0,
    WEB_PAGES_FOR_AP  
};

extern String html_PressureHistory;

extern String pressureLabelsStr;
extern String pressureValuesStr;

bool testWifi(void);
void launchWeb(int webtype);
void setupAP(void);
void createWebServer(int webtype);

#endif // __WEB_H__
