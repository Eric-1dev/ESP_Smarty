#include "Smarty.h"

/**
 * Constructor
 * @param _name - name of node
 * @param _desk - short description
 */
Smarty::Smarty(String _name, String _desc)
{
	LOGSTART;
	LOGln();

	name = _name;
	desc = _desc;
	conn_status.getConnDataMode = false;
	conn_status.triesleft = WIFI_RECONNECT_TRIES;

	WiFi.hostname(name);
	if ( strcmp(name.c_str(), ESP_BASE_NAME) != 0 ) {
		LOGln("Regular stantion mode enabled");
		WiFi.mode(WIFI_STA);
	}
	else {
		LOGln("Base ESP AP mode enabled");
		isESPBase = true;
		WiFi.mode(WIFI_AP_STA);
		WiFi.softAPConfig(IPAddress(ESP_SERVER_IP), IPAddress(ESP_SERVER_IP), IPAddress(255,255,255,0));
  		WiFi.softAP(ESP_AP_SSID, ESP_AP_PASS);
	}
	WiFi.setAutoReconnect(false);

	ArduinoOTA.setHostname(name.c_str());
	ArduinoOTA.begin();

	client.setTimeout(100);

	// Event handlers:
	mConnectHandler = WiFi.onStationModeConnected([this](const WiFiEventStationModeConnected& event) {
		onConnect(event);
	});
	mDisconnectHandler = WiFi.onStationModeDisconnected([this](const WiFiEventStationModeDisconnected& event) {
		onDisconnect(event);
	});
	mGotIPHandler = WiFi.onStationModeGotIP([this](const WiFiEventStationModeGotIP& event) {
		onGotIP(event);
    });
	//////////////////
}

/**
 * Constructor that used for hardcoded wifi & server settings
 * @param _name - name of ESP
 * @param _desk - short description
 * @param _ssid - ssid of your home WiFi
 * @param _pass - password of your WiFi
 * @param _serverIP - IP-address of your server
 * @param _port - port, that you select to TCP & UDP protocols
 */
Smarty::Smarty(String _name, String _desc, const char * _ssid, const char * _pass, IPAddress _serverIP, uint16_t _port) : Smarty(_name, _desc) {
	conn_status.hardcoded_data = true;
	strcpy(conn_data.ssid, _ssid);
	strcpy(conn_data.pass, _pass);
	conn_data.serverIP[0] = _serverIP[0];
	conn_data.serverIP[1] = _serverIP[1];
	conn_data.serverIP[2] = _serverIP[2];
	conn_data.serverIP[3] = _serverIP[3];
	conn_data.port = _port;
}

void Smarty::onConnect(const WiFiEventStationModeConnected& event) {
	LOGln("WiFi connected");
}

void Smarty::onDisconnect(const WiFiEventStationModeDisconnected& event) {
	conn_status.triesleft--;
	LOGf("WiFi disconnected ... tries left: %d\n", conn_status.triesleft);
}

void Smarty::onGotIP(const WiFiEventStationModeGotIP& event) {
	Udp.begin(conn_data.port);

	bcastAddr = (event.ip.v4() & event.mask.v4()) | ~event.mask.v4();
	netaddr = event.ip.v4() & event.mask.v4();

	LOGf("Got IP: %s\n", event.ip.toString().c_str());
	LOGf("Got NETMASK: %s\n", event.mask.toString().c_str());
	LOGf("Got GATEWAY: %s\n", event.gw.toString().c_str());
	LOGf("Got BROADCAST ADDR: %s\n", bcastAddr.toString().c_str());
	LOGf("Got NETWORK: %s\n", netaddr.toString().c_str());

	conn_status.triesleft = SERVER_RECONNECT_TRIES;
	lastDisconnectTime = 0;
}

/**
 * Start WiFi
 * Run this function after all parameter add
 */
