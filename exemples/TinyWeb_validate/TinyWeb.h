/*******************

   objet d'interface mini serveur web
   (C) 07/2020 NET23 Pierre HENRY

   V1.0    Extracted from Betaporte
   V1.0.1  Add interactive js
   V1.0.2  Stand alone captive portal
*/

#pragma once
#include <arduino.h>
//#include "ESP8266.h"
#include <ESP8266WiFi.h>
// littleFS
#include "LittleFS.h"  //Include File System Headers 


#define SERVEUR_PORT 80
#define SERVEUR_APTIMEOUT (2 * 60 )       //5 minutes max for AP

//#define APP_VERSION  "MiniWebserveur  V1.0"   //Signe down AP web page
//#define LED_LIFE  LED_BUILTIN                 //bling led for AP mode

//typedef enum WiFiMode 
//{
//    WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3,
//    /* these two pseudo modes are experimental: */ WIFI_SHUTDOWN = 4, WIFI_RESUME = 8
//} WiFiMode_t;



enum class TW_WiFiMode_t  {   WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_APSETUP = 100 };
//enum tws_WiFiMode_t   { TWS_WIFI_OFF, TWS_WIFI_STATION, TWS_WIFI_SOFTAP, TWS_WIFI_SETUP_STATION };
enum class TW_WiFiStatus_t { WIFI_OFF, WIFI_OK, WIFI_DISCONNECTED, WIFI_TRANSITION  };

// Pseudo abject mono instance just to hide lib
class MiniServeurWeb {
  public:
    MiniServeurWeb();                               // constructor mono instance
    ~MiniServeurWeb();                              // destructor mono instance
//    void begin();                                   // main server start with default host name
    void begin();
    void end();                                     // main server stop
    tw_WiFiMode_t    getWiFiMode();  // Wifi Mode experted by user
    void             setWiFiMode(tw_WiFiMode const mode);
    tw_WiFiStatus_t  getWiFiStatus();                  // Wifi Status 
    
    void connectWiFi(String ssid, String ssidpassword, bool const tryConfig = true); // setup main server wifi credential
    void configureWiFi(const bool active = false);  // active the AP mode to request wifi credential from the user
 //   void softAPconnect(const bool active,const bool persistent = false,const char* = NULL); 
    
    void handleClient();     // handle http service (to call in loop)
    
    void setCallBack_TranslateKey(void (*translateKey)(String &key));  // call back pour Fournir les [# xxxxx #]
    void setCallBack_OnRefreshItem(bool (*onRefreshItem)(const String &keyname, String &key));  // call back pour fournir les class='refresh'
    void setCallBack_OnSubmit(void (*onSubmit)(const String &key));          // call back pour gerer les submit
    void setCallBack_OnRepeatLine(bool (*onRepeatLine)(const int num));     // call back pour gerer les Repeat
    void redirectTo(String const uri);                                      // force a redirect of the current request (to use in OnSubmit)
    String getArg(const String argName);
    String currentUri();                            // return the last requested URI (actual page in calllback)
    // var
    byte debugLevel = 3;  // 0 = none 1 = minimal 2 = variable request 3 = wifi debug
    
    String  lastLocalIp;
   private:
   String  _hostname;   //  SSID en mode AP et Serveur name en mode STATION
    tw_WiFiMode_t    _WifiMode;
    tw_WiFiStatus_t  _WiFiStatus = TWS_TRANSITION;
};
