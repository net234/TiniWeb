/*******************

   objet d'interface mini serveur web complet avec la gestion du WIFI
   todo: Grabout LittleFS.H
   todo: Grabout ESP8266WebServer
   todo: Better implantation of EEPROM
   todo: add level of debugin init
*/
#include "TinyWeb.h"

// pointeur vers l'instance utilisateur
TinyWeb* tinyWebPtr = NULL;


#ifndef APP_VERSION
#define APP_VERSION  "TinyWeb  V1.0"   //Signe down AP web page
#endif
// DNS server
const byte    DNS_PORT = 53;

//// données wifi sauvegardée en EEPROM
//#define MP_STRSIZE 25   // taille maxi d'une chaine dans les MP (Memoire Protegee)
#define MWEB_DEFAULT_SSID "TINYWEB_"

#include "wifisetup.h"

// ====== DO NOT REMOVE ===========================
// Compile without theses include but AP wont work on specific board ????
// with:341900/33152 => without:311880/33140
//#include <ESP8266WiFi.h>        // base d'acces au WIFI (included in .h)
#include <ESP8266WebServer.h>   // web server to answer WEB request and EEDOMUS BOX request
#include <ESP8266mDNS.h>      // not used here we stay with IP on this side (Wifimanageg need id on his side)
//#include <DNSServer.h>        // only used by capture portal to redirect web request to main page
// ====== DO NOT REMOVE ===========================


// Server Instance
//#include "user_interface.h"
ESP8266WebServer   Server(SERVER_PORT);    // Serveur HTTP
//DNSServer          dnsServer;
//HTTPClient       Client;        // Client HHTP (retours vers l'eedomus)
WiFiClient       ClientWifi;    // needed by HTTPClient to use Wifi

//for the captive portal only we need a dnsServer to grab all web request
//ESP8266mDNS   dnsServer;


//pointer du traducteur de Key
void (*translateKeyPtr)(String &key) = NULL;

void translateKey(String &key) {
  if ( key.equals(F("CHIP_ID")) ) {
    key = ESP.getChipId();
  } else if ( key.equals(F("FLASH_CHIP_ID")) ) {
    key = ESP.getFlashChipId();
  } else if ( key.equals(F("IDE_FLASH_SIZE")) ) {
    key = ESP.getFlashChipSize();
  } else if ( key.equals(F("REAL_FLASH_SIZE")) ) {
    key = ESP.getFlashChipRealSize();
  } else if ( key.equals(F("SOFTAP_IP")) ) {
    key = WiFi.softAPIP().toString();
  } else if ( key.equals(F("SOFTAP_MAC")) ) {
    key = WiFi.softAPmacAddress();
  } else if ( key.equals(F("STATION_IP")) ) {
    key = tinyWebPtr->_localIp;
  } else if ( key.equals(F("HOSTNAME")) ) {
    key = tinyWebPtr->_hostname;
  } else if ( key.equals(F("STATION_MAC")) ) {
    key = WiFi.macAddress();

    //specific wifisetuo
  } else if ( key.equals(F("SSID_NAME")) ) {
    key = WiFi.SSID(network[currentLine]);
  } else if ( key.equals(F("SSID_LEVEL")) ) {
    int level = RSSIdbToPercent(network[currentLine]);
    if (level > 100) level = 100;
    key = level;
  } else if ( key.equals(F("SSID_LOCK")) ) {
    key = "&nbsp;";
    if (WiFi.encryptionType(network[currentLine]) != ENC_TYPE_NONE) key = "&#128274;";
  } else if (translateKeyPtr) {
    (*translateKeyPtr)(key);
  }
}

//pointer du gestionaire de request
void (*onRequestPtr)(const String &filename) = NULL;

void onRequest(String &filename) {
  if (onRequestPtr) (*onRequestPtr)(filename);
}

//pointeur du gestionanire de refresh
bool  (*onRefreshItemPtr)(const String &keyname, String &key) = NULL;

bool onRefreshItem(const String &keyname, String &key) {
  if (onRefreshItemPtr) return (*onRefreshItemPtr)(keyname, key);
  return (false);
}

//pointeur du gestionaire de repeatLine
bool (*onRepeatLinePtr)(const int num) = NULL;

bool onRepeatLine(const int num) {

  if ( Server.arg(F("tinyweb")).equals(F("show_wifi")) ) {
    currentLine = num;
    if (currentLine == 0) scanNetwork();
    return (num < networkSize && RSSIdbToPercent(network[num]) > 25 );
  }

  if (onRepeatLinePtr) return (*onRepeatLinePtr)(num);
  return (false);
}




// ============   tools to handle captive portal  ============

//bool softAPRequested = false;
//bool softAP = false;            //Captive portal active
//bool captiveDNS = false;               //Captive portal DNS active
//bool rootWifiSetup = false;             //Specific for CaptivePortal WifiSetup hold in a sub repertory
//unsigned long timerCaptivePortal;      //Used to timeout Captive portal
//unsigned long timerCaptiveDNS;         //Used to reactive captive portal DNS
//unsigned long timerWiFiStop = 0;       //Force a WiFi stop in 2,5 sec