void Smarty::begin() {

	if ( !conn_status.hardcoded_data || isESPBase )
		EEPROM_read();

	LOGf("Connection data from EEPROM:\n\tWifi SSID: %s\n\tWifi Password: %s\n\tServer IP: %s\n\tServer port: %d\n", \
					conn_data.ssid, conn_data.pass, IPAddress(conn_data.serverIP).toString().c_str(), conn_data.port);

	LOGln("Trying to connect to WiFi network");

	if ( WiFi.begin(conn_data.ssid, conn_data.pass) == WL_CONNECT_FAILED ) {
		WiFi.begin(ESP_AP_SSID, ESP_AP_PASS);
	}
}

/**
 * Write connection data to EEPROM
 * @param data - connection data
 */
void Smarty::EEPROM_write(conn_data_t* data) {
	EEPROM.begin(sizeof(conn_data));
	for(uint8_t i = 0; i < sizeof(conn_data); i++) {
		EEPROM.write(i, *( ((uint8_t *)(data)) + i ));
	}
	EEPROM.end();
}

/**
 * Read connection data from EEPROM
 */
void Smarty::EEPROM_read() {
	EEPROM.begin(sizeof(conn_data));
	for(uint8_t i = 0; i < sizeof(conn_data); i++) {
		*((uint8_t *)&conn_data + i) = EEPROM.read(i);
	}
	EEPROM.end();
}

/**
 * Handling ArduinoOTA, checking WiFi connection,
 * checking new messages from server,
 * checking server connection.
 * Run this function periodically from loop()
 */
void Smarty::checkConnection() {
	if ( ( WiFi.status() != WL_CONNECTED ) && ( millis() - lastDisconnectTime >= WIFI_RECONNECT_INTERVAL ) ) {
		lastDisconnectTime = millis();
		if ( !conn_status.hardcoded_data && !isESPBase && (!conn_status.triesleft) ) {
			conn_status.getConnDataMode = !conn_status.getConnDataMode;
			if ( conn_status.getConnDataMode )
				conn_status.triesleft = 1;
			else
				conn_status.triesleft = WIFI_RECONNECT_TRIES;
		}
		if ( conn_status.getConnDataMode ) {
			LOGf("Trying to get new WiFi data:\n\tSSID: %s\n\tPass: %s\n", ESP_AP_SSID, ESP_AP_PASS);
			WiFi.begin(ESP_AP_SSID, ESP_AP_PASS);
		}
		else {
			LOGf("Trying to connect to WiFi normally:\n\tSSID: %s\n\tPass: %s\n", conn_data.ssid, conn_data.pass);
			WiFi.begin(conn_data.ssid, conn_data.pass);
		}
	}
	if ( !conn_status.getConnDataMode && WiFi.status() == WL_CONNECTED && WiFi.localIP() ) {
		ArduinoOTA.handle();
		// Check UDP messages
		char _buf[BUF_SIZE];
		int packetSize = Udp.parsePacket();
		if (packetSize) {
			IPAddress remoteIp = Udp.remoteIP();
			LOGf("Received packet of size %d from %s, port %d\n\tContents: ", packetSize, remoteIp.toString().c_str(), Udp.remotePort());
			LOGln(_buf);
			if ( !deserializeJson( jsonDoc, Udp ) ) {
				messageHandler();
			}
			//int len = Udp.read(_buf, sizeof(_buf)-1);
			//if (len > 0) {
			//	_buf[len] = '\0';
			//}
		}
		/////////////////////
	}

	if ( !client.connected() ) {
		checkTCP();
	}
	else {
		if ( client.available() ) {
			if ( !deserializeJson( jsonDoc, client.readStringUntil('\0') ) ) {
				//char _buf[BUF_SIZE];
				//serializeJsonPretty(jsonDoc, _buf);
				//LOGf("Message received:\n%s\n", _buf );
				messageHandler();
			}
		}
	}
}

/**
 * Send data to server
 * @param mes - message
 * @param broadcast - True - UDP broacast packet. False - TCP packet to current server
 */
