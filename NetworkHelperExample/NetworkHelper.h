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
	NetworkHelper(const String& sServerName);
	~NetworkHelper();

	void start();
	void start(const String& sServerName);

	void background();

	void stop();

	void onNetworkChange(std::function<void(String, String)> fn) { m_OnNetworkChange = fn; }

private:
	void configureServer();

	void handleRoot();
	void handleManualEntry();
	void handleScan();
	void handleNetworkChange();

	ESP8266WebServer m_Server;
	String m_sServerName;
	bool m_bRunning = false;

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
