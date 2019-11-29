#include "NetworkHelper.h"

NetworkHelper::NetworkHelper() :
	m_sServerName("NetworkHelper")
{
  configureServer();
}

NetworkHelper::NetworkHelper(const String& sServerName) :
  m_sServerName(sServerName)
{
  configureServer();
}

NetworkHelper::~NetworkHelper()
{
  stop();
}


void NetworkHelper::start()
{
  NetworkHelper::start(m_sServerName);
}

void NetworkHelper::start(const String& sServerName)
{
  if(!m_bRunning)
  {
    MDNS.begin(sServerName);
    m_Server.begin();
    WiFi.scanNetworks(true);

    m_bRunning = true;
  }
}


void NetworkHelper::stop()
{
  if(m_bRunning)
  {
	  m_Server.stop();

    m_bRunning = false;
  }
}


void NetworkHelper::background()
{
  m_Server.handleClient();
}


void NetworkHelper::configureServer()
{
	m_Server.on("/", HTTP_GET, std::bind(&NetworkHelper::handleRoot, this));
	m_Server.on("/manualentry", HTTP_GET, std::bind(&NetworkHelper::handleManualEntry, this));
	m_Server.on("/manualentry", HTTP_POST, std::bind(&NetworkHelper::handleManualEntry, this));
	m_Server.on("/scan", HTTP_GET, std::bind(&NetworkHelper::handleScan, this));
	m_Server.on("/NetworkChange", HTTP_POST, std::bind(&NetworkHelper::handleNetworkChange, this));
	
#ifdef MQTTHelper
	m_Server.on("/serverentry", HTTP_GET, std::bind(&NetworkHelper::handleServerEntry, this));
	m_Server.on("/subscription", HTTP_GET, std::bind(&NetworkHelper::handleSubscriptions, this));
	m_Server.on("/publish", HTTP_GET std::bind(&NetworkHelper::handlePublish, this));
#endif
}


void NetworkHelper::handleRoot()
{
	m_Server.send(200, "text/html", "<p><a href=\"/manualentry\">Enter SSID Manually</a></p><p><a href=\"/scan\">Scan for Networks</a></p>");
}

void NetworkHelper::handleManualEntry()
{
  String msg;

  msg = "<form action=\"/NetworkChange\" method=\"POST\"> \
        <input type=\"text\" name=\"ssid\"";

  if (m_Server.hasArg("ssid"))
    msg += "value=\"" + m_Server.arg("ssid") + "\"";
  else
    msg += "placeholder=\"SSID\"";

  msg += "> \
         <input type = \"password\" name=\"password\" placeholder=\"Password\">  \
         <input type=\"submit\" value=\"Update\"></form>";

  m_Server.send(200, "text/html",  msg);
}

void NetworkHelper::handleScan()
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

  //Scan count of -1 means the scan is still in progress
  if (scanCount < 0)
  {
    bScanComplete = false;
	//Show no networks
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
    msg += "<td>" + ssid + "</td>";
    msg += "<td>" + ENCRYPTION_NAME[WiFi.encryptionType(i)] + "</td>";
    msg += "<td>" + String(WiFi.RSSI(i)) + " dB</td>";

    msg += "<td>";
    if (WiFi.encryptionType(i) == 7)
      msg += "<form action=\"/NetworkChange\" method=\"POST\">";
    else
      msg += "<form action=\"/manualentry\" method=\"POST\">";

    msg += "<input type=\"hidden\" name=\"ssid\" value=\"" + ssid + "\"/> \
            <input type=\"submit\" name=\"connect\" value=\"Connect\"> \
            </form> \
            </td>";

    msg += "</tr>";
  }
  msg += "</table>";

  if (bScanComplete)
  {
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
  }

  m_Server.send(200, "text/html", msg.c_str());
}

void NetworkHelper::handleNetworkChange()
{
  //Only need the SSID to have information
  //The password field can be blank
  if (!m_Server.hasArg("ssid") || !m_Server.arg("ssid").length())
    m_Server.send(400, "text/plain", "400: Invalid Request");
  else
  {
    m_Server.send(200, "text/html", "<h1>Updated</h1>");

	if(m_OnNetworkChange)
		m_OnNetworkChange(m_Server.arg("ssid"), m_Server.arg("password"));
  }
}