#include "Smarty.h"
#include <Ticker.h>
#include <RCSwitch.h>

#define RED_PIN       D4
#define GREEN_PIN     D3
#define BLUE_PIN      D2
#define WHITE_PIN     D1
#define RCV433        D5
#define LIGHT_SENSOR  A0
#define IR_SENSOR     D6

void aWrite(uint8_t, int16_t);
void rgb_blink(void);
void tick(void);
void white_event(param_value_t);
void rgb_event(param_value_t);

void firstBut();
void secondBut();
void thirdBut();

RCSwitch mySwitch = RCSwitch();

Ticker timer;
uint8_t whiteTimerPeriod = 2;
uint32_t lastIRact = 0;
uint16_t adc_val;
bool rgb_auto = false;
bool whiteOn = false;
int16_t targetW = 0;
int16_t curW = 0;

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
  pinMode(WHITE_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  digitalWrite(WHITE_PIN, HIGH);
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, HIGH);
  
  analogWriteFreq(3000);

  timer.attach_ms(whiteTimerPeriod, tick);

  mySwitch.enableReceive(RCV433);
  
  smarty.addParam(SWITCH, "Включает подсветку столешницы", 0, 0, false, white_event);
  smarty.addParam(RGB, "Включает RGB подсветку цоколя", 0, 0, false, rgb_event);
  smarty.begin();
}

void loop (void) {
  smarty.checkConnection();

  if ( !whiteOn ) {
    if ( whiteTimerPeriod != 10 ) {
      whiteTimerPeriod = 10;
      timer.attach_ms(whiteTimerPeriod, tick);
    }

    adc_val = analogRead(LIGHT_SENSOR);
    if ( adc_val < 100 ) {
      if ( digitalRead(IR_SENSOR) ) {
        lastIRact = millis();
        targetW = 80;
      }
      else if ( millis() - lastIRact > 30000 )
        targetW = 0;
    }
    else {
      targetW = 0;
    }
  }
  else {
    if ( whiteTimerPeriod != 2 ) {
      whiteTimerPeriod = 2;
      timer.attach_ms(whiteTimerPeriod, tick);
    }
  }

  if (mySwitch.available()) {
    if ( mySwitch.getReceivedProtocol() == 1 ) {
      switch (mySwitch.getReceivedValue()) {
        case 69105:
          firstBut();
          break;
        case 69106:
          secondBut();
          break;
        case 69108:
          thirdBut();
          break;
        default:
          break;
      }
    }
    mySwitch.resetAvailable();
  }

  delay(2);
  yield();
}

void aWrite(uint8_t _pin, int16_t _val) {
  analogWrite(_pin, pow(1023-_val,2)/1023);
}

void white_event(param_value_t _val) {
  whiteOn = _val;
  if ( _val ) {
    targetW = 1023;
  }
  else {
    targetW = 0;
  }
}

void rgb_event(param_value_t _val) {
  if ( _val == -32768 )
    rgb_auto = true;
  else {
    rgb_auto = false;
  }
}

void tick(void) {
  if ( targetW > curW )
    curW++;
  else if ( targetW < curW )
    curW--;
    
  aWrite(WHITE_PIN, curW);
}

void firstBut() {
  Serial.println("1 Button pressed");
  whiteOn = !whiteOn;
  if ( whiteOn )
    targetW = 1023;
  else
    targetW = 0;
}

void secondBut() {
  Serial.println("2 Button pressed");
}

void thirdBut() {
  Serial.println("3 Button pressed");
}