//// CaptiveDNS is active until a request to setupServer is done
//void   captiveDNSStart() {
//  Serial.println("Captive DNS ON");
//  /* Setup the DNS server redirecting all the domains to the apIP */
//  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
//  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
//  captiveDNS = true;
//  //timerCaptivePortal = millis();
//}


//void captiveDNSStop() {
//  Serial.println("Captive DNS OFF");
//  dnsServer.stop();
//  captiveDNS = false;
//  // rearme le timerCaptiveDNS
//  timerCaptiveDNS = millis();
//}

//void handleCaptivePortal() {
//#define DELAY_NOVALIDPAGE 1U * 60 * 1000
//#define CAPTIVEPORTAL_TIMEOUT  2U * 60 * 1000
//
//  if (softAP && millis() - timerCaptivePortal > CAPTIVEPORTAL_TIMEOUT) {
//    if (miniServerPtr) miniServerPtr->configureWiFi(false);  //Stop captive
//    WiFi.begin();
//    return;
//  }
//  // if no request arrived we rearm captive DNS
//  if (!captiveDNS && millis() - timerCaptiveDNS > DELAY_NOVALIDPAGE) {
//    captiveDNSStart();
//  }
//  if (captiveDNS) dnsServer.processNextRequest();
//}

//bool    newConfigTry = false;
//bool    newConfigValide = false;
//String  newConfigSSID;
//String  newConfigPASS;

// Out of instance variables
String  redirectUri;

//
// ============   tools to handle captive portal  (END) ============

// ============   tools to handle wifi mode =========




// ============   tools to handle wifi mode (END) =========

//// constructeur
TinyWeb::TinyWeb() {
  if (tinyWebPtr != NULL) {
    Serial.print(F("Only one instance for MiniServeurWeb"));
    while (true) delay(100);
  }
  tinyWebPtr = this;
}
// Destructor
TinyWeb::~TinyWeb() {
  if (tinyWebPtr == NULL) return;
  this->end();
  tinyWebPtr = NULL;
}




void TinyWeb::handleEvent() {
  //  static uint32_t timerDisconnected = 0;
  //  static uint32_t timerConnected = 0;
  //
  //  if (softAP) handleCaptivePortal();
  //
  // Analyse de l'etat du wifi pour detecter connecté ou deconnecté en mode AP
  // pour obtenir _WiFiStatus
  TW_WiFiStatus_t wifiStatus;
  int status = WiFi.status();

  //    0 : WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses
  //    1 : WL_NO_SSID_AVAILin case configured SSID cannot be reached
  //    3 : WL_CONNECTED after successful connection is established
  //    4 : WL_CONNECT_FAILED if password is incorrect
  //    6 : WL_DISCONNECTED if module is not configured in station mode
  //


  while (true) {
    // si IDLE  status = transition
    if (status == WL_IDLE_STATUS) {
      wifiStatus = tws_WIFI_TRANSITION;
      break;
    }
    // si wifi off
    if (_WiFiMode == twm_WIFI_OFF) {
      if (status == WL_DISCONNECTED) {
        wifiStatus = tws_WIFI_OFF;
      } else {
        wifiStatus = tws_WIFI_TRANSITION;
      }
      break;
    }
    // si mode station
    if (_WiFiMode == twm_WIFI_STA) {
      if (status == WL_CONNECTED) {
        wifiStatus = tws_WIFI_OK;
        _localIp = WiFi.localIP().toString();
      } else {
        wifiStatus = tws_WIFI_DISCONNECTED;
      }
      break;
    }
    // else (ap ou ap setup
    wifiStatus = tws_WIFI_OK;
    break;
  }

  if (wifiStatus != _WiFiStatus) {
    _WiFiStatus = wifiStatus;
    WiFiStatusChanged = true;
    MDNS.end();
    if (_WiFiMode == twm_WIFI_STA && _WiFiStatus == tws_WIFI_OK) {
      MDNS.begin(_hostname);
      Serial.print(F("MS DNS ON : "));
      Serial.println(_hostname);
    }
    Serial.print(F("WIFI status = "));
    Serial.println(wifiStatus);


  }

  Server.handleClient();
  if (_WiFiMode == twm_WIFI_STA) MDNS.update();
}


