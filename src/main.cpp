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

void butPressed(uint32_t);

RCSwitch mySwitch = RCSwitch();

Ticker timer;
uint8_t whiteTimerPeriod = 2;
uint32_t lastIRact = 0;
uint16_t adc_val;
bool rgb_auto = false;
bool whiteManualMode = false;
int16_t targetW = 0;
int16_t curW = 0;
uint32_t but1_time = 0, but2_time = 0, but3_time = 0;

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

  if ( !whiteManualMode ) {
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

  if ( curW == 0 && whiteManualMode )
    whiteManualMode = 0;

  if (mySwitch.available()) {
    if ( mySwitch.getReceivedProtocol() == 1 )
      butPressed(mySwitch.getReceivedValue());
    mySwitch.resetAvailable();
  }

  delay(10);
  yield();
}

void aWrite(uint8_t _pin, int16_t _val) {
  analogWrite(_pin, pow(1023-_val,2)/1023);
}

void white_event(param_value_t _val) {
  whiteManualMode = _val;
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

void butPressed(uint32_t _but) {
  switch (_but) {
    case 69105:       // Button 1
      if ( millis() - but1_time > 500 ) {
        but1_time = millis();
        whiteManualMode = true;
        if ( whiteManualMode )
          targetW = 1023;
        else
          targetW = 0;
      }
      break;
    case 69106:       // Button 2
      break;
    case 69108:       // Button 3
      break;
    default:
      break;
  }
}