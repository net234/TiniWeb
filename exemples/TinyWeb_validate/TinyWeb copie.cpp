/*******************

   objet d'interface mini serveur web complet avec la gestion du WIFI
   todo: Grabout LittleFS.H
   todo: Grabout ESP8266WebServer
   todo: Better implantation of EEPROM
   todo: add level of debugin init
*/
#include "MiniserveurWeb.h"

// pointeur vers l'instance utilisateur
MiniServeurWeb* miniServerPtr = NULL;


#ifndef APP_VERSION
#define APP_VERSION  "MiniWebserveur  V1.0.2"   //Signe down AP web page
#endif
// DNS server
const byte    DNS_PORT = 53;

// données wifi sauvegardée en EEPROM
#define MP_STRSIZE 25   // taille maxi d'une chaine dans les MP (Memoire Protegee)
#define MWEB_DEFAULT_SSID "MINIWEB_"



// ====== DO NOT REMOVE ===========================
// Compile without theses include but AP wont work on specific board ????
// with:341900/33152 => without:311880/33140
//#include <ESP8266WiFi.h>        // base d'acces au WIFI (included in .h)
#include <ESP8266WebServer.h>   // web server to answer WEB request and EEDOMUS BOX request
#include <ESP8266mDNS.h>      // not used here we stay with IP on this side (Wifimanageg need id on his side)
#include <DNSServer.h>        // only used by capture portal to redirect web request to main page
// ====== DO NOT REMOVE ===========================


// instance d'objet serveur
// Objet serveur
#include "user_interface.h"
ESP8266WebServer   Serveur(SERVEUR_PORT);    // Serveur HTTP
DNSServer          dnsServer;
//HTTPClient       Client;        // Client HHTP (retours vers l'eedomus)
//WiFiClient       ClientWifi;    // needed by HTTPClient to use Wifi

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
    key = miniServerPtr->lastLocalIp;
  } else if ( key.equals(F("HOSTNAME")) ) {
    key = miniServerPtr->hostname;
  } else if ( key.equals(F("STATION_MAC")) ) {
    key = WiFi.macAddress();
  } else if (translateKeyPtr) {
    (*translateKeyPtr)(key);
  }
}

//pointer du gestionaire de submit
void (*onSubmitPtr)(const String &key) = NULL;

void onSubmit(String &key) {
  if (onSubmitPtr) (*onSubmitPtr)(key);
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
  if (onRepeatLinePtr) return (*onRepeatLinePtr)(num);
  return (false);
}


//void scan() {
//  const char item PROGMEM   = "<div><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='q {i}'>{r}%</span></div>";
//  if (scan) {
//    int n = WiFi.scanNetworks();
//    DEBUG_WM(F("Scan done"));
//    if (n == 0) {
//      DEBUG_WM(F("No networks found"));
//      page += F("No networks found. Refresh to scan again.");
//    } else {
//
//      //sort networks
//      int indices[n];
//      for (int i = 0; i < n; i++) {
//        indices[i] = i;
//      }
//
//      // RSSI SORT
//
//      // old sort
//      for (int i = 0; i < n; i++) {
//        for (int j = i + 1; j < n; j++) {
//          if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
//            std::swap(indices[i], indices[j]);
//          }
//        }
//      }
//
//      /*std::sort(indices, indices + n, [](const int & a, const int & b) -> bool
//        {
//        return WiFi.RSSI(a) > WiFi.RSSI(b);
//        });*/
//
//      // remove duplicates ( must be RSSI sorted )
//      if (_removeDuplicateAPs) {
//        String cssid;
//        for (int i = 0; i < n; i++) {
//          if (indices[i] == -1) continue;
//          cssid = WiFi.SSID(indices[i]);
//          for (int j = i + 1; j < n; j++) {
//            if (cssid == WiFi.SSID(indices[j])) {
//              DEBUG_WM("DUP AP: " + WiFi.SSID(indices[j]));
//              indices[j] = -1; // set dup aps to index -1
//            }
//          }
//        }
//      }
//
//      //display networks in page
//      for (int i = 0; i < n; i++) {
//        if (indices[i] == -1) continue; // skip dups
//        DEBUG_WM(WiFi.SSID(indices[i]));
//        DEBUG_WM(WiFi.RSSI(indices[i]));
//        int quality = getRSSIasQuality(WiFi.RSSI(indices[i]));
//
//        if (_minimumQuality == -1 || _minimumQuality < quality) {
//          String item = FPSTR(HTTP_ITEM);
//          String rssiQ;
//          rssiQ += quality;
//          item.replace("{v}", WiFi.SSID(indices[i]));
//          item.replace("{r}", rssiQ);
//          if (WiFi.encryptionType(indices[i]) != ENC_TYPE_NONE) {
//            item.replace("{i}", "l");
//          } else {
//            item.replace("{i}", "");
//          }
//          //DEBUG_WM(item);
//          page += item;
//          delay(0);
//        } else {
//          DEBUG_WM(F("Skipping due to quality"));
//        }
//
//      }
//      page += "<br/>";
//    }
//  }
//}


