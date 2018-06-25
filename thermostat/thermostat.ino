#include <stdio.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include "Adafruit_MCP23017.h"
#include <mqtt-wrapper.h>

#include "i3Logo.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifndef NAME
#define NAME "NEW-thermostat"
#endif

#ifndef TOPIC
#define TOPIC "i3/program-me/NEW-thermostat"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID "i3detroit-wpa"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "i3detroit"
#endif

#ifndef MQTT_SERVER
#define MQTT_SERVER "10.13.0.22"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

//Set if no cool
#ifndef HEAT_ONLY
//#define HEAT_ONLY
#endif


#define DEBUG

#ifdef DEBUG
# define DEBUG_PRINT(x) Serial.print(x);
# define DEBUG_PRINTLN(x) Serial.println(x);
#else
# define DEBUG_PRINT(x) do {} while (0)
# define DEBUG_PRINTLN(x) do {} while (0)
#endif

//magic numbers stored in eeprom to set boot state
#define MAGIC_EEPROM_NUMBER 0x42
#define EEPROM_OFF 0
#define EEPROM_COOL 1
#define EEPROM_HEAT 2

//default temps to go to based on state
#define DEFAULT_HEAT_TEMP 10
#define DEFAULT_COOL_TEMP 25

//limits for target temp; and will always heat below min temp
#define MIN_TEMP 5
#define MAX_TEMP 42

#define FAN_CONTROL 5 //force on fan, 0 is on. If 1, is auto, furnace will tunr on fan
#define ENABLE_CONTROL 6 //enable/disable heat/cool
#define SELECT_CONTROL 7 //heat:0 cool: 1

#define RESET 2

#define FAN_ON 7
#define FAN_AUTO 6
#define LED_FAN_ON 4
#define LED_FAN_AUTO 3

#define TEMP_UP 0
#define TEMP_DOWN 1

#define HEAT 5
#define LED_HEAT 2

#define OFF 4
#define LED_OFF 1

#define COOL 3
#define LED_COOL 0

uint8_t button_state;
int button_state_last[] = {1,1,1,1,1,1,1,1};
int debounce[] = {0,0,0,0,0,0,0,0};
const int debounce_time = 50;

char buf[1024];
char topicBuf[1024];

char pressureBuf[16];
char humidityBuf[16];
char tempBuf[16];

struct mqtt_wrapper_options mqtt_options;

Adafruit_MCP23017 mcp;

unsigned long nextStatus = 0UL;
unsigned long statusInterval = 60000UL;

unsigned long nextRead = 0UL;
unsigned long readInterval = 1000UL;

unsigned long nextTimeout = 0UL;
unsigned long timeoutInterval = 1000*3*60*60;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R1, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 5, /* data=*/ 4);
Adafruit_BME280 bme;
bool bmeConnected = true;

bool mqttConnected = false;

//swing temp how far above or below the target has to go to actually control
uint8_t swing;

float targetTemp;
float currentTemp;
bool fanForced;
bool heat;
bool cool;

//Currently outputting
bool enabled;
bool heatCool;
bool currentFanForced;

bool displayDirty;
bool stateDirty;
bool mqttDirty;

uint8_t change_bit(uint8_t val, uint8_t num, bool bitval) {
  return (val & ~(1<<num)) | (bitval << num);
}




//Also used for init
void resetState() {
  swing = EEPROM.read(2);
  byte value = EEPROM.read(1);
  Serial.println("reset");
  Serial.println(value);
  switch((int)value) {
#ifndef HEAT_ONLY
    case EEPROM_COOL:
      Serial.println("COOL");
      targetTemp = DEFAULT_COOL_TEMP;
      heat = false;
      cool = true;
      break;
#endif
    case EEPROM_HEAT:
      Serial.println("HEAT");
      targetTemp = DEFAULT_HEAT_TEMP;
      heat = true;
      cool = false;
      break;
    case EEPROM_OFF:
    default:
      Serial.println("OFF");
      targetTemp = DEFAULT_HEAT_TEMP;
      heat = false;
      cool = false;
      break;
  }
  if(!bmeConnected) {
    heat = false;
    cool = false;
  }
  fanForced = false;
  displayDirty = true;
  stateDirty = true;
}