// on arme un timer pour avoir un peu de stabilité
//    if (status == WL_CONNECTED) {
//      timerConnected = millis();
//      timerDisconnected = 0;
//      timerWiFiStop = 0;
//      _WiFiStatus = TWS_TRANSITION;  // transitional
//    } else if (status == WL_DISCONNECTED) {
//      timerDisconnected = millis();
//      timerConnected = 0;
//      timerWiFiStop = 0;
//      _WiFiStatus = TWS_TRANSITION;  // transitional
//    }
//  }
//  uint32_t now = millis();
//  if (timerConnected && now - timerConnected > 2500) {
//    timerConnected = 0;
//    _WiFiStatus = TWS_STATION_CONNECTED; // = station connected
//    Serial.print(F("WIFI Connected, IP address: "));
//    lastLocalIp = WiFi.localIP().toString();
//    Serial.println(lastLocalIp);
//    Serial.print(F("host name "));
//    Serial.println(hostname.c_str());
//    Serial.print(F("Internal host name "));
//    Serial.println(WiFi.hostname(hostname.c_str()));
//    Serial.print(F("WIFI Mode "));
//    Serial.println(WiFi.getMode());
//
//    // rearme un serveur mDNS pour resoudre les hostname.local
//    MDNS.end();
//    if (!MDNS.begin(hostname.c_str())) {             // Start the mDNS responder for esp8266.local
//      Serial.println(F("Error setting up MDNS responder!"));
//    }  else {
//      Serial.println(F("mDNS responder started"));
//    }
//
//  }
//  if (timerDisconnected && now - timerDisconnected > 2000) {
//    timerDisconnected = 0;
//    Serial.println(F("WIFI diconnected "));
//    Serial.print(F("SoftAP SSID "));
//    Serial.println(WiFi.softAPSSID());
//    Serial.print(F("SoftAP IP "));
//    Serial.println(WiFi.softAPIP());
//
//    Serial.print(F("Station SSID "));
//    Serial.println(WiFi.SSID());
//    Serial.print(F("Station IP "));
//    Serial.println(WiFi.localIP());
//
//
//
//    Serial.print(F("WIFI Mode "));
//    Serial.println(WiFi.getMode());
//
//    _WiFiStatus = TWS_STATION_DISCONNECTED;
//    if (WiFiMode == TWS_WIFI_SOFTAP  || WiFiMode == TWS_WIFI_SETUP_STATION ) {
//      Serial.println(F("Disconnected but SoftAP on "));
//      _WiFiStatus = TWS_TRANSITION;
//      uint32_t badip = 0;
//      if (WiFi.softAPIP().isSet()) {
//        Serial.println(F("WIFI SoftAP ok "));
//        _WiFiStatus = TWS_SOFTAP_ACTIVE;
//        if (WiFiMode == TWS_WIFI_SETUP_STATION ) _WiFiStatus = TWS_SOFTAP_SETUP_STATION;
//      }
//    }
//  }
//  // set current wifi mode = to expected wifimode
//  static tw_WiFiMode_t currentWiFiMode = WIFI_OFF;
//  if (currentWiFiMode != WiFiMode ) {
//    Serial.print(F("Set wifimode "));
//    Serial.print(currentWiFiMode);
//    Serial.print(F(" to "));
//    Serial.println(WiFiMode);
//    currentWiFiMode = WiFiMode;
//    switch (WiFiMode) {
//      case TWS_WIFI_OFF:
//        MDNS.end();
//        WiFi.mode(WIFI_OFF);
//        delay(10);
//        WiFi.forceSleepBegin();
//        break;
//      case TWS_WIFI_STATION:
//        WiFi.mode(WIFI_STA);
//        break;
//      case TWS_WIFI_SOFTAP:
//        WiFi.mode(WIFI_AP);
//        break;
//      case TWS_WIFI_SETUP_STATION:
//        WiFi.mode(WIFI_AP);
//        break;
//
//    }
//  }
//  //    uint32_t now = millis();
//  //    if (timerWiFiStop && now - timerWiFiStop > 2500) {
//  //      timerWiFiStop = 0;
//  //      MDNS.end();
//  //      WiFi.mode(WIFI_OFF);
//  //      delay(10);
//  //      WiFi.forceSleepBegin();
//  //    }
//  //
//  //
//  //
//  //
//  //    if (softAPRequested) {
//  //      _status = 0;
//  //      newConfigTry = false;
//  //      newConfigValide = false;
//  //      Serial.print("Captive ON --> ");
//  //      softAP = true;
//  //      //      if (!persistent) timerCaptivePortal = millis();
//  //      //      WiFi.persistent(persistent);
//  //      //      WiFi.mode(WIFI_OFF);
//  //      //      delay(500);
//  //
//  //      Serial.println("Captive ON --> config soft AP config");
//  //      IPAddress local_IP(169, 254, 169, 254);
//  //      IPAddress gateway(169, 254, 169, 254);
//  //      IPAddress subnet(255, 255, 255, 0);
//  //      bool result;
//  //      result = WiFi.softAPConfig(local_IP, gateway, subnet);
//  //      Serial.print("AP config : ");
//  //      Serial.println(result);
//  //      if (WiFi.softAPIP().isSet()) {
//  //        _status = 3;
//  //      }
//  //
//  //      // activavation du captive DNS
//  //      captiveDNSStart();
//  //
//  //      Serial.println("Captive ON --> Start SOFT AP");
//  //      result = WiFi.softAP(hostname.c_str(), NULL); // if password    8 >= len <= 62
//  //      Serial.print("AP begin : ");
//  //      Serial.println(result);
//  //      delay(10);
//  //      Serial.print("AP IP : ");
//  //      Serial.println(WiFi.softAPIP());
//  //      softAPRequested = false;
//  //
//  //    }
//  //    if (newConfigTry) {
//  //      Serial.print(F("New Config Ok "));
//  //      newConfigTry = false;
//  //      newConfigValide = true;
//  //    }
//  //    if (newConfigValide) {
//  //      bool result = WiFi.begin(newConfigSSID, newConfigPASS);
//  //      _status = 0;
//  //      Serial.print(F("New Config Active "));
//  //      delay(200);
//  //      Serial.println(result);
//  //      newConfigValide = false;
//  //    }
//  //
//  //
//  //  }
//  MDNS.update();
//  Serveur.handleClient();
//}

