#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <string>

#include "NetworkHelper.h"

//#define DONT_REMEMBER
#define DEBUG

#define RST_BTN_PIN   0

//Access point configuration
IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

struct ConnectionInfo
{
  char SSID[32];
  char password[32];

  uint16_t checksum;
};

const char* sHelperNetworkServerName = "networkhelper"; //Name for the DNS
const char* sHelperNetworkSSID = "esp8266"; //AP SSID
const char* sHelperNetworkPassword = ""; //AP Password
//If the password field is empty, no password is used

bool bConnectedToAP = false;

ConnectionInfo savedConnectionInfo;
NetworkHelper helper(sHelperNetworkServerName);

void UpdateConnectionInfo(const char* ssid, const char* password)
{
#ifndef DONT_REMEMBER
#ifdef DEBUG
  Serial.println(__FUNCTION__);
#endif

  strcpy(savedConnectionInfo.SSID, ssid);
  strcpy(savedConnectionInfo.password, password);
  savedConnectionInfo.checksum = CalcConnectionInfoChecksum(&savedConnectionInfo);

  EEPROM.put(0, savedConnectionInfo);

#ifdef DEBUG
  Serial.print("Checksum: "); Serial.println(savedConnectionInfo.checksum);
#endif

#endif
}

bool isSavedInfoValid(ConnectionInfo* info)
{
  uint16_t checksum = CalcConnectionInfoChecksum(info);

  return (checksum == info->checksum);
}

uint16_t CalcConnectionInfoChecksum(ConnectionInfo* info)
{
  uint16_t checksum = 0;

  for (uint16_t i = 0; i < sizeof(ConnectionInfo) - sizeof(checksum); i++)
    checksum += ((char*)info)[i];

  return checksum;
}

void Cleanup()
{
  if (bConnectedToAP)
    WiFi.disconnect();
  else
    WiFi.softAPdisconnect (true);

  //Let the EEPROM commit to flash
  delay(100);
}

void SoftReset()
{
  //Wait for anything to finish with a delay
  delay(1000);
  //Shutdown the server and stop the access point
  Cleanup();

#ifdef DEBUG
  Serial.println(__FUNCTION__);
#endif

  //Busy loop
  while (1);
  //Caught by the watchdog
  //The first reset after uploading code doesn't work
  //Everyone afterwards does
}

void ResetConnectionInfo(ConnectionInfo* info)
{
  memset(info, 0, sizeof(ConnectionInfo));
  EEPROM.put(0, *info);
}

void setup()
{
  delay(1000);

  EEPROM.begin(96);

#ifdef DEBUG
  Serial.begin(115200);
  Serial.println();
#endif

  //Recover connection info
  EEPROM.get(0, savedConnectionInfo);
  WiFi.persistent(false);

#ifdef RST_BTN_PIN
  pinMode(RST_BTN_PIN, INPUT_PULLUP);
#endif

#ifndef DONT_REMEMBER
  if (isSavedInfoValid(&savedConnectionInfo) &&
      strlen(savedConnectionInfo.SSID))
  {
#ifdef DEBUG
    Serial.println("Saved info checksum matches");
    Serial.println("Connecting to...");
    Serial.print("SSID: "); Serial.println(savedConnectionInfo.SSID);
    Serial.print("Password: "); Serial.println(savedConnectionInfo.password);
#endif
    WiFi.mode(WIFI_STA);

    if (strlen(savedConnectionInfo.password))
      WiFi.begin(savedConnectionInfo.SSID, savedConnectionInfo.password);
    else
      WiFi.begin(savedConnectionInfo.SSID);

    uint32_t connectionAttemptStart = millis();
    while (WiFi.status() != WL_CONNECTED &&
           (millis() - connectionAttemptStart) < 10000)
    {
      delay(500);
#ifdef DEBUG
      Serial.print(".");
#endif
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      bConnectedToAP = true;
#ifdef DEBUG
      Serial.println("Connected");
      Serial.print("IP address: "); Serial.println(WiFi.localIP());
#endif
    }
  }
#ifdef DEBUG
  else
  {
    Serial.println("Connection info checksum doesn't match OR SSID not valid");
  }
#endif
#endif

  if (!bConnectedToAP)
  {
#ifdef DEBUG
    Serial.println("Creating AP");
    Serial.print("SSID: ");
    Serial.println(sHelperNetworkSSID);
#endif

#ifndef DONT_REMEMBER
    ResetConnectionInfo(&savedConnectionInfo);
    ESP.rtcUserMemoryWrite(0, (uint32_t*) &savedConnectionInfo, sizeof(ConnectionInfo));
#endif

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    if (strlen(sHelperNetworkPassword))
    {
#ifdef DEBUG
      Serial.print("Password: ");
      Serial.println(sHelperNetworkPassword);
#endif
      WiFi.softAP(sHelperNetworkSSID, sHelperNetworkPassword);
    }
    else
    {
      WiFi.softAP(sHelperNetworkSSID);
    }
  }

  helper.start();
}

void loop()
{
#ifdef RST_BTN_PIN
  if (strlen(savedConnectionInfo.SSID) && !digitalRead(RST_BTN_PIN))
  {
    ResetConnectionInfo(&savedConnectionInfo);
#ifdef DEBUG
    Serial.println("Reset button");
#endif
    SoftReset();
  }
#endif

  helper.background();
}
