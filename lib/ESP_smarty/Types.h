#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>

enum headers : uint8_t {
  WHERE_IS_SERVER,              // Finding server with broadcast message
  GIVE_ME_VALUES,               // Servers asking all values
  MY_NAME,                      // Hello message (from client to server on connect)
  MY_PARAMS,                    // List of existing parameters (one by one)
  NEW_VALUE,                    // On value change
  GIVE_ME_DATA,                 // Client asking WiFi data from ESP-BASE
  SERVER_HERE,                  // Server answer on WHERE_IS_SERVER
  SET_VALUE                     // Server sending new TARGET VALUE
};

enum paramType_t : uint8_t {
  SWITCH,
  RGB,
  NUMBER,
  NONE
};

typedef struct {
	char ssid[32];
	char pass[64];
  uint8_t serverIP[4];
  uint16_t port;
} conn_data_t;

typedef struct {
  uint8_t hardcoded_data : 1;         // Hardcoded connection data (ssid, password, server IP, port)
  uint8_t getConnDataMode : 1;        // In this mode ESP tries to get new connection data from ESP-BASE
  uint8_t triesleft : 5;
} conn_status_t;

typedef int16_t param_value_t;

typedef struct {
    uint8_t num;
    char desc[128];
    uint8_t type;
    param_value_t curValue;
    bool remember_target;
    param_value_t targetValue;
    param_value_t minValue;
    param_value_t maxValue;
    void (*callback) (param_value_t);
    int8_t divisor;
} Param;

#endif