#include <Ticker.h>
#include "Smarty.h"

#define RED_PIN       D4
#define GREEN_PIN     D2
#define BLUE_PIN      D3
#define WHITE_PIN     D1
#define RCV433        D5
#define LIGHT_SENSOR  A0
#define IR_SENSOR     D6

void f1(param_value_t val) {
  LOGln("He is call me!");
}

ICACHE_RAM_ATTR void ir_activity();

/*Smarty smarty("Kitchen_light", \
              "Управляет освещением над столешницей и в цоколе кухни.", \
              "Eric's AP 2.4G", \
              "19881989", \
              IPAddress(192, 168, 1, 162), \
              3333 \
              );
*/

Smarty smarty("Kitchen_light", "Управляет освещением над столешницей и в цоколе кухни.");

void setup (void) {
  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(RCV433, INPUT);
  pinMode(IR_SENSOR, INPUT);
  attachInterrupt(IR_SENSOR, ir_activity, RISING);
  smarty.addParam(SWITCH, "Включает подсветку столешницы", 0, 0, false, f1);
  smarty.addParam(RGB, "Включает RGB подсветку цоколя", 0, 0, false, NULL);
  smarty.begin();
}

void loop (void) {
  smarty.checkConnection();
  //Serial.printf("ADC val = %d, IR state = %d\n", analogRead(LIGHT_SENSOR), digitalRead(IR_SENSOR));
  //smarty.setValue(0, 12);

  delay(1000);
  yield();
}

void ir_activity() {
  uint16_t adc_val = analogRead(LIGHT_SENSOR);
  if ( adc_val < 200 ) {
    //Serial.printf("----------------------Light ON (ADC val = %d)---------------------\n", adc_val);
  }
}