// ============   tools to handle captive portal  ============

bool softAPRequested = false;
bool softAP = false;            //Captive portal active
bool captiveDNS = false;               //Captive portal DNS active
bool rootWifiSetup = false;             //Specific for CaptivePortal WifiSetup hold in a sub repertory
unsigned long timerCaptivePortal;      //Used to timeout Captive portal
unsigned long timerCaptiveDNS;         //Used to reactive captive portal DNS
unsigned long timerWiFiStop = 0;       //Force a WiFi stop in 2,5 sec



// CaptiveDNS is active until a request to setupServer is done
void   captiveDNSStart() {
  Serial.println("Captive DNS ON");
  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  captiveDNS = true;
  //timerCaptivePortal = millis();
}


void captiveDNSStop() {
  Serial.println("Captive DNS OFF");
  dnsServer.stop();
  captiveDNS = false;
  // rearme le timerCaptiveDNS
  timerCaptiveDNS = millis();
}

void handleCaptivePortal() {
#define DELAY_NOVALIDPAGE 1U * 60 * 1000
#define CAPTIVEPORTAL_TIMEOUT  2U * 60 * 1000

  if (softAP && millis() - timerCaptivePortal > CAPTIVEPORTAL_TIMEOUT) {
    if (miniServerPtr) miniServerPtr->configureWiFi(false);  //Stop captive
    WiFi.begin();
    return;
  }
  // if no request arrived we rearm captive DNS
  if (!captiveDNS && millis() - timerCaptiveDNS > DELAY_NOVALIDPAGE) {
    captiveDNSStart();
  }
  if (captiveDNS) dnsServer.processNextRequest();
}

bool    newConfigTry = false;
bool    newConfigValide = false;
String  newConfigSSID;
String  newConfigPASS;
String  redirectUri;

// ============   tools to handle captive portal  (END) ============

// ============   tools to handle wifi mode =========




// ============   tools to handle wifi mode (END) =========

//// constructeur
MiniServeurWeb::MiniServeurWeb() {
  if (miniServerPtr != NULL) {
    Serial.print(F("Only one instance for MiniServeurWeb"));
    while (true) delay(100);
  }
  miniServerPtr = this;
}
// Destructor
MiniServeurWeb::~MiniServeurWeb() {
  if (miniServerPtr == NULL) return;
  this->end();
  miniServerPtr = NULL;
}




