#ifndef __WEB_H__
#define __WEB_H__
#include <Arduino.h>

//#define ENABLE_DNS
//#define ENABLE_MDNS

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include "ESPAsyncWebServer.h"
#ifdef ENABLE_MDNS
#include <ESP8266HTTPClient.h>
#include "ESP8266mDNS.h"
#endif

#include <WiFiUdp.h>
#include <Hash.h>

#ifdef ENABLE_DNS
#include "ESPAsyncUDP.h"
#include <ESPAsyncDNSServer.h>
#endif

#define LED_PIN         D9
#define ledInit()       pinMode(LED_PIN,OUTPUT)
#define ledOn()         digitalWrite(LED_PIN, LOW)
#define ledOff()        digitalWrite(LED_PIN, HIGH)
#define ledToggle()     digitalWrite(LED_PIN, !digitalRead(LED_PIN))

enum _web_pages_en { 
    WEB_PAGES_NORMAL = 0,
    WEB_PAGES_FOR_AP  
};

typedef enum state_wifi_en {
    STATE_WIFI_IDLE,
    STATE_WIFI_CONNECTING,
    STATE_WIFI_CONNECTED,
    STATE_WIFI_AP
} StateWifi;
extern StateWifi WifiState;

void web_init(void);
void launchWeb(int webtype);
void setupAP(void);
void createWebServer(int webtype);
void wifi_processing(void);

#endif // __WEB_H__
