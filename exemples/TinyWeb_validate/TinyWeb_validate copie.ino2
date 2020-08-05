// src test file pour valider MiniWebServeur
// (C) V1.0.1 6/6/2020  NET234 P.HENRY
#include <Arduino.h>
#include <ESP8266WiFi.h>        // base d'acces au WIFI

#define LED_LIFE      LED_BUILTIN
#define APP_VERSION   "src-MiniServeurWeb V1.0.1"

#define LED_ON        LOW
//Objet serveur WEB
#include  "MiniServeurWeb.h"
MiniServeurWeb    ServeurWeb;


String  displayResultat = "Bonjour éè";
String  formPrenom;
String  formDate;
String  formDuree;

//  const char item PROGMEM   = "<div><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='q {i}'>{r}%</span></div>";
//String item = FPSTR(HTTP_ITEM);

//===============   Gestion des demande WEB ==========================
#define MAXNETWORK 30
int network[MAXNETWORK + 1];
int networkSize = 0;


int compareNetwork (const void * a, const void * b) {
  return ( WiFi.RSSI(*(int*)b) - WiFi.RSSI(*(int*)a) );  // this is why I dont like C
}


void scanNetwork() {
  networkSize = min(MAXNETWORK, (int)WiFi.scanNetworks());
  Serial.print(F("scanNetworks done"));
  if (networkSize == 0) return;

  //sort networks
  for (int i = 0; i < networkSize; i++)  network[i] = i;
  qsort(network, networkSize, sizeof(network[0]), compareNetwork);
  // remove duplicates SSID
  for (int i = 0; i < networkSize - 1; i++) {
    for (int j = i + 1; j < networkSize; j++) {
      if ( WiFi.SSID(network[i]) == WiFi.SSID(network[j]) )  {
        networkSize--;
        for (int k = j; k < networkSize; k++) network[k] = network[k + 1];
        j--;
      }
    }
  }
}

//===============
int currentLine = 0;

//Gestionaire des repetition de ligne #REPEAT_LINE#  0 = first line
#define RSSIdbToPercent(x) ( 2 * (WiFi.RSSI(x) + 100) )

bool on_RepeatLine(const int num) {

  Serial.print(F("REPEAT_LINE "));
  Serial.println(num);
  currentLine = num;
  if ( ServeurWeb.getArg(F("wifi")).equals(F("1")) ) {
    if (currentLine == 0) scanNetwork();
    return (num < networkSize && RSSIdbToPercent(network[num]) > 25 );
  } else {
    return (false);
  }
}


//Donnée demandée par les pages web
void traductionKey(String & key) {
  char aBuffer[21];
  if ( key.equals(F("APP_VERSION")) ) {
    key = APP_VERSION;
  } else if ( key.equals(F("SSID_NAME")) ) {
    key = WiFi.SSID(network[currentLine]);
  } else if ( key.equals(F("SSID_LEVEL")) ) {
    int level = RSSIdbToPercent(network[currentLine]);
    if (level > 100) level = 100;
    key = level;
  } else if ( key.equals(F("SSID_LOCK")) ) {
    key = "&nbsp;";
    if (WiFi.encryptionType(network[currentLine]) != ENC_TYPE_NONE) {
      key = "&#128274;";
    }
  } else if ( key.equals(F("RESULTAT")) ) {
    key = displayResultat;
  } else if ( key.equals(F("PRENOM")) ) {
    key = formPrenom;
  } else if ( key.equals(F("DATE")) ) {
    key = formDate;
  } else if ( key.equals(F("DUREE")) ) {
    key = formDuree;
    //    Serial.print(F("DisplayResultat="));
    //    Serial.println(key);
  } else {
    Serial.print(F("WEB MISSKEY='"));
    Serial.print(key);
    Serial.println(F("'"));
    key += "???";
  }
}




//Donnée de refraichissement demandée par les pages web
bool on_RefreshItem(const String & keyname, String & key) {
  Serial.print(F("Got refresh "));
  Serial.print(keyname);
  Serial.print(F("="));
  Serial.println(key);

  if ( keyname.equals("CLOCK") ) {
    long now = millis() / 1000;
    String akey = String((now / 60) % 60) + ":" + String(now % 60);
    if (akey != key) {
      key = akey;
      return (true);
    }
    return (false);
  }
  if ( keyname.equals("RESULTAT") ) {
    if (displayResultat != key) {
      key = displayResultat;
      return (true);
    }
    return (false);
  }
  if ( keyname.equals("refreshTimeout") ) {
    if (key.toInt() != 5000) {
      key = "5000";
      return (true);
    }
    return (false);
  }

}


