#include <Ticker.h>
#include <ESP8266WiFi.h>
#include "Smarty.h"

#define RED_PIN D4
#define GREEN_PIN D2
#define BLUE_PIN D3
#define WHITE_PIN D1
#define POWER_PIN D0

void f1(param_value_t val) {
  LOGln("shit");
}

Smarty smarty("Kitchen_light", \
              "Управляет освещением над столешницей и в цоколе кухни.", \
              "Eric's AP 2.4G", \
              "19881989", \
              IPAddress(192, 168, 1, 162), \
              3333 \
              );

void setup (void) {
  smarty.addParam(SWITCH, "Типа чё-то как-то делает", 0, 0, false, f1);
  smarty.begin();
}

void loop (void) {
  smarty.checkConnection();
  //smarty.setValue(0, 12);
  delay(1000);
  yield();
}