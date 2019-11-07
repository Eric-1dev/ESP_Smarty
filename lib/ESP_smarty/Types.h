#ifndef __TYPES_H__
#define __TYPES_H__

#define WHERE_IS_SERVER         "where is server"
#define GIVE_ME_VALUES          "give me values"
#define MY_NAME                 "hello, it's me"
#define MY_PARAMS               "so what i can do"
#define NEW_VALUE               "look, my value has changed"

struct conn_data_t {
	char ssid[32];
	char pass[64];
  uint8_t serverIP[4];
  uint16_t port;
};

enum paramType_t {
  SWITCH,
  RGB,
  NUMBER,
  NONE
};

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