bool debugStat = true;
bool persistentStat = true;
int  listenInterval = 0;
bool resetRequested = false;
String wifiSSD;
String wifiPASS;

void webSubmit(const String & key) {
  Serial.print(F("submit='"));
  Serial.print(key);
  Serial.print(F(" uri='"));
  Serial.print(ServeurWeb.currentUri());
  Serial.println(F("'"));


  // Page Reset de la config WiFi
  if (key.equals(F("reset")) ) {
    Serial.println(F("Reset requested"));
    resetRequested = true;
  }



  // Page de configuration
  if (ServeurWeb.currentUri().endsWith(F("/configure.html")) && key.equals(F("SAVE")) ) {
    String aString = ServeurWeb.getArg(F("SSID"));
    aString.trim();
    if (aString.length() >= 2 && aString.length() <= 32) {
      wifiSSD = aString;
      wifiPASS = ServeurWeb.getArg(F("PASS"));

      aString = ServeurWeb.getArg(F("HOSTNAME"));
      aString.trim();
      Serial.print(F("Got Hostname="));
      Serial.print(aString);
      if (!aString.equals(ServeurWeb.hostname)) {
        ServeurWeb.hostname = aString;
        File aFile = LittleFS.open(F("config.txt"), "w");
        if (aFile) {
          aFile.print(F("HOSTNAME="));
          aFile.println(aString);
          Serial.print(F("saved Hostname="));
          Serial.print(aString);
          aFile.close();
        }
      }
      delay(100);
      ServeurWeb.connectWiFi(wifiSSD, wifiPASS);
      Serial.print(F("request connectWiFi="));
      Serial.println(wifiSSD);
      ServeurWeb.redirectTo(F("wait.html"));
    }
    return;
  }



  if ( !key.equals(F("WRITE")) ) {
    displayResultat = "submit INVALIDE";
    return;
  }
  formPrenom = ServeurWeb.getArg(F("PRENOM"));
  Serial.print(F("Prenom='"));
  Serial.println(formPrenom);
  formDate = ServeurWeb.getArg(F("DATE"));
  Serial.print(F("Date='"));
  Serial.println(formDate);
  formDuree = ServeurWeb.getArg(F("DUREE"));
  Serial.print(F("Duree='"));
  Serial.println(formDuree);
  displayResultat = "Entrée Valides " + formPrenom;
}


//===============   Gestion des demande WEB (FIN) ==========================



void setup() {

  // Initialisation Hard des IO
  pinMode(LED_LIFE, OUTPUT);
  // init Serial
  Serial.begin(115200);
  Serial.println(F(APP_VERSION));

  Serial.setDebugOutput(debugStat);

  // recuperation du nom de la device
  LittleFS.begin();
  String aString;
  File aFile = LittleFS.open(F("config.txt"), "r");
  if (aFile) {
    aString = aFile.readStringUntil( '\n' );
    Serial.println(aString);
    if (aString.startsWith(F("HOSTNAME="))) {
      aString.remove(0, 9);
      ServeurWeb.hostname = aString;
    }
    aFile.close();
  }
  //Serveur WEB
  ServeurWeb.WiFiMode = TWS_WIFI_STATION;
  ServeurWeb.setCallBack_TranslateKey(&traductionKey);
  ServeurWeb.setCallBack_OnSubmit(&webSubmit);
  ServeurWeb.setCallBack_OnRefreshItem(&on_RefreshItem);
  ServeurWeb.setCallBack_OnRepeatLine(&on_RepeatLine);
  ServeurWeb.begin();
}


int wifiStatus = 99;