uint8_t setHeat(uint8_t toWrite) {
  if(!enabled || heatCool == 1) {
    mqttDirty = true;
  }
  enabled = true;
  heatCool = 0;
  toWrite = change_bit(toWrite, 7-ENABLE_CONTROL, 1);
  toWrite = change_bit(toWrite, 7-SELECT_CONTROL, 0);
  toWrite = change_bit(toWrite, 7-FAN_CONTROL, 1);
  return toWrite;
}

uint8_t setCool(uint8_t toWrite) {
  if(!enabled || heatCool == 0) {
    mqttDirty = true;
  }
  enabled = true;
  heatCool = 1;
  toWrite = change_bit(toWrite, 7-ENABLE_CONTROL, 1);
  toWrite = change_bit(toWrite, 7-SELECT_CONTROL, 1);
  toWrite = change_bit(toWrite, 7-FAN_CONTROL, 1);
  return toWrite;
}
uint8_t setOff(uint8_t toWrite) {
  if(enabled) {
    mqttDirty = true;
  }
  enabled = false;
  return change_bit(toWrite, 7-ENABLE_CONTROL, 0);
}

void doControl() {
  uint8_t toWrite = 0;

  //status lights
  toWrite = change_bit(toWrite, 7-LED_FAN_ON, fanForced);
  toWrite = change_bit(toWrite, 7-LED_FAN_AUTO, !fanForced);
  toWrite = change_bit(toWrite, 7-LED_HEAT, heat);
  toWrite = change_bit(toWrite, 7-LED_COOL, cool);
  toWrite = change_bit(toWrite, 7-LED_OFF, !heat && !cool);
  //The status LEDs are pulled down to turn them on,
  //the rest is set high to turn it on
  //force fan on
  toWrite = change_bit(toWrite, 7-FAN_CONTROL, fanForced);

  // heating logic
  /*
   * if heat
   *
   *    if(current + swing < target)
   *        below temp, heat
   *        start heat
   *    if(current - swing > target)
   *        above
   *        stop
   *    if(current > target-swing && current < target+swing && enabled)
   *        heating through swing
   *        heat
   *    if(current > target-swing && current < target+swing && !enabled)
   *        //cooling through swing
   *        stop
   *    else
   *        there is no else
   *
   *
   */
  if(currentTemp < MIN_TEMP) {
    DEBUG_PRINTLN("under min temp");
    toWrite = setHeat(toWrite);
  } else if(heat) {
    DEBUG_PRINT("swing ");
    DEBUG_PRINTLN(swing);
    DEBUG_PRINT("currentTemp ");
    DEBUG_PRINTLN(currentTemp);
    DEBUG_PRINT("targettTemp ");
    DEBUG_PRINTLN(targetTemp);
    DEBUG_PRINTLN(currentTemp + swing <= targetTemp);
    if(currentTemp + swing <= targetTemp || (abs(currentTemp-targetTemp) < swing && enabled)) {
      toWrite = setHeat(toWrite);
      DEBUG_PRINTLN("heat on");
    } else  if(currentTemp - swing >= targetTemp || (abs(currentTemp-targetTemp) < swing && !enabled)) {
      toWrite = setOff(toWrite);
      DEBUG_PRINTLN("heat off");
    } else {
      DEBUG_PRINTLN("HEAT ELSE WRONG");
    }
#ifndef HEAT_ONLY
  } else if(cool) {
    DEBUG_PRINT("enabled ");
    DEBUG_PRINTLN(enabled);
    DEBUG_PRINT("swing ");
    DEBUG_PRINTLN(swing);
    DEBUG_PRINT("currentTemp ");
    DEBUG_PRINTLN(currentTemp);
    DEBUG_PRINT("targettTemp ");
    DEBUG_PRINTLN(targetTemp);
    DEBUG_PRINTLN(currentTemp + swing <= targetTemp);
    if(currentTemp - swing >= targetTemp || (abs(currentTemp-targetTemp) < swing && enabled)) {
      toWrite = setCool(toWrite);
      DEBUG_PRINTLN("cool on");
    } else if(currentTemp + swing <  targetTemp || (abs(currentTemp-targetTemp) < swing && !enabled)) {
      DEBUG_PRINTLN("cool off");
      toWrite = setOff(toWrite);
    } else {
      DEBUG_PRINTLN("COOL ELSE WRONG");
    }
#endif
  } else {
    DEBUG_PRINTLN("off");
    //off
    toWrite = setOff(toWrite);
  }

  // //write control
  //Serial.println("control update");
  //Serial.println(fanForced);
  //Serial.println(heat);
  //Serial.println(cool);
  //Serial.println(toWrite, BIN);

  /*
   * EN  SEL | H  C
   * 0   0   | 0  1
   * 0   1   | 1  0
   * 1   0   | 0  0
   * 1   1   | 0  0
   */

  writeLow(toWrite);
  //Serial.println(writeLow(toWrite), BIN);
}