void MiniServeurWeb::handleClient() {
  static uint32_t timerDisconnected = 0;
  static uint32_t timerConnected = 0;

  if (softAP) handleCaptivePortal();

  // Analyse de l'etat du wifi pour detecter connecté ou deconnecté
  int status = WiFi.status();
  int static oldStatus = 0;
  if (status != oldStatus) {
    oldStatus = status;

    Serial.print(F("WIFI status = "));
    Serial.println(status);
    // on arme un timer pour avoir un peu de stabilité
    if (status == WL_CONNECTED) {
      timerConnected = millis();
      timerDisconnected = 0;
      timerWiFiStop = 0;
      _WiFiStatus = TWS_TRANSITION;  // transitional
    } else if (status == WL_DISCONNECTED) {
      timerDisconnected = millis();
      timerConnected = 0;
      timerWiFiStop = 0;
      _WiFiStatus = TWS_TRANSITION;  // transitional
    }
  }
  uint32_t now = millis();
  if (timerConnected && now - timerConnected > 2500) {
    timerConnected = 0;
    _WiFiStatus = TWS_STATION_CONNECTED; // = station connected
    Serial.print(F("WIFI Connected, IP address: "));
    lastLocalIp = WiFi.localIP().toString();
    Serial.println(lastLocalIp);
    Serial.print(F("host name "));
    Serial.println(hostname.c_str());
    Serial.print(F("Internal host name "));
    Serial.println(WiFi.hostname(hostname.c_str()));
    Serial.print(F("WIFI Mode "));
    Serial.println(WiFi.getMode());

    // rearme un serveur mDNS pour resoudre les hostname.local
    MDNS.end();
    if (!MDNS.begin(hostname.c_str())) {             // Start the mDNS responder for esp8266.local
      Serial.println(F("Error setting up MDNS responder!"));
    }  else {
      Serial.println(F("mDNS responder started"));
    }

  }
  if (timerDisconnected && now - timerDisconnected > 2000) {
    timerDisconnected = 0;
    Serial.println(F("WIFI diconnected "));
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

    _WiFiStatus = TWS_STATION_DISCONNECTED;
    if (WiFiMode == TWS_WIFI_SOFTAP  || WiFiMode == TWS_WIFI_SETUP_STATION ) {
      Serial.println(F("Disconnected but SoftAP on "));
      _WiFiStatus = TWS_TRANSITION;
      uint32_t badip = 0;
      if (WiFi.softAPIP().isSet()) {
        Serial.println(F("WIFI SoftAP ok "));
        _WiFiStatus = TWS_SOFTAP_ACTIVE;
        if (WiFiMode == TWS_WIFI_SETUP_STATION ) _WiFiStatus = TWS_SOFTAP_SETUP_STATION;
      }
    }
  }
  // set current wifi mode = to expected wifimode
  static tw_WiFiMode_t currentWiFiMode = WIFI_OFF;
  if (currentWiFiMode != WiFiMode ) {
    Serial.print(F("Set wifimode "));
    Serial.print(currentWiFiMode);
    Serial.print(F(" to "));
    Serial.println(WiFiMode);
    currentWiFiMode = WiFiMode;
    switch (WiFiMode) {
      case TWS_WIFI_OFF:
        MDNS.end();
        WiFi.mode(WIFI_OFF);
        delay(10);
        WiFi.forceSleepBegin();
        break;
      case TWS_WIFI_STATION:
        WiFi.mode(WIFI_STA);
        break;
      case TWS_WIFI_SOFTAP:
        WiFi.mode(WIFI_AP);
        break;
      case TWS_WIFI_SETUP_STATION:
        WiFi.mode(WIFI_AP);
        break;

    }
  }
  //    uint32_t now = millis();
  //    if (timerWiFiStop && now - timerWiFiStop > 2500) {
  //      timerWiFiStop = 0;
  //      MDNS.end();
  //      WiFi.mode(WIFI_OFF);
  //      delay(10);
  //      WiFi.forceSleepBegin();
  //    }
  //
  //
  //
  //
  //    if (softAPRequested) {
  //      _status = 0;
  //      newConfigTry = false;
  //      newConfigValide = false;
  //      Serial.print("Captive ON --> ");
  //      softAP = true;
  //      //      if (!persistent) timerCaptivePortal = millis();
  //      //      WiFi.persistent(persistent);
  //      //      WiFi.mode(WIFI_OFF);
  //      //      delay(500);
  //
  //      Serial.println("Captive ON --> config soft AP config");
  //      IPAddress local_IP(169, 254, 169, 254);
  //      IPAddress gateway(169, 254, 169, 254);
  //      IPAddress subnet(255, 255, 255, 0);
  //      bool result;
  //      result = WiFi.softAPConfig(local_IP, gateway, subnet);
  //      Serial.print("AP config : ");
  //      Serial.println(result);
  //      if (WiFi.softAPIP().isSet()) {
  //        _status = 3;
  //      }
  //
  //      // activavation du captive DNS
  //      captiveDNSStart();
  //
  //      Serial.println("Captive ON --> Start SOFT AP");
  //      result = WiFi.softAP(hostname.c_str(), NULL); // if password    8 >= len <= 62
  //      Serial.print("AP begin : ");
  //      Serial.println(result);
  //      delay(10);
  //      Serial.print("AP IP : ");
  //      Serial.println(WiFi.softAPIP());
  //      softAPRequested = false;
  //
  //    }
  //    if (newConfigTry) {
  //      Serial.print(F("New Config Ok "));
  //      newConfigTry = false;
  //      newConfigValide = true;
  //    }
  //    if (newConfigValide) {
  //      bool result = WiFi.begin(newConfigSSID, newConfigPASS);
  //      _status = 0;
  //      Serial.print(F("New Config Active "));
  //      delay(200);
  //      Serial.println(result);
  //      newConfigValide = false;
  //    }
  //
  //
  //  }
  MDNS.update();
  Serveur.handleClient();
}

