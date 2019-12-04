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

#define LOW_WHITE     80
#define BUT_DELAY     500

void aWrite(uint8_t, int16_t);
void rgb_tick(void);
void white_tick(void);
void white_event(param_value_t);
void rgb_event(param_value_t);
void butPressed(uint32_t);
void rgbSetColor(int16_t, int16_t, int16_t);

RCSwitch mySwitch = RCSwitch();

Ticker white_timer;
Ticker rgb_timer;

uint8_t leds_arr[3] = { BLUE_PIN, GREEN_PIN, RED_PIN };
int16_t cPin1 = leds_arr[0], cPin2 = leds_arr[1], cPin3 = leds_arr[2];

uint16_t i = 0;
uint8_t j = 1;

uint8_t whiteTimerPeriod = 2;
uint32_t lastIRact = 0;
uint16_t adc_val;
uint8_t rgb_mode = 0;
bool whiteManualMode = false;
bool singleFlag = false;
int16_t targetW = 0;
int16_t targetR = 0;
int16_t targetG = 0;
int16_t targetB = 0;
int16_t curW = 0;
int16_t curR = 0;
int16_t curG = 0;
int16_t curB = 0;
uint32_t but1_time = 0, but2_time = 0, but3_time = 0;

Smarty smarty("Kitchen_light1", \
                "Управляет освещением над столешницей и в цоколе кухни.", \
                "Eric's AP 2.4G", \
                "19881989", \
                IPAddress(192, 168, 1, 162), \
                3333 \
                );

//Smarty smarty("Kitchen_light", "Управляет освещением над столешницей и в цоколе кухни.");

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

  analogWriteFreq(500);

  white_timer.attach_ms(whiteTimerPeriod, white_tick);

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
      white_timer.attach_ms(whiteTimerPeriod, white_tick);
    }

    adc_val = analogRead(LIGHT_SENSOR);
    if ( adc_val < 100 && !rgb_mode ) {
      if ( digitalRead(IR_SENSOR) || singleFlag ) {
        singleFlag = false;
        lastIRact = millis();
        targetW = LOW_WHITE;
      }
      else if ( millis() - lastIRact > 30000 )
        targetW = 0;
    }
    else {
      targetW = 0;
    }
  }
  else {
    if ( whiteTimerPeriod != 1 ) {
      whiteTimerPeriod = 1;
      white_timer.attach_ms(whiteTimerPeriod, white_tick);
    }
  }

  if ( curW == 0 && whiteManualMode && !rgb_mode )
    whiteManualMode = false;

  if (mySwitch.available()) {
    if ( mySwitch.getReceivedProtocol() == 1 )
      butPressed(mySwitch.getReceivedValue());
    mySwitch.resetAvailable();
  }

  delay(10);
  yield();
}

void aWrite(uint8_t _pin, int16_t _val) {
  analogWrite(_pin, pow(1023-_val,3)/1046529);
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
    rgb_mode = 1;
  else {
    rgb_mode = false;
  }
}

void white_tick(void) {
  if ( targetW > curW )
    curW++;
  else if ( targetW < curW )
    curW--;
  
  aWrite(WHITE_PIN, curW);
}

void rgb_tick(void) {
  if ( rgb_mode == 1 ) {
    aWrite(cPin1, 0);
    aWrite(cPin2, 1023-i);
    aWrite(cPin3, i);

    i++;
    if ( i > 1023 ) {
      i = 0;
      cPin1 = leds_arr[j++%3];
      cPin2 = leds_arr[j++%3];
      cPin3 = leds_arr[j++%3];
      j++;
      if ( j > 8 ) j = 0;
    }
  }
  else {
    if ( targetR > curR )
      curR++;
    else if ( targetR < curR )
      curR--;
  
    if ( targetG > curG )
      curG++;
    else if ( targetG < curG )
      curG--;
  
    if ( targetB > curB )
      curB++;
    else if ( targetB < curB )
      curB--;
  
    aWrite(RED_PIN, curR);
    aWrite(GREEN_PIN, curG);
    aWrite(BLUE_PIN, curB);
  }
}

void butPressed(uint32_t _but) {
  switch (_but) {
    case 69105:       // Button 1
      if ( millis() - but1_time > BUT_DELAY ) {
        but1_time = millis();
        whiteManualMode = true;
        if ( targetW == 0 || targetW == LOW_WHITE ) {
          targetW = 1023;
          smarty.setValue(0, 1);
        }
        else {
          targetW = 0;
          smarty.setValue(0, 1);
        }
      }
      break;
    case 69106:       // Button 2
      if ( millis() - but2_time > BUT_DELAY ) {
        but2_time = millis();
        rgb_mode++;
        if ( rgb_mode > 6 )
          rgb_mode = 0;
        switch (rgb_mode) {
          case 0:
            rgbSetColor(0, 0, 0);
            singleFlag = true;
            break;
          case 1:
            rgb_timer.attach_ms(8, rgb_tick);
            smarty.setValue(1, -32768);
            break;
          case 2:
            rgbSetColor(0, 0, 1023);
            break;
          case 3:
            rgbSetColor(0, 1023, 0);
            break;
          case 4:
            rgbSetColor(1023, 0, 0);
            break;
          case 5:
            rgbSetColor(0, 1023, 1023);
            break;
          case 6:
            rgbSetColor(1023, 1023, 0);
            break;
          default:
            break;
        }
      }
      break;
    case 69108:       // Button 3
      if ( millis() - but3_time > BUT_DELAY ) {
        but3_time = millis();
        rgb_mode = 0;
        rgbSetColor(0, 0, 0);
        singleFlag = true;
      }
      break;
    default:
      break;
  }
}

void rgbSetColor(int16_t _r, int16_t _g, int16_t _b) {
  rgb_timer.attach_ms(1, rgb_tick);
  targetR = _r;
  targetG = _g;
  targetB = _b;
  smarty.setValue(1, _r);
}