void handleButton(int button) {
  nextTimeout = millis() + timeoutInterval;

  stateDirty = true;
  switch(button) {
    case RESET:
      DEBUG_PRINTLN("button RESET");
      resetState();
      break;
    case FAN_ON:
      DEBUG_PRINTLN("button FAN_ON");
      fanForced = true;
      break;
    case FAN_AUTO:
      DEBUG_PRINTLN("button FAN_AUTO");
      fanForced = false;
      break;
    case TEMP_UP:
      DEBUG_PRINTLN("button TEMP_UP");
      if(++targetTemp > MAX_TEMP) {
        targetTemp = MAX_TEMP;
      }
      displayDirty = true;
      break;
    case TEMP_DOWN:
      DEBUG_PRINTLN("button TEMP_DOWN");
      if(--targetTemp < MIN_TEMP) {
        targetTemp = MIN_TEMP;
      }
      displayDirty = true;
      break;
    case HEAT:
      DEBUG_PRINTLN("button HEAT");
      EEPROM.write(1, EEPROM_HEAT);
      EEPROM.commit();
      heat = true;
      cool = false;
      break;
    case OFF:
      DEBUG_PRINTLN("button OFF");
      EEPROM.write(1, EEPROM_OFF);
      EEPROM.commit();
      heat = false;
      cool = false;
      break;
#ifndef HEAT_ONLY
    case COOL:
      DEBUG_PRINTLN("button COOL");
      EEPROM.write(1, EEPROM_COOL);
      EEPROM.commit();
      heat = false;
      cool = true;
      break;
#endif
  }
}

void display() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_inr38_mf);
  itoa(targetTemp, buf, 10);
  u8g2.drawStr(0, 116, buf);
  if(bmeConnected) {
    itoa(currentTemp, buf, 10);
    u8g2.drawStr(0, 50, buf);
  } else {
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.drawStr(0,30,"No BME");
    u8g2.drawStr(0,50,"temp 10");
  }
  u8g2.setFont(u8g2_font_unifont_t_symbols);
  if(mqttConnected) {
    u8g2.drawGlyph(55, 10, 0x25C8);	/* dec 9672/hex 25C8 some dot */
  } else {
    u8g2.drawGlyph(55, 10, 0x25C7);	/* dec 9671/hex 25C7 some dot */
  }
  u8g2.sendBuffer();
}

void readTemp() {
  if(bmeConnected) {
    currentTemp = bme.readTemperature();
    dtostrf(currentTemp, 0, 2, tempBuf);
  } else {
    currentTemp = 10;
  }
  displayDirty = true;
  stateDirty = true;
}

