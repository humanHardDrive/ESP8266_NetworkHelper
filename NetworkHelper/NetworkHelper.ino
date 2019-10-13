#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>

#define DONT_REMEMBER
#define DEBUG

//Access point configuration
IPAddress local_IP(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

const char* sHelperNetworkServerName = "networkhelper";
const char* sHelperNetworkSSID = "esp8266";
const char* sHelperNetworkPassword = "";

ESP8266WebServer server(80);

void handleRoot()
{
 server.send(200, "text/html", "<form action=\"/login\" method=\"POST\"><input type=\"text\" name=\"username\" placeholder=\"Username\"></br><input type=\"password\" name=\"password\" placeholder=\"Password\"></br><input type=\"submit\" value=\"Login\"></form><p>Try 'John Doe' and 'password123' ...</p>");
}

void configureServer(ESP8266WebServer& server)
{
  server.on("/", handleRoot);
  MDNS.begin(sHelperNetworkServerName);
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
