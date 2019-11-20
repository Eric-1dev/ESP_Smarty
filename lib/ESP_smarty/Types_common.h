#ifndef __TYPES_COMMON_H__
#define __TYPES_COMMON_H__

#include <stdint.h>

enum headers : uint8_t {
  WHERE_IS_SERVER,              // Finding server with broadcast message
  GIVE_ME_VALUES,               // Servers asking all values
  MY_NAME,                      // Hello message (from client to server on connect)
  MY_PARAMS,                    // List of existing parameters (one by one)
  NEW_VALUE,                    // On value change
  GIVE_ME_DATA,                 // Client asking WiFi data from ESP-BASE
  TAKE_CONN_DATA,               // ESP_BASE tell new connection data to client
  SERVER_HERE,                  // Server answer on WHERE_IS_SERVER
  SET_VALUE                     // Server sending new TARGET VALUE
};

enum paramType_t : uint8_t {
  SWITCH,
  RGB,
  NUMBER,
  NONE
};

typedef int16_t param_value_t;

#endif