TW_WiFiStatus_t TinyWeb::getWiFiStatus() {
  WiFiStatusChanged = false;
  return (_WiFiStatus);
}
TW_WiFiMode_t TinyWeb::getWiFiMode() {
  WiFiModeChanged = false;
  return (_WiFiMode);
}

//case TWS_WIFI_OFF:
//        MDNS.end();
//        WiFi.mode(WIFI_OFF);
//        delay(10);
//        WiFi.forceSleepBegin();
//        break;
//      case TWS_WIFI_STATION:
//        WiFi.mode(WIFI_STA);
//        break;
//      case TWS_WIFI_SOFTAP:
//        WiFi.mode(WIFI_AP);
//        break;
//      case TWS_WIFI_SETUP_STATION:
//        WiFi.mode(WIFI_AP);
//        break;

//
//
//// Mode STA
//// todo  try connection and revalidate old credential if not ok
//void MiniServeurWeb::connectWiFi(String ssid, String password, bool const tryConfig) {
//  configureWiFi(false);  //Stop captive
//  newConfigSSID = ssid;
//  newConfigPASS = password;
//  newConfigTry  = tryConfig;
//  newConfigValide = !tryConfig;
//  timerWiFiStop = millis();
//  _WiFiStatus = TWS_TRANSITION;  // en transition
//  delay(10);
//  //  Serial.print("connectWiFi: ");
//  //
//  // // ETS_UART_INTR_DISABLE();
//  // //   WiFi.disconnect();
//  // // ETS_UART_INTR_ENABLE();
//  // // delay(10);
//  //  WiFi.persistent(true);  // just in case
//  //  delay(10);
//  //  WiFi.mode(WIFI_OFF);
//  //
//  //  int N = 0;
//  //  while (WiFi.status() !=WL_DISCONNECTED && N++ < 50) {
//  //    delay(100); Serial.print('X');
//  //  }
//  ////    wifi_station_disconnect();
//  ////    wifi_set_opmode(NULL_MODE);
//  ////    wifi_set_sleep_type(MODEM_SLEEP_T);
//  ////    wifi_fpm_open();
//  ////    #define FPM_SLEEP_MAX_TIME 0xFFFFFFF
//  ////    wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);
//  ////    delay(500);
//  //
//  //  //      WiFi.set_sleep_level(MAX_SLEEP_T) (SDK3)
//  //  //     WiFi.set_listen_interval          (SDK3)
//  //  //    WiFi.set_sleep_type               (all SDKs)
//  //  Serial.print(F("Internal set host name1 "));
//  //  Serial.println(hostname);
//  //  WiFi.hostname(hostname);
//  //  Serial.print(F("Internalset host name2 "));
//  //  Serial.println(WiFi.hostname());
//  // // delay(500);
//  // // WiFi.mode(WIFI_STA);
//  // // delay(500);
//  //  bool result = WiFi.begin(ssid, password);
//  //  delay(200);
//  //  Serial.println(result);
//  //  N = 0;
//  //   while (WiFi.status() !=WL_CONNECTED && N++ < 50) {
//  //    delay(100); Serial.print('Y');
//  //  }
//  //
//  //  return result;
//}

// Mode AP

//void MiniServeurWeb::softAPconnect(const bool active, const bool persistent, const char* password) {
//  if (softAP == active) {
//    return;
//  }
//  rootWifiSetup = false;
//  if (softAP) {
//    Serial.print("Captive OFF --> ");
//    //   WiFi.persistent(true);  // just in case
//    WiFi.softAPdisconnect (true);
//    //WiFi.mode(WIFI_STA);
//    softAP = false;
//    captiveDNSStop();
//
//    Serial.println("Captive OFF --> END");
//    return;
//  }
//  Serial.print("Captive ON --> ");
//  softAP = true;
//  if (!persistent) timerCaptivePortal = millis();
//  WiFi.persistent(persistent);
//  WiFi.mode(WIFI_OFF);
//  delay(500);
//
//  Serial.println("Captive ON --> config soft AP config");
//  IPAddress local_IP(169, 254, 169, 254);
//  IPAddress gateway(169, 254, 169, 254);
//  IPAddress subnet(255, 255, 255, 0);
//  bool result;
//  result = WiFi.softAPConfig(local_IP, gateway, subnet);
//  Serial.print("AP config : ");
//  Serial.println(result);
//
//  // activavation du captive DNS
//  captiveDNSStart();
//
//  Serial.println("Captive ON --> Start SOFT AP");
//  result = WiFi.softAP(hostname.c_str(), password);
//  Serial.print("AP begin : ");
//  Serial.println(result);
//  delay(10);
//  Serial.print("AP IP : ");
//  Serial.println(WiFi.softAPIP());
//  WiFi.persistent(true);
//}