void reportState(PubSubClient *client) {
  if(bmeConnected) {
    dtostrf(bme.readPressure(), 0, 2, pressureBuf);
    dtostrf(bme.readHumidity(), 0, 1, humidityBuf);
    sprintf(topicBuf, "tele/%s/bme280", TOPIC);
    sprintf(buf, "{\"Temperature\":%s, \"Pressure\":%s, \"Humidity\":%s}", tempBuf, pressureBuf, humidityBuf);
    client->publish(topicBuf, buf);
  } else {
    sprintf(topicBuf, "tele/%s/error", TOPIC);
    client->publish(topicBuf, "bme280 DISCONNECTED");
  }

  sprintf(topicBuf, "tele/%s/request", TOPIC);
  sprintf(buf, "{\"TargetTemp\":");
  itoa(targetTemp, buf + strlen(buf), 10);
  //TODO: move all these to publish to their topics if changed
  //TODO: status
  sprintf(buf + strlen(buf), ", \"requested\":\"%s\", \"fan\":\"%s\"}", (!heat && !cool) ? "off" : heat ? "heat" : "cool", fanForced ? "on" : "auto");
  client->publish(topicBuf, buf);

  sprintf(topicBuf, "tele/%s/output", TOPIC);
  //TODO: Remove swing
  //TODO: Fan as on/off not on/auto
  sprintf(buf, "{\"output\":\"%s\", \"fan\":\"%s\", \"swing\":%d}", !enabled ? "off" : heatCool ? "cool" : "heat", fanForced ? "on" : "auto", swing);
  client->publish(topicBuf, buf);
}

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  if(length == 0) {
    reportState(client);
    return;
  }
  //Topics
  //    target temp
  //    mode: heat|cool|off
  //    fan: auto|on
  //    swing: uint8_t
  if (strcmp(topic, "target") == 0) {
    payload[length] = '\0';
    sprintf(topicBuf, "stat/%s/target", TOPIC);
    targetTemp = atoi((char*)payload);
    if(targetTemp < MIN_TEMP) {
      targetTemp = MIN_TEMP;
    }
    if(targetTemp > MAX_TEMP) {
      targetTemp = MAX_TEMP;
    }
    nextTimeout = millis() + timeoutInterval;
    stateDirty = true;
    displayDirty = true;
    itoa(targetTemp, buf, 10);
    client->publish(topicBuf, buf);
  } else if (strncmp(topic, "mode", length - 1) == 0) {
    stateDirty = true;
    sprintf(topicBuf, "stat/%s/mode", TOPIC);
    if(strncmp((char*)payload, "heat", length-1) == 0) {
      heat = true;
      cool = false;
      EEPROM.write(1, EEPROM_HEAT);
      EEPROM.commit();
      nextTimeout = millis() + timeoutInterval;
      client->publish(topicBuf, "heat");
    } else if(strncmp((char*)payload, "cool", length-1) == 0) {
#ifndef HEAT_ONLY
      heat = false;
      cool = true;
      EEPROM.write(1, EEPROM_COOL);
      EEPROM.commit();
      nextTimeout = millis() + timeoutInterval;
      client->publish(topicBuf, "cool");
#else
      client->publish(topicBuf, "cool not supported");
#endif
    } else if(strncmp((char*)payload, "off", length-1) == 0) {
      heat = false;
      cool = false;
      EEPROM.write(1, EEPROM_OFF);
      EEPROM.commit();
      nextTimeout = millis() + timeoutInterval;
      client->publish(topicBuf, "off");
    } else {
      client->publish(topicBuf, "bad command");
    }
  } else if (strncmp(topic, "fan", length - 1) == 0) {
    stateDirty = true;
    sprintf(topicBuf, "stat/%s/fan", TOPIC);
    if(strncmp((char*)payload, "auto", 4) == 0) {
      fanForced = false;
      nextTimeout = millis() + timeoutInterval;
      client->publish(topicBuf, "auto");
    } else if(strncmp((char*)payload, "on", 2) == 0) {
      fanForced = true;
      nextTimeout = millis() + timeoutInterval;
      client->publish(topicBuf, "on");
    } else {
      client->publish(topicBuf, "bad command");
    }
  } else if (strncmp(topic, "swing", length - 1) == 0) {
    payload[length] = '\0';
    uint8_t newSwing = atoi((char*)payload);
    sprintf(topicBuf, "stat/%s/swing", TOPIC);
    if(newSwing <= 3 && newSwing >= 0) {
      swing = newSwing;
      EEPROM.write(2, swing);
      EEPROM.commit();
      sprintf(buf, "%d", swing);
      client->publish(topicBuf, buf);
    } else {
      client->publish(topicBuf, "new swing out of range 0-3");
    }
  } else {
    sprintf(topicBuf, "stat/%s/what", TOPIC);
    client->publish(topicBuf, "bad command");
  }
}