tws_WiFiStatus_t MiniServeurWeb::getWiFiStatus() {
  return (_WiFiStatus);
}


// Mode STA
// todo  try connection and revalidate old credential if not ok
void MiniServeurWeb::connectWiFi(String ssid, String password, bool const tryConfig) {
  configureWiFi(false);  //Stop captive
  newConfigSSID = ssid;
  newConfigPASS = password;
  newConfigTry  = tryConfig;
  newConfigValide = !tryConfig;
  timerWiFiStop = millis();
  _WiFiStatus = TWS_TRANSITION;  // en transition
  delay(10);
  //  Serial.print("connectWiFi: ");
  //
  // // ETS_UART_INTR_DISABLE();
  // //   WiFi.disconnect();
  // // ETS_UART_INTR_ENABLE();
  // // delay(10);
  //  WiFi.persistent(true);  // just in case
  //  delay(10);
  //  WiFi.mode(WIFI_OFF);
  //
  //  int N = 0;
  //  while (WiFi.status() !=WL_DISCONNECTED && N++ < 50) {
  //    delay(100); Serial.print('X');
  //  }
  ////    wifi_station_disconnect();
  ////    wifi_set_opmode(NULL_MODE);
  ////    wifi_set_sleep_type(MODEM_SLEEP_T);
  ////    wifi_fpm_open();
  ////    #define FPM_SLEEP_MAX_TIME 0xFFFFFFF
  ////    wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);
  ////    delay(500);
  //
  //  //      WiFi.set_sleep_level(MAX_SLEEP_T) (SDK3)
  //  //     WiFi.set_listen_interval          (SDK3)
  //  //    WiFi.set_sleep_type               (all SDKs)
  //  Serial.print(F("Internal set host name1 "));
  //  Serial.println(hostname);
  //  WiFi.hostname(hostname);
  //  Serial.print(F("Internalset host name2 "));
  //  Serial.println(WiFi.hostname());
  // // delay(500);
  // // WiFi.mode(WIFI_STA);
  // // delay(500);
  //  bool result = WiFi.begin(ssid, password);
  //  delay(200);
  //  Serial.println(result);
  //  N = 0;
  //   while (WiFi.status() !=WL_CONNECTED && N++ < 50) {
  //    delay(100); Serial.print('Y');
  //  }
  //
  //  return result;
}

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




void MiniServeurWeb::configureWiFi(const bool active) {

  if (softAP == active) {
    return;
  }
  //  ETS_UART_INTR_DISABLE();
  //  WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
  //  ETS_UART_INTR_ENABLE();
  //  delay(10);

  if (softAP) {
    Serial.print("Captive OFF --> ");
    //   WiFi.persistent(true);  // just in case
    WiFi.softAPdisconnect (true);
    //WiFi.mode(WIFI_STA);
    softAP = false;
    rootWifiSetup = false;
    captiveDNSStop();

    Serial.println("Captive OFF --> END");
    return;
  }
  rootWifiSetup = true;
  Serial.print("Captive ON --> ");
  softAP = true;
  timerCaptivePortal = millis();
  WiFi.persistent(false);
  // disconnect sta, start ap
  //  ETS_UART_INTR_DISABLE();
  //  WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
  //  ETS_UART_INTR_ENABLE();
  WiFi.mode(WIFI_OFF);
  delay(100);


  Serial.println("Captive ON --> config soft AP config");

  IPAddress local_IP(169, 254, 169, 254);
  IPAddress gateway(169, 254, 169, 254);
  IPAddress subnet(255, 255, 255, 0);
  bool result;
  result = WiFi.softAPConfig(local_IP, gateway, subnet);
  Serial.print("AP config : ");
  Serial.println(result);

  // activavation du captive DNS
  captiveDNSStart();

  Serial.println("Captive ON --> Start SOFT AP");
  result = WiFi.softAP(hostname.c_str(), NULL);
  Serial.print("AP begin : ");
  Serial.println(result);
  delay(10);
  Serial.print("AP IP : ");
  Serial.println(WiFi.softAPIP());




  WiFi.persistent(true);
}




