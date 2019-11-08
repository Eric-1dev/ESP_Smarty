#ifndef __SMARTY_H__
#define __SMARTY_H__

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "Types.h"
#include "config.h"

#ifdef DEBUG
	#define LOGSTART Serial.begin(115200);
	#define LOG Serial.print
    #define LOGf Serial.printf
	#define LOGln Serial.println
#else
	#define LOGSTART
	#define LOG(x)
    #define LOGf(x, ...)
	#define LOGln(x)
#endif

class Smarty
{
	public:
		Smarty(String, String);
		Smarty(String, String, const char *, const char *, IPAddress, uint16_t);
		void begin();
		void checkConnection();
		void addParam(paramType_t _type, \
					  const char *_desc, \
					  param_value_t _minValue, \
					  param_value_t _maxValue, \
					  bool _rem_target, \
					  void (*_cb)(param_value_t) = NULL, \
					  int8_t _divisor = 1);
		param_value_t getTargetVal(uint8_t _num);
		bool setValue(uint8_t _num, param_value_t _value);

	private:
		const char *baseApSSID = ESP_AP_SSID;
		const char *baseApPass = ESP_AP_PASS;
		String name;
		String desc;
		conn_data_t conn_data;
		conn_status_t conn_status;
		WiFiEventHandler mConnectHandler;
		WiFiEventHandler mGotIPHandler;
		WiFiEventHandler mDisconnectHandler;
		IPAddress bcastAddr;
		WiFiClient client;
		WiFiUDP Udp;
		StaticJsonDocument<512> jsonBuffer;
		std::vector<Param> params;
		uint8_t numRemValues = 0;	// Number of remembered taget values

		void EEPROM_write(conn_data_t*);
		void EEPROM_read();
		void onConnect(const WiFiEventStationModeConnected&);
		void onDisconnect(const WiFiEventStationModeDisconnected&);
		void onGotIP(const WiFiEventStationModeGotIP&);
		bool send(bool broadcast = false);
		bool receivedVal(uint8_t _num, param_value_t _value);
		void sendFullInfo();
		void sendParam(uint8_t _num);
		bool tcpConnect();
		void askConnData();
};

#endif