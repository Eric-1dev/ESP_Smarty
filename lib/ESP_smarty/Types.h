#ifndef __TYPES_H__
#define __TYPES_H__

#define WHERE_IS_SERVER         "where is server"
#define GIVE_ME_VALUES          "give me values"
#define MY_NAME                 "hello, it's me"
#define MY_PARAMS               "so what i can do"
#define NEW_VALUE               "look, my value has changed"
#define GIVE_ME_DATA            "give me wifi data"

enum paramType_t {
  SWITCH,
  RGB,
  NUMBER,
  NONE
};

typedef struct {
	char ssid[32];
	char pass[64];
  IPAddress serverIP;
  uint16_t port;
} conn_data_t;

typedef struct {
  uint8_t wifi_connected : 1;
  uint8_t got_ip : 1;
  uint8_t server_connected : 1;
  uint8_t hardcoded_data : 1;        // Hardcoded connection data (ssid, password, server IP, port)
  uint8_t getConnDataFlag : 1;
  uint8_t disconnect_counter : 3;
} conn_status_t;

typedef int16_t param_value_t;

typedef struct {
    char desc[128];
    paramType_t type;
    param_value_t curValue;
    bool remember_target;
    param_value_t targetValue;
    param_value_t minValue;
    param_value_t maxValue;
    void (*callback) (param_value_t);
    int8_t divisor;
} Param;

#endif