void loop() {
  // run AP mode on BP0
  if (digitalRead(0) == LOW) {
    ServeurWeb.configureWiFi(true);
  }
  ServeurWeb.handleClient ();     // handle http service

  //Check ServeurWeb Status
  static long timer = millis();
  if (millis() - timer > 1000) {
    timer += 1000;
    if (resetRequested) {
      delay(1000);
      ESP.reset();
      while (1) {};
    }
    int result = ServeurWeb.getWiFiStatus();
    if (result != wifiStatus) {
      wifiStatus = result;

      //, , TWS_STATION_CONNECTED, TWS_STATION_DISCONNECTED, TWS_SOFTAP_ACTIVE, TWS_WIFI_SOFTAP_SETUP_STATION
      //    0 = working;
      //    1 = station unconnected
      //    2 = station connected
      //    3 = portal  active
      //    3 = captive portal (setup wifi)
      //   99 = wifi off
      switch (wifiStatus) {
        case TWS_TRANSITION:
          Serial.println(F("TWS wifi en transition"));
          break;
        case TWS_STATION_OFF:
          Serial.println(F("TSW wifi off"));
          digitalWrite(LED_LIFE, !LED_ON);
          break;
        case TWS_STATION_DISCONNECTED:
          Serial.println(F("TSW wifi Deconnecte"));
          digitalWrite(LED_LIFE, !LED_ON);
          break;
        case TWS_STATION_CONNECTED:
          digitalWrite(LED_LIFE, LED_ON);
          Serial.println("TSW wifi station Connected");
          break;
        case TWS_SOFTAP_ACTIVE:
          Serial.println(F("TSW softAP actif"));
          break;
        case TWS_SOFTAP_SETUP_STATION:
          Serial.println(F("TSW softAP wifi setup"));
          break;
        default:
          Serial.print(F("TSW Status:"));
          Serial.println(wifiStatus);
      } //switch
    }// if status changed
  }
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
      case 'E':
        persistentStat = !persistentStat;
        Serial.print("persistent ");
        Serial.println(persistentStat);
        Serial.setDebugOutput(persistentStat);
        break;
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



      case 'G':
        debugStat = !debugStat;
        Serial.print("Debug ");
        Serial.println(debugStat);
        Serial.setDebugOutput(debugStat);
        break;

      case 'L':
        listenInterval = (listenInterval + 1) % 11;
        Serial.print("Light listenInterval ");
        Serial.println(listenInterval);

        WiFi.setSleepMode (WIFI_LIGHT_SLEEP, listenInterval);
        break;
      case 'M':
        listenInterval = (listenInterval + 1) % 11;
        Serial.print("Modem listenInterval ");
        Serial.println(listenInterval);

        WiFi.setSleepMode (WIFI_MODEM_SLEEP, listenInterval);
        break;

      case '1':
        Serial.println("configureWiFi(false)");
        ServeurWeb.configureWiFi(false);
        break;
      case '2':
        Serial.println("configureWiFi(true)");
        ServeurWeb.configureWiFi(true);
        break;
      case '3':
        Serial.println("setmode Wifi Off");
        ServeurWeb.WiFiMode = TWS_WIFI_OFF;
        break;
      case '4':
        Serial.println("setmode wifi station");
        ServeurWeb.WiFiMode = TWS_WIFI_STATION;
        break;
      case '5':
        Serial.println("setmode wifi softap");
        ServeurWeb.WiFiMode = TWS_WIFI_SOFTAP;
        break;
      case '6':
        Serial.println("setmode wifi setup station station");
        ServeurWeb.WiFiMode = TWS_WIFI_SETUP_STATION;
        break;
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

    //bool setSleepMode (WiFiSleepType_t type, int listenInterval=0)

    //Sleep mode type is WIFI_NONE_SLEEP, WIFI_LIGHT_SLEEP or WIFI_MODEM_SLEEP.

    //  int32_t  channel (void)
    //  WiFiSleepType_t  getSleepMode ()
    //  bool  setPhyMode (WiFiPhyMode_t mode)
    //  WiFiPhyMode_t  getPhyMode ()
    //  void  setOutputPower (float dBm)
    //  WiFiMode_t  getMode ()
    //  bool  enableSTA (bool enable)
    //  bool  enableAP (bool enable)
    //  bool  forceSleepBegin (uint32 sleepUs=0)
    //  bool  forceSleepWake ()
    //  int  hostByName (const char *aHostname, IPAddress &aResult)
    //
    //  appeared with SDK pre-V3:
    //  uint8_t getListenInterval ();
    //  bool isSleepLevelMax ();


    // pour l'ESP le sleep mode est activé par un delay

    //ESP.deepSleep(5000000);
    //   esp_sleep_enable_timer_wakeup(5000000); //5 seconds
    //   yield();


  }
  delay(10);
}// loop