String MiniServeurWeb::getArg(const String argName) {
  return Serveur.arg(argName);
}


// Pages WEB
//#define LOCHex2Char(X) (X + (X <= 9 ? '0' : ('A' - 10)))
char LOCHex2Char( byte aByte) {
  return aByte + (aByte <= 9 ? '0' : 'A' -  10);
}


// looking for requested file into littleFS
void HTTPCallBack_HandleRequests() {

  Serial.print(F("WEB receved a "));
  Serial.print( String((Serveur.method() == HTTP_GET) ? "GET " : "POST " ));
  Serial.print(Serveur.uri());
  Serial.print(F(" from "));
  Serial.print(Serveur.client().remoteIP());
  Serial.println();
  // interception en mode captive
  Serial.print(F("hostHeader : "));
  Serial.print(Serveur.hostHeader());
  Serial.println();

  // in captive mode all requests to html or txt are re routed to "http://localip()" with a 302 reply
  if (softAP && !(Serveur.hostHeader().startsWith("169.254.169.254")) && (Serveur.uri().endsWith(".html") || Serveur.uri().endsWith("redirect") || Serveur.uri().endsWith(".txt")) ) {
    Serial.println(F("Request redirected to captive portal"));
    String aStr = F("http://");
    aStr += Serveur.client().localIP().toString();
    //   aStr += F("/APSetup/WifiManagement.html");
    Serveur.sendHeader("Location", aStr, true);
    //    Serveur.sendHeader("Location", String("http://") + Serveur.client().localIP().toString() + "/APSetup/WifiManagement.html", true);
    Serveur.send ( 302, "text/plain", "");
    Serveur.client().stop();
    return;
  }

  // specific for firefox to hide captive mode
  if (softAP && Serveur.uri().endsWith("generate_204") ) {
    Serial.println(F("Generate204"));
    Serveur.setContentLength(0);
    Serveur.send ( 204 );
    Serveur.client().stop();
    return;
  }

  // rearm timeout for captive portal
  // to hide captive mode stop DNS captive if a request is good (hostheader=localip)
  if (softAP) {
    timerCaptivePortal = millis();
    if (captiveDNS) captiveDNSStop();
  }


  // standard query are rooted to web folder
  String fileName = "/web";
  // captive query are rooted to web/wifisetup folder
  if (rootWifiSetup) fileName += "/wifisetup";
  fileName += Serveur.uri();

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
  // ================ SPECIFIC QUERY REFRESH  START =======================

  // query 'refresh' sended by 'miniServerWeb.js' to update class 'refresh' web items
  // if first arg is named 'refresh'  all itmes are translated with  onRefreshItem  call back
  // then they are sended back to client in a urlencoded string
  if (Serveur.args() > 0 && Serveur.argName(0) == "refresh") {
    // debug track
    Serial.print(F("WEB: Query refresh ("));
    if (Serveur.method() == HTTP_POST) {
      Serial.print(Serveur.arg(Serveur.args() - 1).length());
      Serial.print(F(") "));
      Serial.println(Serveur.arg(Serveur.args() - 1)); // try to get 'plain'
    } else {
      Serial.print(Serveur.arg(0).length());
      Serial.print(F(") "));
      Serial.println(Serveur.arg(0)); // try to get 'plain'
    }

    // traitement des chaines par le Sketch
    String answer;
    answer.reserve(1000);   // should be max answer
    String aKey;
    aKey.reserve(500);      // should be max for 1 value
    String aKeyName;
    aKeyName.reserve(50);
    for (uint8_t N = 1; N < Serveur.args(); N++) {
      //     Serial.print(F("WEB: refresh "));
      //     Serial.print(Serveur.argName(N));
      //     Serial.print(F("="));
      //      Serial.println(Serveur.arg(N));
      aKeyName = Serveur.argName(N);
      aKey = Serveur.arg(N);
      if ( !aKeyName.equals("refresh") && !aKeyName.equals("plain") && onRefreshItem(aKeyName, aKey) ) {
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

    Serveur.sendHeader("Cache-Control", "no-cache");
    Serveur.setContentLength(answer.length());
    Serveur.send(200, fileMIME.c_str(), answer);
    //    Serveur.client().stop();
    //    Serial.print(answer.length());
    //    Serial.println(F(" car."));

    return;

  }
  // ================ QUERY REFRESH  END =======================



  // if any arg is named 'submit' ==> send submit to call back onSubmit
  redirectUri = "";
  for (uint8_t i = 0; i < Serveur.args(); i++) {
    if ( Serveur.argName(i).equals(F("submit")) ) {

      Serial.print(F("WEB: Submit "));
      Serial.println(Serveur.arg(i));
      String value = Serveur.arg(i);
      onSubmit(value);   //appel du callback
    }
  }
  // if call back onSubmit want we redirect
  if (redirectUri.length() > 0) {
    Serial.print(F("WEB redirect "));
    Serial.println(redirectUri);
    //    String aStr = F("http://");
    //    aStr += Serveur.client().localIP().toString();
    String aStr = redirectUri;
    redirectUri = "";
    Serveur.sendHeader("Location", aStr, true);
    Serveur.send ( 302, "text/plain", "");
    Serveur.client().stop();
    return;
  }

  File aFile;
  if (fileType != NONE) {
    aFile = LittleFS.open(fileName, "r");
  }
  bool doChunk = false;
  if (aFile) {
    if (fileType == HTML) {
      Serveur.sendHeader("Cache-Control", "no-cache");
      Serveur.setContentLength(CONTENT_LENGTH_UNKNOWN);
      doChunk = true;
    } else {
      Serveur.sendHeader("Cache-Control", "max-age=10800, public");
      Serveur.setContentLength(aFile.size()); //CONTENT_LENGTH_UNKNOWN
    }
    Serveur.send(200, fileMIME.c_str());
    aFile.setTimeout(0);
    static    char aBuffer[1025];               // static dont overload heap
    static    char repeatBuffer[1025];          // static dont overload heap
    int  repeatNumber;
    bool repeatActive = false;
    // tant que tout n'est pas lut
    while (aFile.available()) {

      int size;
      if (!doChunk) {
        size = aFile.readBytes( aBuffer, 1024 );
      } else {
        if (!repeatActive) {
          size = aFile.readBytesUntil( '\n', aBuffer, 1000 );
          if (size < 1000) aBuffer[size++] = '\n'; //!!! on exactly 1000 bytes lines the '\n' will be lost :)
          aBuffer[size] = 0x00; // make aBuffer a Cstring
          // Gestion du [# REPEAT #]
          // si une ligne commence par [#repeat] elle sera repeté et tant que OnRepat Call bask retourne true
          if (strncmp( "[#REPEAT_LINE#]", aBuffer, 15) == 0) {
            Serial.println(F("Repeat line detected"));
            // copie de la chaine dans le buffer de repeat
            strcpy(repeatBuffer, aBuffer + 15);
            repeatActive = true;
            repeatNumber = 0;
          }
        }
        if (repeatActive) {
          aBuffer[0] = 0x00;
          repeatActive = onRepeatLine(repeatNumber++);
          if ( repeatActive ) {
            strcpy(aBuffer, repeatBuffer);
          }
          size = strlen(aBuffer);
        }

        char* currentPtr = aBuffer;
        // decoupage de la ligne pour gerer les "[# xxxxx #]"

        while ( currentPtr = strstr(currentPtr, "[#") ) {  // il y a au moins un start
          char* startPtr = currentPtr + 2;
          char* stopPtr = strstr( startPtr + 1, "#]" ); // au moins un char entre [# et #]
          int len = stopPtr - startPtr;
          if (  !stopPtr || len <= 0 || len >= 40 ) { // pas de stop ou longeur incorrect
            break;
          }

          // recuperation du mot clef
          char aKey[41];
          strncpy(aKey, startPtr, len);
          aKey[len] = 0x00;   // aKey is Cstring
          //   Serial.print("Key=");
          //   Serial.println(aKey);

          String aStr;
          aStr.reserve(100);
          aStr = aKey;
          aStr.trim();
          translateKey(aStr);

          // Copie de la suite de la chaine ailleur
          static  char bBuffer[500];
          strncpy(bBuffer, stopPtr + 2, 500);

          // Ajout de la chaine de remplacement
          strncpy(currentPtr, aStr.c_str(), 100);
          currentPtr += aStr.length();
          // Ajoute la suite
          strncpy(currentPtr, bBuffer, 500);
          size = strlen(aBuffer);


        }// while
      } // else do chunk

      Serial.print('.');
      if (size) Serveur.sendContent_P(aBuffer, size);
    }  // if avail
    if (doChunk) Serveur.chunkedResponseFinalize();
    Serial.println("<");
    Serveur.client().stop();
    aFile.close();
    return;
  }
  //  }
  Serial.println("error 404");
  String message = F("File Not Found\n");
  message += "URI: ";
  message += Serveur.uri();
  message += F("\nMethod: ");
  message += (Serveur.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += Serveur.args(); // last is plain = all arg
  message += F("\n<br>");
  for (uint8_t i = 0; i < Serveur.args(); i++) {
    message += " " + Serveur.argName(i) + ": " + Serveur.arg(i) + "\n<br>";
  }
  Serial.println(message);
  message += "<H2><a href=\"/\">go home</a></H2><br>";
  Serveur.send(404, "text/html", message);
  Serveur.client().stop();
}








void MiniServeurWeb::setCallBack_TranslateKey(void (*translatekey)(String & key))  {
  translateKeyPtr =  translatekey;
}

void MiniServeurWeb::setCallBack_OnSubmit(void (*onsubmit)(const String & key))  {
  onSubmitPtr =  onsubmit;
}

void MiniServeurWeb::setCallBack_OnRefreshItem(bool (*onrefreshitem)(const String & keyname, String & key)) {
  onRefreshItemPtr = onrefreshitem;
}


void MiniServeurWeb::setCallBack_OnRepeatLine(bool (*onrepeatline)(const int num)) {     // call back pour gerer les Repeat
  onRepeatLinePtr = onrepeatline;
}


String MiniServeurWeb::currentUri() {
  return Serveur.uri();
}


void MiniServeurWeb::redirectTo(String const uri) {
  redirectUri = uri;
}


void MiniServeurWeb::end() {
  WiFi.mode(WIFI_OFF);
  softAP = false;
  delay(100);
  WiFi.forceSleepBegin();
  delay(100);
  Serveur.close();
  delay(100);
}


// initialise le serveur WEB
// de fait on suppose que le WiFi interne de l'ESP est deja configuré
// et qu'il se connectera tout seul
// return false si le WiFi interne de l'ESP n'est pas en mode AP
// la config se fera par l'utilisateur via


void MiniServeurWeb::begin() {

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
      _WifiMode = WIFI_STA;
    case WIFI_AP:
      _WifiMode = WIFI_AP;
    default   // dans les autres cas on force un OFF
        _WifiMode = WIFI_OFF;
      WiFi.mode(WIFI_OFF);
      delay(100);
      WiFi.forceSleepBegin();
      delay(100);
  }
  // le hostname est SSID du mode AP il servira aussi pour le nom DNS en mode station
  _hostname=WiFi.softAPSSID();
  // Check a valid hostname
  // configation du nom du reseau AP : LITTLEWEB_XXYY  avec les 2 dernier chifre hexa de la mac adresse
  if (_hostname.length() > 30) _hostname.remove(30);   // chop at 30char
  _hostname.trim();
  if (_hostname.length() < 4 ) _hostname = F(MWEB_DEFAULT_SSID "*");
  if (_hostname.endsWith(F("*"))) {
    _hostname.remove(hostname.length() - 1);
    _hostname += WiFi.macAddress().substring(12, 14);
    _hostname += WiFi.macAddress().substring(15, 17);
  }
  _hostname.replace(' ', '_');
  Serial.print("Hostname:");
  Serial.println(_hostname);
  // FS
  if (!LittleFS.begin()) {
    Serial.println("FS en erreur");
  } else {
    Serial.println("FS Ok");
  }

  //  if (WiFi.) {
  //    WiFi.mode(WIFI_OFF);
  //    softAPRequested = true;
  //  }


  //  Serial.println(F("Set WIFI ON"));
  //  WiFi.begin();
  //  delay(1000);
  // mise en place des call back web
  //Serveur.on(F("/"), HTTPCallBack_display);                        // affichage de l'etat du SONOFF
  Serveur.onNotFound(HTTPCallBack_HandleRequests);


  Serveur.begin();
  Serial.setDebugOutput(debugLevel >= 3);
  Serial.print("Hostname : ");
  Serial.println(hostname);
  delay(100);



  return ;
}
