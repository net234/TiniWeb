/*******************
   TiniWeb.h (C) 2020 net234 net23@frdev.com
   library to setup an interactive web server


  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>

  History
  cleaner version of WebServer (C) V1.2 6/6/2020  NET234 P.HENRY net23@frdev.com

   V1.0    Extracted from Betaporte
   V1.0.1  Add interactive js
   V1.0.2  Stand alone captive portal
*/

#pragma once
#include <arduino.h>
//#include "ESP8266.h"
//#include <ESP8266WiFi.h>
// littleFS
#include "LittleFS.h"  //Include File System Headers 


#define SERVER_PORT 80                    // default port for http
#define SERVER_APSETUPTIMEOUT (3 * 60 )       // Timeout to release mode WIFI_APSETUP


//typedef enum WiFiMode
//{
//    WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3,
//    /* these two pseudo modes are experimental: */ WIFI_SHUTDOWN = 4, WIFI_RESUME = 8
//} WiFiMode_t;



enum TW_WiFiMode_t   { twm_WIFI_OFF = 0, twm_WIFI_STA, twm_WIFI_AP, twm_WIFI_APSETUP };
enum TW_WiFiStatus_t { tws_WIFI_OFF = 0, tws_WIFI_OK, tws_WIFI_DISCONNECTED, tws_WIFI_TRANSITION  };

// Object limited to one instance
class TinyWeb {
  public:
    TinyWeb();                               // constructor mono instance grab current wifi configuration
    ~TinyWeb();                              // destructor mono instance
    void begin();                                   // main server start as was configured
    void end();                                     // main server stop
    TW_WiFiMode_t    getWiFiMode();  // Wifi Mode expected by user
    void             setWiFiMode(TW_WiFiMode_t const mode, const char* ssid = NULL , const char* ssidpassword = NULL);
    TW_WiFiStatus_t  getWiFiStatus();                  // Wifi Status is scanned during handleClient()
    void setHostname(const String hostname);

    //void setSTAWiFi(String ssid, String ssidpassword); // setup AP server wifi credential
    //    void configureWiFi(const bool active = false);  // active the AP mode to request wifi credential from the user
    // //   void softAPconnect(const bool active,const bool persistent = false,const char* = NULL);
    //
    void handleEvent();     // handle http service (to call in top of loop)

    void setCallBack_OnRequest(void (*onRequest)(const String &key));          // call back pour gerer les request
    void redirectTo(String const uri);   //  request will be redirected to this URI if set during or after onRequest call back                                   // force a redirect of the current request (to use in OnSubmit)
    //
    void setCallBack_TranslateKey(void (*translateKey)(String &key));  // call back pour Fournir les [# xxxxx #]
    void setCallBack_OnRefreshItem(bool (*onRefreshItem)(const String &keyname, String &key));  // call back pour fournir les class='refresh'

    void setCallBack_OnRepeatLine(bool (*onRepeatLine)(const int num));     // call back pour gerer les Repeat
    //    String getArg(const String argName);
    //    String currentUri();                            // return the last requested URI (actual page in calllback)
    //    // var
    byte debugLevel = 3;  // 0 = none 1 = minimal 2 = variable request 3 = wifi debug (must be set before begin)
    //
    String  _localIp;

    bool WiFiStatusChanged = false;   // Flag set when wifi_Status change
    bool WiFiModeChanged = false;   // Flag set when wifi_Status change
    String webFolder = "/web";
    String  _hostname;      //  SSID en mode AP et Serveur name en mode STATION
  private:
    //    String  _redirectUri;   //  request will be redirected to this URI if set after onRequest call back

    TW_WiFiMode_t    _WiFiMode;
    TW_WiFiStatus_t  _WiFiStatus = tws_WIFI_TRANSITION;

};