bool Smarty::send(bool broadcast) {
	char _buf[BUF_SIZE];	// buffer for network message

	//_buf[serializeJsonPretty(jsonDoc, _buf)] = '\0';

	if ( broadcast ) {
		LOGf("Sending broadcast message: %s ...", _buf);
		Udp.beginPacket(bcastAddr, conn_data.port);
		serializeJsonPretty(jsonDoc, Udp);
		Udp.endPacket();
		jsonDoc.clear();
		LOGln("Success");
		return true;
	}
	else {
		LOGf("Sending message to server: %s ... ", _buf);
		if ( client.connected() ) {
			if ( serializeJsonPretty(jsonDoc, client) ) {
				client.write('\0');
				jsonDoc.clear();
				LOGln("Success");
				return true;
			}
		}
		else {
			LOG("not connected to server ... ");
		}
	}
	jsonDoc.clear();
	LOGln("Failed");
	return false;
}

/**
 * Add new parameter of our ESP
 * @param _type - can be SWITCH, RGB, NUMBER or NONE
 * @param _desc - a brief description of what the parameter is for
 * @param _minValue - minimum allowed value. Used with _type = NUMBER
 * @param _maxValue - maximum allowed value. Used with _type = NUMBER
 * @param _remember_target - should ESP remember the target value of the parameter in EEPROM
 * @param _cb - callback function called on receive target value from server
 * @param _divisor - value of divisior for transmiting non-integer values. Default equal 1
 */
void Smarty::addParam(paramType_t _type, \
					  const char *_desc, \
					  param_value_t _minValue, \
					  param_value_t _maxValue, \
					  bool _rem_target, \
					  void (*_callback)(param_value_t), \
					  int8_t _divisor) {

	Param *new_param = new Param;

	new_param->num = params.size();
	strcpy(new_param->desc, _desc);
	new_param->type = _type;
	switch (_type)
	{
		case SWITCH:
			new_param->minValue = 0;
			new_param->maxValue = 1;
			new_param->divisor = 1;
			break;

		case RGB:
			new_param->minValue = -32768;
			new_param->maxValue = 32767;
			new_param->divisor = 1;
			break;

		default:
			new_param->minValue = _minValue;
			new_param->maxValue = _maxValue;
			new_param->divisor = _divisor;
			break;
	}
	new_param->remember_target = _rem_target;
	if ( _rem_target )
		numRemValues++;
	new_param->callback = _callback;

	params.push_back(*new_param);

	delete new_param;
}

/**
 * Received new target value from server
 * @param _num - number of parameter
 * @param _value - new target value
 */
bool Smarty::receivedVal(uint8_t _num, param_value_t _value) {
	if ( _num < params.size() ) {
		if ( params[_num].targetValue != _value ) {
			if ( _value > params[_num].maxValue || _value < params[_num].minValue ) {
				LOGln("Parameter value out of range");
				return false;
			}
			if ( params[_num].remember_target ) {
				/* Write in EEPROM */
			}
			params[_num].targetValue = _value;
			params[_num].callback(_value);
		}
		return true;
	}
	LOGln("Parameter not exist");
	return false;
}

/**
 * Return target value of current parameter
 * @param _num number of parameter
 * @return {param_value_t} target value
 */
param_value_t Smarty::getTargetVal(uint8_t _num) {
	if ( _num < params.size() )
		return params[_num].targetValue;
	LOGln("Parameter not exist");
	return -32768;
}

/**
 * Set current value to selected parameter
 * @param _num - number of parameter
 * @param _value - new current value
 */
bool Smarty::setValue(uint8_t _num, param_value_t _value) {
	if ( _num < params.size() ) {
		params[_num].curValue = _value;
		sendParam(_num);
		return true;
	}
	LOGln("Parameter not exist");
	return false;
}

void Smarty::sendFullInfo() {
	uint8_t i;

	jsonDoc["header"] = (uint8_t)MY_NAME;
	jsonDoc["mac"] = WiFi.macAddress();
	jsonDoc["name"] = name;
	jsonDoc["desc"] = desc;
	send();

	for ( i = 0; i < params.size(); i++ ) {
		jsonDoc["header"] = (uint8_t)MY_PARAMS;
		jsonDoc["mac"] = WiFi.macAddress();
		jsonDoc["num"] = params[i].num;
		jsonDoc["desc"] = params[i].desc;
		jsonDoc["type"] = params[i].type;
		jsonDoc["curValue"] = params[i].curValue;
		jsonDoc["targetValue"] = params[i].targetValue;
		jsonDoc["minValue"] = params[i].minValue;
		jsonDoc["maxValue"] = params[i].maxValue;
		jsonDoc["divisor"] = params[i].divisor;
		send();
	}
}

