#include <Ticker.h>
#include "Smarty.h"
#include "ESPiLight.h"

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

void rfCallback(const String &protocol, const String &message, int status, size_t repeats, const String &deviceID);

ESPiLight rf(-1);
Ticker timer;
uint8_t whiteTimerPeriod = 2;
uint32_t lastIRact = 0;
uint16_t adc_val;
bool rgb_auto = false;
bool manualMode = false;
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
  
  analogWriteFreq(300);

  timer.attach_ms(whiteTimerPeriod, tick);

  rf.setCallback(rfCallback);
  rf.initReceiver(RCV433);
  
  smarty.addParam(SWITCH, "Включает подсветку столешницы", 0, 0, false, white_event);
  smarty.addParam(RGB, "Включает RGB подсветку цоколя", 0, 0, false, rgb_event);
  smarty.begin();
}

void loop (void) {
  smarty.checkConnection();

  rf.loop();

  if ( !manualMode ) {
    if ( whiteTimerPeriod != 50 ) {
      whiteTimerPeriod = 50;
      timer.detach();
      timer.attach_ms(whiteTimerPeriod, tick);
    }

    adc_val = analogRead(LIGHT_SENSOR);
    if ( adc_val < 200 ) {
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
      timer.detach();
      timer.attach_ms(whiteTimerPeriod, tick);
    }
  }

  delay(10);
  yield();
}

void aWrite(uint8_t _pin, int16_t _val) {
  analogWrite(_pin, pow(1023-_val,2)/1023);
}

void white_event(param_value_t _val) {
  manualMode = _val;
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

void rfCallback(const String &protocol, const String &message, int status,
                size_t repeats, const String &deviceID) {
  Serial.print("RF signal arrived [");
  Serial.print(protocol);  // protocoll used to parse
  Serial.print("][");
  Serial.print(deviceID);  // value of id key in json message
  Serial.print("] (");
  Serial.print(status);  // status of message, depending on repeat, either:
                         // FIRST   - first message of this protocoll within the
                         //           last 0.5 s
                         // INVALID - message repeat is not equal to the
                         //           previous message
                         // VALID   - message is equal to the previous message
                         // KNOWN   - repeat of a already valid message
  Serial.print(") ");
  Serial.print(message);  // message in json format
  Serial.println();

  // check if message is valid and process it
  if (status == VALID) {
    Serial.print("Valid message: [");
    Serial.print(protocol);
    Serial.print("] ");
    Serial.print(message);
    Serial.println();
  }
}