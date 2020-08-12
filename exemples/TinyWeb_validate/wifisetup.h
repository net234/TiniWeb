/* Gestion des pages wifisetup
 *  (C) 07/2020 P.HENRY NET234 
 */ 


 #define wifisetup_on
 #include <ESP8266WiFi.h>        // base d'acces au WIFI

 // Scan les reseaux wifi le fill up des page comportant un arg tinyweb=show_wifi
 #define MAXNETWORK 30
int network[MAXNETWORK + 1];
int networkSize = 0;
int currentLine = 0;

// call back pour sort network
int compareNetwork (const void * a, const void * b) {
  return ( WiFi.RSSI(*(int*)b) - WiFi.RSSI(*(int*)a) );  // this is why I dont like C
}

// Fill up network array with current network
void scanNetwork() {
  networkSize = min(MAXNETWORK, (int)WiFi.scanNetworks());
  //Serial.print(F("scanNetworks done"));
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

#define RSSIdbToPercent(x) ( 2 * (WiFi.RSSI(x) + 100) )
