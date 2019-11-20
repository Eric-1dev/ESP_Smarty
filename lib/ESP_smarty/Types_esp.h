#ifndef __TYPES_ESP_H__
#define __TYPES_ESP_H__

#include <stdint.h>
#include "Types_common.h"

typedef struct {
	char ssid[32];
	char pass[64];
  uint8_t serverIP[4];
  uint16_t port;
} conn_data_t;

typedef struct {
  uint8_t gotIP : 1;
  uint8_t hardcoded_data : 1;         // Hardcoded connection data (ssid, password, server IP, port)
  uint8_t getConnDataMode : 1;        // In this mode ESP tries to get new connection data from ESP-BASE
  uint8_t newDataTestMode : 1;        // Testing new connection data. If all ok - remember in EEPROM
  uint8_t triesleft : 4;
} conn_status_t;

typedef struct {
    uint8_t num;
    char desc[128];
    uint8_t type;
    param_value_t curValue;
    uint8_t remember_target;
    param_value_t targetValue;
    param_value_t minValue;
    param_value_t maxValue;
    void (*callback) (param_value_t);
    int8_t divisor;
} Param;

#endif