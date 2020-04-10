#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <string>
#include "NetworkHelper.h"

/*Use the DONT_REMEMBER flag to not recover infomration on startup,
  and not to store any changes to the network configuration*/
//#define DONT_REMEMBER
#define WIPE_ON_INVALID

//Access point configuration
IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

#define MAX_MQTT_PATH_LENGTH      32
#define MAX_MQTT_PUBLICATIONS     4
#define MAX_MQTT_SUBSCRIPTIONS    4

//Structure for saving the connection info to be recovred on startup
struct ConnectionInfo
{
  char SSID[32];
  char password[32];

  char mqttAddress[MAX_MQTT_PATH_LENGTH];
  uint16_t mqttPort;
  char mqttUser[MAX_MQTT_PATH_LENGTH];
  char mqttPass[MAX_MQTT_PATH_LENGTH];

  char subscriptions[MAX_MQTT_SUBSCRIPTIONS][MAX_MQTT_PATH_LENGTH];
  char publications[MAX_MQTT_PUBLICATIONS][MAX_MQTT_PATH_LENGTH];

  uint16_t checksum;
};

const char* sHelperNetworkServerName = "networkhelper"; //Name for the DNS
const char* sHelperNetworkSSID = "esp8266"; //AP SSID
const char* sHelperNetworkPassword = ""; //AP Password
//If the password field is empty, no password is used

bool bConnectedToAP = false;

char* pubAlias[4] =
{
  "LIGHT_1",
  "LIGHT_2",
  "SWITCH_1",
  "SWITCH_2"
};

/*Create a wrapper for the pub/sub info*/
/*This is needed because the helper expects an array of pointers,
not a generic block of memory*/
char* pubWrapper[MAX_MQTT_PUBLICATIONS];

char* subAlias[4] =
{
  "TEMPERATURE",
  "LIGHT",
  "HUMIDITY",
  "OCCUPANT"
};

char* subWrapper[MAX_MQTT_SUBSCRIPTIONS];

ConnectionInfo savedConnectionInfo;
NetworkHelper helper(sHelperNetworkServerName, (char**)pubWrapper, (char**)pubAlias, 4,
                     (char**)subWrapper, (char**)subAlias, 4);

void UpdateConnectionInfo(const char* ssid, const char* password)
{
  strcpy(savedConnectionInfo.SSID, ssid);
  strcpy(savedConnectionInfo.password, password);
  savedConnectionInfo.checksum = CalcConnectionInfoChecksum(&savedConnectionInfo);

  EEPROM.put(0, savedConnectionInfo);
}

void UpdateMQTTServerInfo(const char* sAddr, uint16_t nPort, const char* sUser, const char* sPass)
{
  strcpy(savedConnectionInfo.mqttAddress, sAddr);
  savedConnectionInfo.mqttPort = nPort;
  strcpy(savedConnectionInfo.mqttUser, sUser);
  strcpy(savedConnectionInfo.mqttPass, sPass);

  savedConnectionInfo.checksum = CalcConnectionInfoChecksum(&savedConnectionInfo);

  EEPROM.put(0, savedConnectionInfo);
}

void UpdateMQTTSubInfo(uint8_t nIndex, const char* sSubName)
{
  strcpy(savedConnectionInfo.subscriptions[nIndex], sSubName);

  savedConnectionInfo.checksum = CalcConnectionInfoChecksum(&savedConnectionInfo);

  EEPROM.put(0, savedConnectionInfo);
}

void UpdateMQTTPubInfo(uint8_t nIndex, const char* sPubName)
{
  strcpy(savedConnectionInfo.publications[nIndex], sPubName);

  savedConnectionInfo.checksum = CalcConnectionInfoChecksum(&savedConnectionInfo);

  EEPROM.put(0, savedConnectionInfo);
}

uint16_t CalcConnectionInfoChecksum(ConnectionInfo* info)
{
  uint16_t checksum = 0;

  for (uint16_t i = 0; i < sizeof(ConnectionInfo) - sizeof(checksum); i++)
    checksum += ((char*)info)[i];

  return checksum;
}

bool isSavedInfoValid(ConnectionInfo* info)
{
  //Verify that the stored checksum matches the data
  uint16_t checksum = CalcConnectionInfoChecksum(info);

  return (checksum == info->checksum);
}