//void MiniServeurWeb::configureWiFi(const bool active) {
//
//  if (softAP == active) {
//    return;
//  }
//  //  ETS_UART_INTR_DISABLE();
//  //  WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
//  //  ETS_UART_INTR_ENABLE();
//  //  delay(10);
//
//  if (softAP) {
//    Serial.print("Captive OFF --> ");
//    //   WiFi.persistent(true);  // just in case
//    WiFi.softAPdisconnect (true);
//    //WiFi.mode(WIFI_STA);
//    softAP = false;
//    rootWifiSetup = false;
//    captiveDNSStop();
//
//    Serial.println("Captive OFF --> END");
//    return;
//  }
//  rootWifiSetup = true;
//  Serial.print("Captive ON --> ");
//  softAP = true;
//  timerCaptivePortal = millis();
//  WiFi.persistent(false);
//  // disconnect sta, start ap
//  //  ETS_UART_INTR_DISABLE();
//  //  WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
//  //  ETS_UART_INTR_ENABLE();
//  WiFi.mode(WIFI_OFF);
//  delay(100);
//
//
//  Serial.println("Captive ON --> config soft AP config");
//
//  IPAddress local_IP(169, 254, 169, 254);
//  IPAddress gateway(169, 254, 169, 254);
//  IPAddress subnet(255, 255, 255, 0);
//  bool result;
//  result = WiFi.softAPConfig(local_IP, gateway, subnet);
//  Serial.print("AP config : ");
//  Serial.println(result);
//
//  // activavation du captive DNS
//  captiveDNSStart();
//
//  Serial.println("Captive ON --> Start SOFT AP");
//  result = WiFi.softAP(hostname.c_str(), NULL);
//  Serial.print("AP begin : ");
//  Serial.println(result);
//  delay(10);
//  Serial.print("AP IP : ");
//  Serial.println(WiFi.softAPIP());
//
//
//
//
//  WiFi.persistent(true);
//}




