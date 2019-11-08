#include "Smarty.h"

/**
 * Constructor
 * @param _name - name of node
 * @param _desk - short description
 */
Smarty::Smarty(String _name, String _desc)
{
	name = _name;
	desc = _desc;
	conn_status.wifi_connected = false;
	conn_status.server_connected = false;
	conn_status.got_ip = false;

	WiFi.hostname(name);
	WiFi.mode(WIFI_STA);

	ArduinoOTA.setHostname(name.c_str());
	ArduinoOTA.begin();

	client.setTimeout(50);

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

Smarty::Smarty(String _name, String _desc, const char * _ssid, const char * _pass, IPAddress _serverIP, uint16_t _port) : Smarty(_name, _desc) {
	conn_status.hardcoded_data = true;
	strcpy(conn_data.ssid, _ssid);
	strcpy(conn_data.pass, _pass);
	conn_data.serverIP = _serverIP;
	conn_data.port = _port;
}

void Smarty::onConnect(const WiFiEventStationModeConnected& event) {
	LOGln("WiFi connected");
}

void Smarty::onDisconnect(const WiFiEventStationModeDisconnected& event) {
	conn_status.got_ip = false;
	LOGln("WiFi disconnected");
}

void Smarty::onGotIP(const WiFiEventStationModeGotIP& event) {
	Udp.begin(conn_data.port);

	bcastAddr = (event.ip.v4() & event.mask.v4()) | ~event.mask.v4();
	LOGf("Got IP: %s\n", event.ip.toString().c_str());
	LOGf("Got NETMASK: %s\n", event.mask.toString().c_str());
	LOGf("Got GATEWAY: %s\n", event.gw.toString().c_str());
	LOGf("Got BROADCAST ADDR: %s\n", bcastAddr.toString().c_str());

	jsonBuffer["header"] = WHERE_IS_SERVER;
	send(true);

	conn_status.got_ip = true;
}

void Smarty::begin() {
	LOGSTART();
	LOGln();

	if ( !conn_status.hardcoded_data )
		EEPROM_read();

	LOGf("Connection data:\n\tWifi SSID: %s\n\tWifi Password: %s\n\tServer IP: %s\n\tServer port: %d\n", \
					conn_data.ssid, conn_data.pass, conn_data.serverIP.toString().c_str(), conn_data.port);
	LOGln("Trying to connect to WiFi network");
	WiFi.begin(conn_data.ssid, conn_data.pass);
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
 * Read connection data to EEPROM
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
 * checking new messages from server.
 * Run this function periodically from loop()
 */
void Smarty::checkConnection() {
	ArduinoOTA.handle();

	// Check UDP messages
	char _buf[BUF_SIZE];
	int packetSize = Udp.parsePacket();
	if (packetSize) {
		int len = Udp.read(_buf, sizeof(_buf)-1);
		if (len > 0) {
			_buf[len] = '\0';
		}
		IPAddress remoteIp = Udp.remoteIP();
		LOGf("Received packet of size %d from %s, port %d\n", packetSize, remoteIp.toString().c_str(), Udp.remotePort());
		LOGf("\tContents: ");
		LOGln(_buf);
  	}
	/////////////////////

	if ( conn_status.got_ip && !conn_status.server_connected ) {
		if ( tcpConnect() ) {
			sendFullInfo();
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

	serializeJson(jsonBuffer, _buf);
	jsonBuffer.clear();

	if ( broadcast ) {
		LOGf("Sending broadcast message: %s ...", _buf);
		Udp.beginPacket(bcastAddr, conn_data.port);
		Udp.write(_buf);
		Udp.endPacket();
		LOGln("Success");
		return true;
	}
	else {
		LOGf("Sending message to server: %s ... ", _buf);
		if ( conn_status.server_connected ) {
			if ( client.write(_buf) ) {
				LOGln("Success");
				return true;
			}
			else {
				conn_status.server_connected = false;
			}
		}
		else {
			LOG("not connected to server ... ");
		}
	}
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
					  void (*_cb)(param_value_t), \
					  int8_t _divisor) {

	Param *new_param = new Param;

	new_param->type = _type;
	strcpy(new_param->desc, _desc);
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
	new_param->callback = _cb;

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

	jsonBuffer["header"] = MY_NAME;
	jsonBuffer["mac"] = WiFi.macAddress();
	jsonBuffer["name"] = name;
	jsonBuffer["desc"] = desc;
	send();

	for ( i = 0; i < params.size(); i++ ) {
		jsonBuffer["header"] = MY_PARAMS;
		jsonBuffer["mac"] = WiFi.macAddress();
		jsonBuffer["desc"] = params[i].desc;
		jsonBuffer["curValue"] = params[i].curValue;
		jsonBuffer["minValue"] = params[i].minValue;
		jsonBuffer["maxValue"] = params[i].maxValue;
		jsonBuffer["divisor"] = params[i].divisor;
		send();
	}
}

/**
 * Send current parameter value to server
 * @param _num - number of parameter
 */
void Smarty::sendParam(uint8_t _num) {
	jsonBuffer["header"] = NEW_VALUE;
	jsonBuffer["mac"] = WiFi.macAddress();
	jsonBuffer["param"] = _num;
	jsonBuffer["value"] = params[_num].curValue;
	send();
}

bool Smarty::tcpConnect() {
	if (WiFi.status() == WL_CONNECTED && WiFi.localIP()) {
		LOG("Trying to connect to server... ");
		if ( client.connect(conn_data.serverIP, conn_data.port) ) {
			LOGln("Connected");
			conn_status.server_connected = true;
			return true;
		}
	}
	LOGln("Failed");
	return false;
}