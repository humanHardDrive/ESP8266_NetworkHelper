#ifndef __NETWORK_HELPER_H__
#define __NETWORK_HELPER_H__

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>

#include <string>

#define MQTTHelper

#ifdef MQTTHelper
#define MQTT_STATUS_OFFSET	-4
#endif

class NetworkHelper
{
public:
	NetworkHelper();
	NetworkHelper(const String& sServerName);
#ifdef MQTTHelper
	NetworkHelper(char** pPubList, char** pSubList, uint8_t nMaxPubCount, uint8_t nMaxSubCount);
	NetworkHelper(const String& sServerName, char** pPubList, char** pSubList, uint8_t nMaxPubCount, uint8_t nMaxSubCount);
#endif
	~NetworkHelper();

	void start();
	void start(const String& sServerName);

	void background();

	void stop();

	void onNetworkChange(std::function<void(String, String)> fn) { m_OnNetworkChange = fn; }
#ifdef MQTTHelper
	void setSubList(char** list);
	void setPubList(char** list);

	void onServerChange(std::function<void(String, uint16_t, String, String)> fn) { m_OnServerChange = fn; }
	
	void onAddSub(std::function<void(String)> fn) { m_OnAddSubscription = fn; }
	void onRemoveSub(std::function<void(String)> fn) { m_OnRemoveSubscription = fn; }
	
	void onAddPub(std::function<void(String)> fn) { m_OnAddPublication = fn; }
	void onRemovePub(std::function<void(String)> fn) { m_OnRemovePublication = fn; }
#endif

private:
	void configureServer();

	void handleRoot();
	
	void handleManualEntry();
	void handleScan();
	void handleNetworkChange();

#ifdef MQTTHelper
	void handleServerEntry();
	void handleServerChange();
	
	void handleTestServerConnection();
	
	void handleSubscriptions();
	void handleModifySubscription();
	
	void handlePublications();
	void handleModifyPublication();
#endif

	ESP8266WebServer m_Server;
	String m_sServerName;
	bool m_bRunning = false;

	std::function<void(String, String)> m_OnNetworkChange;
#ifdef MQTTHelper
	char** m_pPubList;
	char** m_pSubList;

	uint8_t m_nMaxSubCount, m_nMaxPubCount;

	std::function<void(String, uint16_t, String, String)> m_OnServerChange;
	
	std::function<void(String)> m_OnAddSubscription;
	std::function<void(String)> m_OnRemoveSubscription;
	
	std::function<void(String)> m_OnAddPublication;
	std::function<void(String)> m_OnRemovePublication;
#endif

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
	
#ifdef MQTTHelper
	const String MQTT_STATUS_NAME[10]
	{
		"CONNECTION TIMEOUT",		//-4
		"CONNECTION LOST",			//-3
		"CONNECT FAILED",			//-2
		"DISCONNECTED",				//-1
		"CONNECTED",				//0
		"BAD PROTOCOL",				//1
		"BAD CLIENT ID",			//2
		"UNAVAILABLE",				//3
		"BAD CREDENTIALS",			//4
		"UNAUTHORIZED"				//5
	};
#endif
};

#endif