void Cleanup()
{
  //Stop the network helper
  helper.stop();

  if (bConnectedToAP)
    //Disconnect from the current access point
    WiFi.disconnect();
  else
    //Or stop the current access point
    WiFi.softAPdisconnect (true);

  EEPROM.end();

  //Let the EEPROM commit to flash
  delay(100);
}

void SoftReset()
{
  //Wait for anything to finish with a delay
  delay(1000);
  //Shutdown the server and stop the access point
  Cleanup();

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

  EEPROM.begin(sizeof(ConnectionInfo));

  Serial.begin(115200);
  Serial.println();

  //Recover connection info
  EEPROM.get(0, savedConnectionInfo);
  WiFi.persistent(false);

#ifndef DONT_REMEMBER
  if (isSavedInfoValid(&savedConnectionInfo) &&
      strlen(savedConnectionInfo.SSID))
  {
    Serial.println("Saved info checksum matches");
    Serial.println("Connecting to...");
    Serial.print("SSID: "); Serial.println(savedConnectionInfo.SSID);
    Serial.print("Password: "); Serial.println(savedConnectionInfo.password);

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
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      bConnectedToAP = true;

      Serial.println("Connected");
      Serial.print("IP address: "); Serial.println(WiFi.localIP());
    }
  }
  else
  {
    Serial.println("Connection info checksum doesn't match OR SSID not valid");

#ifdef WIPE_ON_INVALID
    Serial.println("Resetting connection info");
    memset(&savedConnectionInfo, 0, sizeof(ConnectionInfo));
#endif
  }
#endif

  /*Update the wrappes with string pointers*/
  for(size_t i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++)
    subWrapper[i] = savedConnectionInfo.subscriptions[i];

  for(size_t i = 0; i < MAX_MQTT_PUBLICATIONS; i++)
    pubWrapper[i] = savedConnectionInfo.publications[i];

  if (!bConnectedToAP)
  {
    Serial.println("Creating AP");
    Serial.print("SSID: ");
    Serial.println(sHelperNetworkSSID);

#ifndef DONT_REMEMBER
    ResetConnectionInfo(&savedConnectionInfo);
#endif

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    if (strlen(sHelperNetworkPassword)) //Password
    {
      Serial.print("Password: ");
      Serial.println(sHelperNetworkPassword);

      WiFi.softAP(sHelperNetworkSSID, sHelperNetworkPassword);
    }
    else //No password
      WiFi.softAP(sHelperNetworkSSID);
  }

  //Setup lambda function to handle network change from helper
  helper.onNetworkChange(
    [](String ssid, String password)
  {
    //Update the connection info in EEPROM
    UpdateConnectionInfo(ssid.c_str(), password.c_str());
    //Use the watchdog to reset the device
    SoftReset();
  });

  helper.onServerChange(
    [](String ip, uint16_t port, String user, String pass)
  {
    Serial.println("Server change");
    Serial.print("IP ");
    Serial.print(ip);
    Serial.print(":");
    Serial.print(port);
    Serial.print("\tUser ");
    Serial.print(user);
    Serial.print(" Pass ");

    UpdateMQTTServerInfo(ip.c_str(), port, user.c_str(), pass.c_str());
  });

  helper.onSubChange(
    [](uint8_t nIndex, String sSubName)
  {
    if (nIndex < MAX_MQTT_SUBSCRIPTIONS)
    {
      Serial.println("Sub name change");
      Serial.print(nIndex);
      Serial.print(" changed to ");
      Serial.println(sSubName);

      UpdateMQTTSubInfo(nIndex, sSubName.c_str());
    }
    else
    {
      Serial.print("Sub index ");
      Serial.print(nIndex);
      Serial.println(" out of bounds");
    }
  });

  helper.onPubChange(
    [](uint8_t nIndex, String sPubName)
  {
    if (nIndex < MAX_MQTT_PUBLICATIONS)
    {
      Serial.println("Pub name change");
      Serial.print(nIndex);
      Serial.print(" changed to ");
      Serial.println(sPubName);

      UpdateMQTTPubInfo(nIndex, sPubName.c_str());
    }
    else
    {
      Serial.print("Pub index ");
      Serial.print(nIndex);
      Serial.println(" out of bounds");
    }
  });

  //Start the network helper
  helper.start();
}

void loop()
{
  if (Serial.available())
  {
    if (Serial.read() == 'r')
    {
      Serial.println("Resetting connection info...");
      ResetConnectionInfo(&savedConnectionInfo);
      Serial.println("Rebooting...");
      SoftReset();
    }
  }

  helper.background();
}
