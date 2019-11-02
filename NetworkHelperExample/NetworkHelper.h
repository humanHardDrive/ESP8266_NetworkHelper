#ifndef __NETWORK_HELPER_H__
#define __NETWORK_HELPER_H__

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>

#include <string>

class NetworkHelper
{
public:
	NetworkHelper();
	NetworkHelper(const IPAddress& localIP, const IPAddress& gatewayIP, const IPAddress& subnet);
	NetworkHelper(const String& sServerName, const String& sNetworkSSID, const String& sNetworkPassword);
	NetworkHelper(const IPAddress& localIP, const IPAddress& gatewayIP, const IPAddress& subnet,
				const String& sServerName, const String& sNetworkSSID, const String& sNetworkPassword);
	~NetworkHelper();

	void start();
	void start(const IPAddress& localIP, const IPAddress& gatewayIP, const IPAddress& subnet);
	void start(const String& sServerName, const String& sNetworkSSID, const String& sNetworkPassword);
	void start(const IPAddress& localIP, const IPAddress& gatewayIP, const IPAddress& subnet,
			const String& sServerName, const String& sNetworkSSID, const String& sNetworkPassword);

	void stop();

	void setLocalIP(IPAddress ip);
	void setGateway(IPAddress ip);
	void setSubnet(IPAddress mask);

	void setNetworkName(const String& sName);
	void setNetworkSSID(const String& sSSID);
	void setNetworkPassword(const String& sPassword);

	void onNetworkChange(std::function<void(String, String)> fn) { m_OnNetworkChange = fn; }

private:
	void configureServer();

	void handleRoot();
	void handleManualEntry();
	void handleScan();
	void handleNetworkChange();

	ESP8266WebServer m_Server;
	IPAddress m_LocalIP, m_GatewayIP, m_Subnet;
	String m_sServerName, m_sNetworkSSID, m_sNetworkPassword;

	std::function<void(String, String)> m_OnNetworkChange;

	const String ENCRYPTION_NAME[9] =
	{
		"UNKOWN",   	//0
		"UNKOWN",   	//1
		"WPA/PSK",  	//2
		"UNKOWN",		//3
		"WPA2/PSK",		//4
		"WEP",			//5
		"UNKOWN",		//6
		"NONE",			//7
		"WPA/WPA2/PSK"	//8
	};
};

#endif