void connectSuccess(PubSubClient* client, char* ip) {
  //Are subscribed to cmnd/fullTopic/+
  // u8g2.clearBuffer();
  // u8g2.drawStr(0,10,ip);
  // u8g2.sendBuffer();
  readTemp();
  reportState(client);
  mqttConnected = true;
}


void setup() {
  Serial.begin(115200);
  Serial.println("Starting");

  EEPROM.begin(3);
  if(EEPROM.read(0) != MAGIC_EEPROM_NUMBER) {
      EEPROM.write(0, MAGIC_EEPROM_NUMBER);
      EEPROM.write(1, EEPROM_OFF);
      EEPROM.write(2, 1);//swing
  }
  resetState();

  // --- Display ---
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_inr38_mf);
  u8g2.drawXBM( 0, 0, i3Logo_width, i3Logo_height, i3Logo_bits);
  u8g2.sendBuffer();
  delay(500);

  // --- MQTT ---
  mqtt_options.connectedLoop = connectedLoop;
  mqtt_options.callback = callback;
  mqtt_options.connectSuccess = connectSuccess;
  mqtt_options.ssid = WIFI_SSID;
  mqtt_options.password = WIFI_PASSWORD;
  mqtt_options.mqtt_server = MQTT_SERVER;
  mqtt_options.mqtt_port = MQTT_PORT;
  mqtt_options.host_name = NAME;
  mqtt_options.fullTopic = TOPIC;
  mqtt_options.debug_print = true;
  setup_mqtt(&mqtt_options);

  // --- Sensors ---
  if (!bme.begin(0x76)) {
    Serial.println("Could not find BMP 280 sensor");
    bmeConnected = false;
    heat = false;
    cool = false;
  }

  mcp.begin();

  for(int i=0; i<8; ++i) {
    mcp.pinMode(i, OUTPUT);
  }
  for(int i=8; i<16; ++i) {
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, HIGH);  // turn on a 100K pullup internally
  }
  writeLow(0b00000101);
  delay(100);
  writeLow(0xFF);
}
uint16_t writeHigh(uint8_t high) {
  uint16_t a = mcp.readGPIOAB();
  a = high << 8 | (a & 0x00FF);
  mcp.writeGPIOAB(a);
  return a;
}
uint16_t writeLow(uint8_t low) {
  uint16_t a = mcp.readGPIOAB();
  a = low | (a & 0xFF00);
  mcp.writeGPIOAB(a);
  return a;
}
uint8_t readHigh() {
  return mcp.readGPIOAB() << 8;
}
uint8_t readLow() {
  return mcp.readGPIOAB() & 0x00FF;
}


void connectedLoop(PubSubClient* client) {
  if( (long)( millis() - nextStatus ) >= 0 || displayDirty || mqttDirty) {
    mqttDirty = false;
    nextStatus = millis() + statusInterval;
    reportState(client);
  }
}


void loop() {
  loop_mqtt();
  if( (long)( millis() - nextRead ) >= 0) {
    nextRead = millis() + readInterval;
    readTemp();
  }
  if( (long)( millis() - nextTimeout ) >= 0) {
    nextTimeout = millis() + timeoutInterval;
    resetState();
  }

  //mqtt loop called before this
  if(displayDirty) {
    displayDirty = false;
    display();
  }

  if(stateDirty) {
    doControl();
    stateDirty = false;
  }

  for(int i=0; i < 8; ++i) {
    button_state = mcp.digitalRead(i+8);
    if(button_state != button_state_last[i] && millis() - debounce[i] > debounce_time) {
      if(button_state == LOW) {
        handleButton(i);
      }
      button_state_last[i] =  button_state;
      debounce[i] = millis();
    }
  }
}
