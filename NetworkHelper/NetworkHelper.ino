#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>

//#define DONT_REMEMBER
#define DEBUG

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

const char* sHelperNetworkServerName = "networkhelper";
const char* sHelperNetworkSSID = "esp8266";
const char* sHelperNetworkPassword = "";

ESP8266WebServer server(80);
ConnectionInfo savedConnectionInfo;

void handleRoot()
{
  server.send(200, "text/html", "<form action=\"/NetworkChange\" method=\"POST\"><input type=\"text\" name=\"ssid\" placeholder=\"SSID\"></br><input type=\"password\" name=\"password\" placeholder=\"Password\"></br><input type=\"submit\" value=\"Update\"></form>");
}

void handleNetworkChange()
{
  //Only need the SSID to have information
  //The password field can be blank
  if (!server.hasArg("ssid") || !server.arg("ssid").length())
    server.send(400, "text/plain", "400: Invalid Request");
  else
  {
#ifdef DEBUG
    Serial.println("Connect to...");
    Serial.print("SSID: "); Serial.println(server.arg("ssid"));
    Serial.print("Password: "); Serial.println(server.arg("password"));
#endif
    server.send(200, "text/html", "<h1>Updated</h1><p>Rebooting system...</p>");

    UpdateConnectionInfo(server.arg("ssid").c_str(), server.arg("password").c_str());
    SoftReset();
  }
}

void configureServer(ESP8266WebServer& server)
{
  server.on("/", handleRoot);
  server.on("/NetworkChange", HTTP_POST, handleNetworkChange);
  MDNS.begin(sHelperNetworkServerName);
}

void UpdateConnectionInfo(const char* ssid, const char* password)
{
#ifndef DONT_REMEMBER
#ifdef DEBUG
  Serial.println(__FUNCTION__);
#endif

  strcpy(savedConnectionInfo.SSID, ssid);
  strcpy(savedConnectionInfo.password, password);
  savedConnectionInfo.checksum = CalcConnectionInfoChecksum(&savedConnectionInfo);

#ifdef DEBUG
  Serial.print("Checksum: "); Serial.println(savedConnectionInfo.checksum);
#endif

#endif
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
  //TODO: detect wheter or not this is an access point
  WiFi.softAPdisconnect (true);
  server.stop();
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

void setup()
{
  delay(1000);

#ifdef DEBUG
  Serial.begin(115200);
  Serial.println();
#endif

#ifdef DEBUG
  Serial.print("SSID: ");
  Serial.println(sHelperNetworkSSID);
#endif

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  if (strlen(sHelperNetworkPassword))
  {
#ifdef DEBUG
    Serial.print("PSWRD: ");
    Serial.println(sHelperNetworkPassword);
#endif
    WiFi.softAP(sHelperNetworkSSID, sHelperNetworkPassword);
  }
  else
  {
    WiFi.softAP(sHelperNetworkSSID);
  }

  configureServer(server);

  server.begin();
}

void loop()
{
  server.handleClient();
}
