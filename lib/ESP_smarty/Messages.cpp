#include "Smarty.h"

/**
 * Send data to server
 * @param mes - message
 * @param broadcast - True - UDP broacast packet. False - TCP packet to current server
 */
bool Smarty::send(bool broadcast) {
#ifdef DEBUG
	char _buf[BUF_SIZE];	// buffer for network message
	_buf[serializeJsonPretty(jsonDoc, _buf)] = '\0';
#endif

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
 * Received new target value from server
 * @param _num - number of parameter
 * @param _value - new target value
 */
bool Smarty::setTargetVal(uint8_t _num, param_value_t _value) {
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
	for ( uint8_t i = 0; i < params.size(); i++ )
		sendParam(i);
}

void Smarty::askConnData() {
	LOGln("Asking new WiFi & Server data");
	jsonDoc["header"] = (uint8_t)GIVE_ME_DATA;
	jsonDoc["mac"] = WiFi.macAddress();
	send();
}

void Smarty::messageHandler() {
	switch ( (uint8_t)jsonDoc["header"] ) {
		case MY_NAME:
			LOGln("What?");
			break;
		case SET_VALUE:
			setTargetVal(jsonDoc["num"], jsonDoc["targetValue"]);
			break;
		case GIVE_ME_VALUES:
			sendAllParams();
			break;
		case SERVER_HERE: {
			IPAddress _ip;
			_ip.fromString( (const char*)jsonDoc["serverIP"] );
			conn_data.serverIP[0] = _ip[0];
			conn_data.serverIP[1] = _ip[1];
			conn_data.serverIP[2] = _ip[2];
			conn_data.serverIP[3] = _ip[3];
			lastDisconnectTime = 0 - SERVER_RECONNECT_INTERVAL;
			break;
		}
		case GIVE_ME_DATA:
			if ( isESPBase ) {
				jsonDoc["ssid"] = String(conn_data.ssid);
				jsonDoc["pass"] = String(conn_data.pass);
				jsonDoc["serverIP"] = IPAddress(conn_data.serverIP).toString();
				jsonDoc["port"] = conn_data.port;
			}
			break;
		case TAKE_CONN_DATA: {
			IPAddress _ip;
			_ip.fromString( (const char*)jsonDoc["serverIP"] );
			strcpy(conn_data.ssid, jsonDoc["ssid"]);
			strcpy(conn_data.pass, jsonDoc["pass"]);
			conn_data.serverIP[0] = _ip[0];
			conn_data.serverIP[1] = _ip[1];
			conn_data.serverIP[2] = _ip[2];
			conn_data.serverIP[3] = _ip[3];
			conn_data.port = jsonDoc["port"];
			conn_status.newDataTestMode = true;
			conn_status.getConnDataMode = false;
			WiFi.begin(conn_data.ssid, conn_data.pass);
			break;
		}
		default:
			break;
	}
}