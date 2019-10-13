#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <string>

//#define DONT_REMEMBER
#define RST_BTN
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

const char* sHelperNetworkServerName = "networkhelper"; //Name for the DNS
const char* sHelperNetworkSSID = "esp8266"; //AP SSID
const char* sHelperNetworkPassword = ""; //AP Password
//If the password field is empty, no password is used

bool bConnectedToAP = false;

ESP8266WebServer server(80);
ConnectionInfo savedConnectionInfo;

void handleRoot()
{
  server.send(200, "text/html", "<p><a href=\"/manualentry\">Enter SSID Manually</a></p><p><a href=\"/scan\">Scan for Networks</a></p>");
}

void handleManualEntry()
{
  server.send(200, "text/html", "<form action=\"/NetworkChange\" method=\"POST\"><input type=\"text\" name=\"ssid\" placeholder=\"SSID\"></br><input type=\"password\" name=\"password\" placeholder=\"Password\"></br><input type=\"submit\" value=\"Update\"></form>");
}

void handleScan()
{
  String msg = "<head>  \
                <meta http-equiv='refresh' content='15'> \
                <style> \
                table { \
                  font-family: arial, sans-serif; \
                  border-collapse: collapse;  \
                  width: 75%;  \
                } \
                td, th {  \
                  border: 1px solid #dddddd;  \
                  text-align: left; \
                  padding: 8px; \
                } \
                tr:nth-child(even) {  \
                  background-color: #dddddd;  \
                } \
                </style>  \
                </head>";
  bool bScanComplete = true;

  int scanCount = WiFi.scanComplete();

#ifdef DEBUG
  Serial.print(scanCount); Serial.println(" networks found");
#endif

  if (scanCount < 0)
  {
    if(scanCount == -1) //Scan is still in progress
      bScanComplete = false;
    //A scanCount of -2 means that a scan was never started
    //Use the scan complete flag to bootstrap
      
    scanCount = 0;
  }

  msg += "<h1>";
  msg += scanCount;
  msg += " Networks Found</h1>";

  msg += "<table> \
          <tr>  \
          <th>SSID</th> \
          <th>Encryption</th> \
          <th>RSSI</th> \
          <th></th>";

  for (int i = 0; i < scanCount; i++)
  {
    String ssid = WiFi.SSID(i);
    
    msg += "<tr>";
    // WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE
    msg += "<td>" + ssid + "</td>";
    msg += "<td>" + String(WiFi.encryptionType(i)) + "</td>";
    msg += "<td>" + String(WiFi.RSSI(i)) + "</td>";

    msg += "<td>  \
            <form action=\"/NetworkChange\" method=\"post\" \
            <input type=\"hidden\" name=\"ssid\" value=\"" + ssid + "\"/> \
            <input type=\"submit\" name=\"Connect\" \
            </form> \
            </td>";
    
    msg += "</tr>";
  }
  msg += "</table>";

  if (bScanComplete)
  {
#ifdef DEBUG
    Serial.println("Starting new scan");
#endif
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
  }

  server.send(200, "text/html", msg.c_str());
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
  server.on("/", HTTP_GET, handleRoot);
  server.on("/manualentry", HTTP_GET, handleManualEntry);
  server.on("/scan", HTTP_GET, handleScan);
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

  EEPROM.end();
  server.stop();

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

  configureServer(server);

  server.begin();
}

void loop()
{
  server.handleClient();
}