/**
 * Send current parameter value to server
 * @param _num - number of parameter
 */
void Smarty::sendParam(uint8_t _num) {
	if ( conn_status.getConnDataMode )
		return;
	jsonDoc["header"] = (uint8_t)NEW_VALUE;
	jsonDoc["mac"] = WiFi.macAddress();
	jsonDoc["num"] = _num;
	jsonDoc["value"] = params[_num].curValue;
	send();
}

void Smarty::sendAllParams() {
	uint8_t i;
	for ( i = 0; i < params.size(); i++ )
		sendParam(i);
}

bool Smarty::checkTCP() {
	if ( WiFi.status() == WL_CONNECTED && WiFi.localIP() ) {
		if ( conn_status.getConnDataMode ) {
			if ( serverConnect(IPAddress(ESP_SERVER_IP), ESP_SERVER_PORT) )
				askConnData();
		}
		else {
			if ( millis() - lastDisconnectTime > SERVER_RECONNECT_INTERVAL ) {
				lastDisconnectTime = millis();
				if ( serverConnect(IPAddress(conn_data.serverIP), conn_data.port) )
					sendFullInfo();
			}
		}
	}

	if ( !conn_status.triesleft ) {
		if ( !conn_status.hardcoded_data && !isESPBase ) {
			if ( conn_status.getConnDataMode ) {
				conn_status.getConnDataMode = false;
				conn_status.triesleft = WIFI_RECONNECT_TRIES + 1;
			}
			else {
				conn_status.getConnDataMode = false;
				conn_status.triesleft = 1;
			}
			lastDisconnectTime = 0;
			WiFi.disconnect();
		}
		else {
			conn_status.triesleft = SERVER_RECONNECT_TRIES + 1;
		}
	}
	return false;
}

bool Smarty::serverConnect(IPAddress _server, uint16_t _port) {
	if ( _server == IPAddress(0,0,0,0) ) {
		jsonDoc["header"] = (uint8_t)WHERE_IS_SERVER;
		jsonDoc["mac"] = WiFi.macAddress();
		jsonDoc["netaddr"] = netaddr;
		send(true);
		return false;
	}
	LOGf("Trying to connect to server %s, port %d\n", _server.toString().c_str(), _port);
	if ( client.connect(_server, _port) ) {
		LOGln("\tConnected to server");
		client.keepAlive(5, 1, 3);
		conn_status.triesleft = SERVER_RECONNECT_TRIES;
		return true;
	}
	else {
		conn_status.triesleft--;
		LOGf("\tFailed ... tries = %d\n", conn_status.triesleft);
	}
	return false;
}

void Smarty::askConnData() {
	LOGln("Asking new WiFi & Server data");
	jsonDoc["header"] = (uint8_t)GIVE_ME_DATA;
	jsonDoc["mac"] = WiFi.macAddress();
	send();
}

char* Smarty::getSSID() {
	return conn_data.ssid;
}

char* Smarty::getPASS() {
	return conn_data.pass;
}

IPAddress Smarty::getServerIP() {
	return IPAddress(conn_data.serverIP);
}

uint16_t Smarty::getPort() {
	return conn_data.port;
}

void Smarty::messageHandler() {
	switch ( (uint8_t)jsonDoc["header"] ) {
		case MY_NAME:
			LOGln("What?");
			break;
		case SET_VALUE:
			receivedVal(jsonDoc["num"], jsonDoc["targetValue"]);
			break;
		case GIVE_ME_VALUES:
			sendAllParams();
			break;
		case SERVER_HERE:
			//const char* _ip_str = jsonDoc["serverIP"];
			//IPAddress _ip = IPAddress::fromString( _ip_str );
			IPAddress _ip(0,0,0,0);
			//_ip.fromString( _ip_str );
			conn_data.serverIP[0] = _ip[0];
			conn_data.serverIP[1] = _ip[1];
			conn_data.serverIP[2] = _ip[2];
			conn_data.serverIP[3] = _ip[3];
			break;
		default:
			break;
	}
}