// src test file to validate the lib TinyWeb
// cleaner version of WebServer (C) V1.0.1 6/6/2020  NET234 P.HENRY
//
#include <Arduino.h>


#define LED_LIFE      LED_BUILTIN
#define APP_VERSION   "TinyWeb Validate V1.0"

#define LED_ON        LOW
//Objet serveur WEB
#include  "TinyWeb.h"
TinyWeb    ServeurWeb;




//
//bool debugStat = true;
//bool persistentStat = true;
//int  listenInterval = 0;
bool resetRequested = false;
//String wifiSSD;
//String wifiPASS;




void setup() {

  // Initialisation Hard des IO
  pinMode(LED_LIFE, OUTPUT);
  // init Serial
  Serial.begin(115200);
  Serial.println(F(APP_VERSION));

  Serial.setDebugOutput(true);  // affichage du debug mode pour webserver

  //  // recuperation du nom de la device
  //  LittleFS.begin();
  //  String aString;
  //  File aFile = LittleFS.open(F("config.txt"), "r");
  //  if (aFile) {
  //    aString = aFile.readStringUntil( '\n' );
  //    Serial.println(aString);
  //    if (aString.startsWith(F("HOSTNAME="))) {
  //      aString.remove(0, 9);
  //      ServeurWeb.hostname = aString;
  //    }
  //    aFile.close();
  //  }
  //Serveur WEB



  //  ServeurWeb.WiFiMode = WIFI_STA;  // mode par defaut
  //  ServeurWeb.setCallBack_TranslateKey(&traductionKey);
  //  ServeurWeb.setCallBack_OnSubmit(&webSubmit);
  //  ServeurWeb.setCallBack_OnRefreshItem(&on_RefreshItem);
  //  ServeurWeb.setCallBack_OnRepeatLine(&on_RepeatLine);
  ServeurWeb.begin();
}



void loop() {
  //  // run AP mode on BP0
  //  if (digitalRead(0) == LOW) {
  //    ServeurWeb.configureWiFi(true);
  //  }
  ServeurWeb.handleEvent();     // handle http service and wifi

  //Check ServeurWeb Status
  static long timer = millis();
  if (millis() - timer > 1000) {
    timer += 1000;
    if (resetRequested) {
      delay(1000);
      ESP.reset();
      while (1) {};
    }
  }

 if (ServeurWeb.WiFiModeChanged) {
    //{ twm_WIFI_OFF = 0, twm_WIFI_STA, twm_WIFI_AP,twm_WIFI_APSETUP };
    switch (ServeurWeb.getWiFiMode()) {
      case twm_WIFI_OFF:
        Serial.println(F("TW wifi Mode OFF"));
        break;
      case twm_WIFI_STA:
        Serial.println(F("TW wifi Mode Station"));
        break;
      case twm_WIFI_AP:
        Serial.println(F("TW wifi Mode AP"));
        break;
      case twm_WIFI_APSETUP:
        Serial.println("TW wifi Mode AP Setup");
        break;
      default:
        Serial.print(F("TW mode ?:"));
        Serial.println(ServeurWeb.getWiFiMode());
    } //switch
  }// if Mode changed

  
  
  
  if (ServeurWeb.WiFiStatusChanged) {
    //WIFI_OFF, WIFI_OK, WIFI_DISCONNECTED, WIFI_TRANSITION
    switch (ServeurWeb.getWiFiStatus()) {
      case tws_WIFI_TRANSITION:
        Serial.println(F("TS wifi en transition"));
        break;
      case tws_WIFI_OFF:
        Serial.println(F("TW wifi off"));
        digitalWrite(LED_LIFE, !LED_ON);
        break;
      case tws_WIFI_DISCONNECTED:
        Serial.println(F("TW wifi Deconnecte"));
        digitalWrite(LED_LIFE, !LED_ON);
        break;
      case tws_WIFI_OK:
        digitalWrite(LED_LIFE, LED_ON);
        Serial.println("TW wifi station Connected");
        break;
      default:
        Serial.print(F("TW Status?:"));
        Serial.println(ServeurWeb.getWiFiStatus());
    } //switch
  }// if status changed

  if (Serial.available())
  {
    char inChar = (char)Serial.read();
    switch (inChar) {
      case 'A':
        Serial.println("Wifi.begin()");
        WiFi.begin();
        break;
      case 'B':
        Serial.println("Wifi.mode(OFF)");

        WiFi.mode(WIFI_OFF);

        delay(100);
        break;
      case 'C':
        Serial.println("Wifi.mode(AP)");
        WiFi.mode(WIFI_AP);
        break;
      case 'D':
        Serial.println("Wifi.mode(STA)");
        WiFi.mode(WIFI_STA);
        break;
      //      case 'E':
      //        persistentStat = !persistentStat;
      //        Serial.print("persistent ");
      //        Serial.println(persistentStat);
      //        Serial.setDebugOutput(persistentStat);
      //        break;
      case 'H':
        Serial.println("forceSleepWake");

        WiFi.forceSleepWake();
        delay(100);
        break;

      case 'I':
        Serial.println("forceSleepBegin");

        WiFi.forceSleepBegin();
        delay(100);
        break;



      //      case 'G':
      //        debugStat = !debugStat;
      //        Serial.print("Debug ");
      //        Serial.println(debugStat);
      //        Serial.setDebugOutput(debugStat);
      //        break;

      //      case 'L':
      //        listenInterval = (listenInterval + 1) % 11;
      //        Serial.print("Light listenInterval ");
      //        Serial.println(listenInterval);
      //
      //        WiFi.setSleepMode (WIFI_LIGHT_SLEEP, listenInterval);
      //        break;
      //      case 'M':
      //        listenInterval = (listenInterval + 1) % 11;
      //        Serial.print("Modem listenInterval ");
      //        Serial.println(listenInterval);
      //
      //        WiFi.setSleepMode (WIFI_MODEM_SLEEP, listenInterval);
      //        break;

            case '0':
              Serial.println("setWiFiMode(WiFi_OFF)");
              ServeurWeb.setWiFiMode(twm_WIFI_OFF);
              break;
            case '1':
              Serial.println("setWiFiMode(WiFi_STA)");
              ServeurWeb.setWiFiMode(twm_WIFI_STA);
              break;
           case '2':
              Serial.println("setWiFiMode(WiFi_STA)");
              ServeurWeb.setWiFiMode(twm_WIFI_STA);
              break;
               
            case '3':
              Serial.println("setWiFiMode(WiFi_AP with SSID & PASS)");
              ServeurWeb.setWiFiMode(twm_WIFI_STA,"mon_wifi","ultrasecret");
              break;
      //      case '4':
      //        Serial.println("setmode wifi station");
      //        ServeurWeb.WiFiMode = TWS_WIFI_STATION;
      //        break;
      //      case '5':
      //        Serial.println("setmode wifi softap");
      //        ServeurWeb.WiFiMode = TWS_WIFI_SOFTAP;
      //        break;
      //      case '6':
      //        Serial.println("setmode wifi setup station station");
      //        ServeurWeb.WiFiMode = TWS_WIFI_SETUP_STATION;
      //        break;
      case 'R':
        Serial.println("reset.");
        delay(3000);
        ESP.reset();
        while (1) {};
        break;
      case 'S':
        WiFi.printDiag(Serial);
        break;
      case 'Z':
        Serial.print("long delay ");
        for (int N = 1; N <= 30; N++) {
          delay(1000);
          Serial.print('.');
        }
        Serial.println();
        break;

    }
  }
  delay(10);
}// loop