//String MiniServeurWeb::getArg(const String argName) {
//  return Serveur.arg(argName);
//}
//
//
//// Dealing with WEB HTTP request
//#define LOCHex2Char(X) (X + (X <= 9 ? '0' : ('A' - 10)))
char LOCHex2Char( byte aByte) {
  return aByte + (aByte <= 9 ? '0' : 'A' -  10);
}
//
//
//// looking for requested file into local web pages
void HTTP_HandleRequests() {

  Serial.print(F("WEB receved a "));
  Serial.print( String((Server.method() == HTTP_GET) ? "GET " : "POST " ));
  Serial.print(Server.uri());
  Serial.print(F(" from "));
  Serial.print(Server.client().remoteIP());
  Serial.println();
  //  // interception en mode captive
  //  Serial.print(F("hostHeader : "));
  //  Serial.print(Serveur.hostHeader());
  //  Serial.println();
  //
  //  // in captive mode all requests to html or txt are re routed to "http://localip()" with a 302 reply
  //  if (softAP && !(Serveur.hostHeader().startsWith("169.254.169.254")) && (Serveur.uri().endsWith(".html") || Serveur.uri().endsWith("redirect") || Serveur.uri().endsWith(".txt")) ) {
  //    Serial.println(F("Request redirected to captive portal"));
  //    String aStr = F("http://");
  //    aStr += Serveur.client().localIP().toString();
  //    //   aStr += F("/APSetup/WifiManagement.html");
  //    Serveur.sendHeader("Location", aStr, true);
  //    //    Serveur.sendHeader("Location", String("http://") + Serveur.client().localIP().toString() + "/APSetup/WifiManagement.html", true);
  //    Serveur.send ( 302, "text/plain", "");
  //    Serveur.client().stop();
  //    return;
  //  }
  //
  //  // specific for firefox to hide captive mode
  //  if (softAP && Serveur.uri().endsWith("generate_204") ) {
  //    Serial.println(F("Generate204"));
  //    Serveur.setContentLength(0);
  //    Serveur.send ( 204 );
  //    Serveur.client().stop();
  //    return;
  //  }
  //
  //  // rearm timeout for captive portal
  //  // to hide captive mode stop DNS captive if a request is good (hostheader=localip)
  //  if (softAP) {
  //    timerCaptivePortal = millis();
  //    if (captiveDNS) captiveDNSStop();
  //  }
  //
  //
  // standard query are rooted to web folder
  String fileName = tinyWebPtr->webFolder;
  //  // captive query are rooted to web/wifisetup folder
  //  if (rootWifiSetup) fileName += "/wifisetup";

  fileName += Server.uri();
  // todo   protection against ../

  if (fileName.endsWith(F("/")) ) fileName += "index.html";   //default page ;

  String fileMIME;

  // find MIMETYPE for standard files
  enum fileType_t { NONE, CSS, ICO, PNG, JPG, GIF, JS, HTML } fileType = NONE;
  if ( fileName.endsWith(F(".css")) ) {
    fileType = CSS;
    fileMIME = F("text/css");
  } else if ( fileName.endsWith(F(".ico")) ) {
    fileType = ICO;
    fileMIME = F("image/x-icon");
  } else if ( fileName.endsWith(F(".png")) ) {
    fileType = PNG;
    fileMIME = F("image/png");
  } else if ( fileName.endsWith(F(".jpg")) ) {
    fileType = JPG;
    fileMIME = F("image/jpg");
  } else if ( fileName.endsWith(F(".gif")) ) {
    fileType = GIF;
    fileMIME = F("image/gif");
  } else if ( fileName.endsWith(F(".js")) ) {
    fileType = JS;
    fileMIME = F("application/javascript");
  } else if ( fileName.endsWith(F(".html")) ) {
    fileType = HTML;
    fileMIME = F("text/html");
  }

  // On request callback is call to inform sketch of any http request
  // if redirectTo(aUri) is set then an error 302 will be sent to redirect request

  redirectUri = "";
  onRequest(fileName);
  // if call back onRequest want a redirect
  if ( redirectUri.length() == 0 && Server.hasArg(F("submit")) ) {
    String submit = Server.arg(F("submit"));
    Serial.print(F("WEB: Submit action '"));
    Serial.print(submit);
    Serial.println("'");
    for (uint8_t i = 0; i < Server.args(); i++) {
      String argname = Server.argName(i);
      if ( !argname.equals(F("submit")) && !argname.equals(F("plain")) ) {
        Serial.print(F("WEB: Submit arg "));
        Serial.print(argname);
        Serial.print(F("->"));
        Serial.println(Server.arg(i));
        String value = Server.arg(i);
        //  onSubmit(value);   //appel du callback
      }
    }
  }

  if (redirectUri.length() > 0) {
    Serial.print(F("WEB redirect "));
    Serial.println(redirectUri);
    Server.sendHeader("Location", redirectUri, true);
    Server.send ( 302, "text/plain", "");
    Server.client().stop();
    return;
  }




  //  // ================ SPECIFIC FOR "QUERY REFRESH"  START =======================
  //
  // query 'refresh' is a POST sended by 'tinyWeb.js' to update class 'refresh' web items in real time
  // if first arg is named 'refresh'  all itmes are translated with  onRefreshItem  call back
  // then they are sended back to client in a urlencoded string
  if (Server.method() == HTTP_POST && Server.args() > 0 && Server.argName(0) == "refresh") {
    // debug track
    Serial.print(F("WEB: Query refresh ("));

    Serial.print(Server.arg(Server.args() - 1).length());
    Serial.print(F(") "));
    Serial.println(Server.arg(Server.args() - 1)); // try to get 'plain'
    // traitement des chaines par le Sketch
    String answer;
    answer.reserve(1000);   // should be max answer
    String aKey;
    aKey.reserve(500);      // should be max for 1 value
    String aKeyName;
    aKeyName.reserve(50);
    for (uint8_t N = 0; N < Server.args(); N++) {
      //     Serial.print(F("WEB: refresh "));
      //     Serial.print(Serveur.argName(N));
      //     Serial.print(F("="));
      //      Serial.println(Serveur.arg(N));
      aKeyName = Server.argName(N);
      aKey = Server.arg(N);
      if ( !aKeyName.equals(F("plain")) && onRefreshItem(aKeyName, aKey) ) {
        if (answer.length() > 0) answer += '&';
        answer +=  aKeyName;
        answer +=  '=';
        if ( aKey.length() > 500) {
          aKey.remove(490);
          aKey += "...";
        }
        // uri encode maison :)
        for (int N = 0; N < aKey.length(); N++) {
          char aChar = aKey[N];
          if ( isAlphaNumeric(aChar) ) {
            answer +=  aChar;
          } else {
            answer +=  '%';
            answer += LOCHex2Char( aChar >> 4 );
            answer += LOCHex2Char( aChar & 0xF);
          } // if alpha
        } // for
      } // valide keyname
    } // for each arg

    Serial.print(F("WEB: refresh answer ("));
    Serial.print(answer.length());
    Serial.print(F(") "));
    Serial.println(answer);
    //
    Server.sendHeader("Cache-Control", "no-cache");
    Server.setContentLength(answer.length());
    Server.send(200, fileMIME.c_str(), answer);
    //    Serveur.client().stop();
    //    Serial.print(answer.length());
    //    Serial.println(F(" car."));

    return;
  }





  //  // ================ QUERY REFRESH  END =======================
  //
  //
  //  ================= Deal with local web pages ============
  File aFile;
  if (fileType != NONE) aFile = LittleFS.open(fileName, "r");
  bool doChunk = false;
  if (aFile) {
    if (fileType == HTML) {
      Server.sendHeader("Cache-Control", "no-cache");
      Server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      doChunk = true;
    } else {
      Server.sendHeader("Cache-Control", "max-age=10800, public");
      Server.setContentLength(aFile.size()); //CONTENT_LENGTH_UNKNOWN
    }
    Server.send(200, fileMIME.c_str());
    aFile.setTimeout(0);   // to avoid delay at EOF
    static    char aBuffer[1025];               // static dont overload heap
    static    char repeatBuffer[1025];          // static dont overload heap
    //todo avoid repeatBuffer if repeat not used
    int  repeatNumber;
    bool repeatActive = false;
    // repeat until end of file
    while (aFile.available()) {
      int size;
      if (!doChunk) {

        // standard file (not HTML) are with 1024 byte buffer
        size = aFile.readBytes( aBuffer, 1024 );
      } else {

        // chunked file are read line by line with spefic keyword detection
        if (!repeatActive) {

          // if not in repeat line mode   just read one line
          size = aFile.readBytesUntil( '\n', aBuffer, 1000 );
          if (size < 1000) aBuffer[size++] = '\n'; //!!! on exactly 1000 bytes lines the '\n' will be lost :)
          aBuffer[size] = 0x00; // make aBuffer a Cstring
          // Gestion du [# REPEAT_LINE #]
          // if a line start with  [#REPEAT_LINE#] it will be sended while OnRepat Call bask retourne true
          // this help to display records of database
          if (strncmp( "[#REPEAT_LINE#]", aBuffer, 15) == 0) {
            //            Serial.println(F("Repeat line detected"));
            // Save the line in  repeat buffer
            strcpy(repeatBuffer, aBuffer + 15);
            repeatActive = true;
            repeatNumber = 0;
          }
        }
        if (repeatActive) {
          aBuffer[0] = 0x00;
          // ask the sketch if we should repeat
          repeatActive = onRepeatLine(repeatNumber++);
          if ( repeatActive ) strcpy(aBuffer, repeatBuffer);

          size = strlen(aBuffer);
        }

        char* currentPtr = aBuffer;
        // cut line in par to deal with kerwords "[# xxxxx #]"
        while ( currentPtr = strstr(currentPtr, "[#") ) {  // start detected
          char* startPtr = currentPtr + 2;
          char* stopPtr = strstr( startPtr + 1, "#]" ); // at least 1 letter keyword [#  #]
          int len = stopPtr - startPtr;
          if (  !stopPtr || len <= 0 || len >= 40 ) { // abort if no stop or lenth of keyword over 40
            break;
          }
          // grab keyword
          char aKey[41];
          strncpy(aKey, startPtr, len);
          aKey[len] = 0x00;   // aKey is Cstring
          //Serial.print("Key=");
          //Serial.println(aKey);
          String aStr;
          aStr.reserve(100);
          aStr = aKey;
          aStr.trim();
          // callback to deal with keywords
          translateKey(aStr);

          // Copie de la suite de la chaine ailleur
          static  char bBuffer[500];   //  todo   deal correctly with over 500 char lines
          strncpy(bBuffer, stopPtr + 2, 500);

          // Ajout de la chaine de remplacement
          strncpy(currentPtr, aStr.c_str(), 100);
          currentPtr += aStr.length();
          // Ajoute la suite
          strncpy(currentPtr, bBuffer, 500);
          size = strlen(aBuffer);

          //
        }// while
      } // else do chunk
      //
      //      Serial.print('.');
      if (size) Server.sendContent_P(aBuffer, size);
    }  // if avail
    if (doChunk) Server.chunkedResponseFinalize();
    //    Serial.println("<");
    Server.client().stop();
    aFile.close();
    return;
  }
  //  //  }
  Serial.println("error 404");
  String message = F("File Not Found\n");
  message += "URI: ";
  message += Server.uri();
  message += F("\nMethod: ");
  message += (Server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += Server.args(); // last is plain = all arg
  message += F("\n<br>");
  for (uint8_t i = 0; i < Server.args(); i++) {
    message += " " + Server.argName(i) + ": " + Server.arg(i) + "\n<br>";
  }
  Serial.println(message);
  message += "<H2><a href=\"/\">go home</a></H2><br>";
  Server.send(404, "text/html", message);
  Server.client().stop();
  //}
}





//
//
void TinyWeb::setCallBack_TranslateKey(void (*translatekey)(String & key))  {
  translateKeyPtr =  translatekey;
}
//
//void MiniServeurWeb::setCallBack_OnSubmit(void (*onsubmit)(const String & key))  {
//  onSubmitPtr =  onsubmit;
//}
//
void TinyWeb::setCallBack_OnRefreshItem(bool (*onrefreshitem)(const String & keyname, String & key)) {
  onRefreshItemPtr = onrefreshitem;
}
//
//
void TinyWeb::setCallBack_OnRepeatLine(bool (*onrepeatline)(const int num)) {     // call back pour gerer les Repeat
  onRepeatLinePtr = onrepeatline;
}
//
//
//String MiniServeurWeb::currentUri() {
//  return Serveur.uri();
//}
//
//
void TinyWeb::redirectTo(String const uri) {
  redirectUri = uri;
}


void TinyWeb::end() {
  WiFi.mode(WIFI_OFF);
  //  softAP = false;
  delay(10);
  WiFi.forceSleepBegin();
  delay(10);
  Server.close();
  delay(10);
}


// initialise le serveur WEB
// de fait on suppose que le WiFi interne de l'ESP est deja configuré
// et qu'il se connectera tout seul
// return false si le WiFi interne de l'ESP n'est pas en mode AP
// la config se fera par l'utilisateur via

// Just check if hostname is valide,
void TinyWeb::setHostname(const String hostname) {
  _hostname = hostname;
  // Check a valid hostname
  // configation du nom du reseau AP : LITTLEWEB_XXYY  avec les 2 dernier chifre hexa de la mac adresse
  if (_hostname.length() > 30) _hostname.remove(30);   // chop at 30char
  _hostname.trim();
  if (_hostname.length() < 4 ) _hostname = F(MWEB_DEFAULT_SSID "*");
  if (_hostname.endsWith(F("*"))) {
    _hostname.remove(_hostname.length() - 1);
    _hostname += WiFi.macAddress().substring(12, 14);
    _hostname += WiFi.macAddress().substring(15, 17);
  }
  _hostname.replace(' ', '_');

}
void TinyWeb::begin() {

  // recuperation des info en flash WIFI
  Serial.println(F("Flash config "));
  Serial.print(F("SoftAP SSID "));
  Serial.println(WiFi.softAPSSID());
  Serial.print(F("SoftAP IP "));
  Serial.println(WiFi.softAPIP());

  Serial.print(F("Station SSID "));
  Serial.println(WiFi.SSID());
  Serial.print(F("Station IP "));
  Serial.println(WiFi.localIP());



  Serial.print(F("WIFI Mode "));
  Serial.println(WiFi.getMode());



  // la config est en flash coté module WIFI
  // le mode de enregistré est obtenu par WiFi.getMode() qui retourne WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3
  switch (WiFi.getMode()) {
    case WIFI_STA:
      _WiFiMode = twm_WIFI_STA;
      break;
    case WIFI_AP:
      _WiFiMode = twm_WIFI_AP;
      break;
    default:   // dans les autres cas on force un OFF
      _WiFiMode = twm_WIFI_OFF;
      WiFi.mode(WIFI_OFF);
      delay(10);
      WiFi.forceSleepBegin();
      delay(10);
  }
  WiFiModeChanged = true;

  // le hostname est SSID du mode AP il servira aussi pour le nom DNS en mode station
  setHostname(WiFi.softAPSSID());
  Serial.print("Hostname:");
  Serial.println(_hostname);
  // FS
  if (!LittleFS.begin()) {
    Serial.println("FS en erreur");
  } else {
    Serial.println("FS Ok");
  }



  //  Serial.println(F("Set WIFI ON"));
  //  WiFi.begin();
  //  delay(1000);
  // mise en place des call back web
  //Serveur.on(F("/"), HTTPCallBack_display);                        // affichage de l'etat du SONOFF
  Server.onNotFound(HTTP_HandleRequests);
  Serial.setDebugOutput(debugLevel >= 3);
  Serial.print("Serveur.begin");
  Server.begin();

  Serial.print("Hostname : ");
  Serial.println(_hostname);
  delay(100);



  return ;
}


void TinyWeb::setWiFiMode(TW_WiFiMode_t mode, const char *ssid, const char *password) {
  _WiFiMode = mode;
  WiFiModeChanged = true;
  switch (mode) {
    case twm_WIFI_OFF:
      MDNS.end();
      WiFi.mode(WIFI_OFF);
      delay(10);
      WiFi.forceSleepBegin();
      delay(10);
      break;
    case twm_WIFI_STA:
      _WiFiMode = twm_WIFI_STA;
      WiFi.softAP(_hostname);  // memorise hostname in flash
      WiFi.hostname(_hostname);
      WiFi.mode(WIFI_STA);
      if (ssid != NULL) WiFi.begin(ssid, password);
      break;
    case twm_WIFI_AP:
      WiFi.mode(WIFI_AP);
      break;
      //    case TWS_WIFI_SETUP_STATION:
      //      WiFi.mode(WIFI_AP);
      //      break;

  }
}
