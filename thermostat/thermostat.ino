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

uint32_t nextStatus = 0UL;
uint32_t statusInterval = 60000UL;

uint32_t nextRead = 0UL;
uint32_t readInterval = 1000UL;

uint32_t nextTimeout = 0UL;

union TimeoutInterval {
    uint32_t timeout;
    uint8_t octets[4];
};

#define DEFAULT_TIMEOUT_INTERVAL 3 * 60 * 60 * 1000UL

enum Mode { OFF = 0, HEAT, COOL };
struct ControlState {
  enum Mode mode;
  bool fan;
  int target;
  int swing;
  TimeoutInterval timeout;
};
struct SensorState {
  float temp;
  float pressure;
  float humidity;
  bool batteryPower;
  bool tempSensor; //if sensor connected
};
struct OutputState {
  enum Mode mode;
  uint32_t fanDelayEnd;
  bool fan;
};

struct ControlState controlState;
struct ControlState mqttControlState;

struct SensorState currentSensorState;

struct OutputState outputState;

#define DEBUG

#ifdef DEBUG
# define DEBUG_PRINT(x) Serial.print(x);
# define DEBUG_PRINTLN(x) Serial.println(x);
#else
# define DEBUG_PRINT(x) do {} while (0)
# define DEBUG_PRINTLN(x) do {} while (0)
#endif

//magic numbers stored in eeprom to set boot state
#define MAGIC_EEPROM_NUMBER 0x44

//default temps to go to based on state
#define DEFAULT_HEAT_TEMP 10
#define DEFAULT_COOL_TEMP 25

//limits for target temp; and will always heat below min temp
#define MIN_TEMP 5
#define MAX_TEMP 42

#define FAN_CONTROL 5 //force on fan, 0 is on. If 1, is auto, furnace will tunr on fan
#define ENABLE_CONTROL 6 //enable/disable heat/cool
#define SELECT_CONTROL 7 //heat:0 cool: 1

#define BTN_RESET 2

#define BTN_FAN_ON 7
#define BTN_FAN_AUTO 6
#define LED_FAN_ON 4
#define LED_FAN_AUTO 3

#define TEMP_UP 0
#define TEMP_DOWN 1

#define BTN_HEAT 5
#define LED_HEAT 2

#define BTN_OFF 4
#define LED_OFF 1

#define BTN_COOL 3
#define LED_COOL 0

uint8_t button_state;
int button_state_last[] = {1,1,1,1,1,1,1,1};
int debounce[] = {0,0,0,0,0,0,0,0};
const int debounce_time = 50;

char buf[1024];
char topicBuf[1024];

struct mqtt_wrapper_options mqtt_options;
enum ConnState connState;

Adafruit_MCP23017 mcp;


U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R1, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 5, /* data=*/ 4);
Adafruit_BME280 bme;

//swing temp how far above or below the target has to go to actually control


bool displayDirty;
bool stateDirty;
bool controlDirty;
bool outputDirty;

uint8_t change_bit(uint8_t val, uint8_t num, bool bitval) {
  return (val & ~(1<<num)) | (bitval << num);
}




//Also used for init
void resetState() {
  Serial.println("reset");
  controlState.swing = EEPROM.read(2);
  byte mode = EEPROM.read(1);
  Serial.println("start read timeout");
  for(int i=0; i<4; ++i) {
    controlState.timeout.octets[i] = EEPROM.read(3+i);
  }

  switch((int)mode) {
#ifndef HEAT_ONLY
    case COOL:
      Serial.println("COOL");
      controlState.target = DEFAULT_COOL_TEMP;
      controlState.mode = COOL;
      break;
#endif
    case HEAT:
      Serial.println("HEAT");
      controlState.target = DEFAULT_HEAT_TEMP;
      controlState.mode = HEAT;
      break;
    case OFF:
    default:
      Serial.println("OFF");
      controlState.target = DEFAULT_HEAT_TEMP;
      controlState.mode = OFF;
      break;
  }
  if(!currentSensorState.tempSensor) {
    controlState.mode = OFF;
  }
  controlState.fan = false;
  displayDirty = true;
  stateDirty = true;
}

uint8_t setHeat(uint8_t toWrite) {
  if(outputState.mode != HEAT) {
    outputDirty = true;
    DEBUG_PRINTLN("heat changed");
    outputState.fanDelayEnd = millis() + 30*1000;
    //We use if it's 0 to mean we're done waiting;
    if(outputState.fanDelayEnd == 0) {
      outputState.fanDelayEnd = 1;
    }
  }
  outputState.mode = HEAT;
  toWrite = change_bit(toWrite, 7-ENABLE_CONTROL, 1);
  toWrite = change_bit(toWrite, 7-SELECT_CONTROL, 0);
  return toWrite;
}

uint8_t setCool(uint8_t toWrite) {
  if(outputState.mode != COOL) {
    DEBUG_PRINTLN("cool changed");
    outputDirty = true;
    outputState.fanDelayEnd = millis() + 30*1000;
    //We use if it's 0 to mean we're done waiting;
    if(outputState.fanDelayEnd == 0) {
      outputState.fanDelayEnd = 1;
    }
  }
  outputState.mode = COOL;
  toWrite = change_bit(toWrite, 7-ENABLE_CONTROL, 1);
  toWrite = change_bit(toWrite, 7-SELECT_CONTROL, 1);
  return toWrite;
}
uint8_t setOff(uint8_t toWrite) {
  if(outputState.mode != OFF) {
    DEBUG_PRINTLN("off changed");
    outputDirty = true;
  }
  outputState.mode = OFF;
  return change_bit(toWrite, 7-ENABLE_CONTROL, 0);
}

void doControl() {
  uint8_t toWrite = 0;

  //status lights
  toWrite = change_bit(toWrite, 7-LED_FAN_ON, controlState.fan);
  toWrite = change_bit(toWrite, 7-LED_FAN_AUTO, !controlState.fan);
  toWrite = change_bit(toWrite, 7-LED_HEAT, controlState.mode == HEAT);
  toWrite = change_bit(toWrite, 7-LED_COOL, controlState.mode == COOL);
  toWrite = change_bit(toWrite, 7-LED_OFF, controlState.mode == OFF);

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
  if(currentSensorState.temp < MIN_TEMP) {
    DEBUG_PRINTLN("under min temp");
    toWrite = setHeat(toWrite);
  } else if(controlState.mode == HEAT) {
    if(currentSensorState.temp < controlState.target - controlState.swing) {
      toWrite = setHeat(toWrite);
      DEBUG_PRINTLN("heat on");
    } else  if(currentSensorState.temp >= controlState.target + controlState.swing) {
      toWrite = setOff(toWrite);
      DEBUG_PRINTLN("heat off");
    } else {
      DEBUG_PRINTLN("HEAT inside swing; continue");
      if(outputState.mode == HEAT) {
        toWrite = setHeat(toWrite);
      } else {
        toWrite = setOff(toWrite);
      }
    }
#ifndef HEAT_ONLY
  } else if(controlState.mode == COOL) {
    //DEBUG_PRINTLN("COOLING");
    //DEBUG_PRINTLN(currentSensorState.temp);
    //DEBUG_PRINTLN(controlState.target);

    //DEBUG_PRINTLN(currentSensorState.temp + controlState.swing <  controlState.target);
    //DEBUG_PRINTLN(abs(currentSensorState.temp-controlState.target) <= controlState.swing);
    //DEBUG_PRINTLN(outputState.mode != COOL);
    if(currentSensorState.temp >= controlState.target + controlState.swing) {
      toWrite = setCool(toWrite);
      DEBUG_PRINTLN("cool on");
    } else if(currentSensorState.temp < controlState.target - controlState.swing ) {
      DEBUG_PRINTLN("cool off");
      toWrite = setOff(toWrite);
    } else {
      DEBUG_PRINTLN("COOL inside swing; continue");
      if(outputState.mode == COOL) {
        toWrite = setCool(toWrite);
      } else {
        toWrite = setOff(toWrite);
      }
    }
#endif
  } else if(controlState.mode == OFF) {
    DEBUG_PRINTLN("off");
    //off
    toWrite = setOff(toWrite);
  } else {
    DEBUG_PRINTLN("holy fuck mode is broken");
    //TODO: error to mqtt
  }

  //DEBUG_PRINTLN("FAN");
  //DEBUG_PRINTLN(outputState.fan);
  //DEBUG_PRINTLN(controlState.fan);
  //DEBUG_PRINTLN(outputState.mode != OFF);
  // //(lastFan && (!force fan && !active)) || (!lastFan && (force fan || (active && fanDelay ready)))
  if((outputState.fan && (!controlState.fan && outputState.mode == OFF)) || (!outputState.fan && (controlState.fan || (outputState.mode != OFF && outputState.fanDelayEnd == 0)))) {
    DEBUG_PRINTLN("Fan changed");
    outputDirty = true;
  }
  outputState.fan = controlState.fan || (outputState.mode != OFF && outputState.fanDelayEnd == 0);
  toWrite = change_bit(toWrite, 7-FAN_CONTROL, outputState.fan);

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
  nextTimeout = millis() + controlState.timeout.timeout;

  stateDirty = true;
  controlDirty = true;

  switch(button) {
    case BTN_RESET:
      DEBUG_PRINTLN("button RESET");
      resetState();
      break;
    case BTN_FAN_ON:
      DEBUG_PRINTLN("button FAN_ON");
      controlState.fan = true;
      break;
    case BTN_FAN_AUTO:
      DEBUG_PRINTLN("button FAN_AUTO");
      controlState.fan = false;
      break;
    case TEMP_UP:
      DEBUG_PRINTLN("button TEMP_UP");
      if(++(controlState.target) > MAX_TEMP) {
        controlState.target = MAX_TEMP;
      }
      displayDirty = true;
      break;
    case TEMP_DOWN:
      DEBUG_PRINTLN("button TEMP_DOWN");
      if(--(controlState.target) < MIN_TEMP) {
        controlState.target = MIN_TEMP;
      }
      displayDirty = true;
      break;
    case BTN_HEAT:
      DEBUG_PRINTLN("button HEAT");
      EEPROM.write(1, HEAT);
      EEPROM.commit();
      controlState.mode = HEAT;
      break;
    case BTN_OFF:
      DEBUG_PRINTLN("button OFF");
      EEPROM.write(1, OFF);
      EEPROM.commit();
      controlState.mode = OFF;
      break;
#ifndef HEAT_ONLY
    case BTN_COOL:
      DEBUG_PRINTLN("button COOL");
      EEPROM.write(1, COOL);
      EEPROM.commit();
      controlState.mode = COOL;
      break;
#endif
  }
}

void display() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_inr38_mf);
  itoa(controlState.target, buf, 10);
  u8g2.drawStr(0, 116, buf);
  if(currentSensorState.tempSensor) {
    itoa(currentSensorState.temp, buf, 10);
    u8g2.drawStr(0, 50, buf);
  } else {
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.drawStr(0,30,"No BME");
    u8g2.drawStr(0,50,"temp 10");
  }
  u8g2.setFont(u8g2_font_unifont_t_symbols);
  switch(connState) {
    case F_MQTT_CONNECTED:
      u8g2.drawGlyph(55, 10, 0x25C6);	/* dec 9670/hex 25C6 some dot */
      break;
    case F_MQTT_DISCONNECTED:
      u8g2.drawGlyph(55, 10, 0x25C8);	/* dec 9672/hex 25C8 some dot */
      break;
    case WIFI_DISCONNECTED:
      u8g2.drawGlyph(55, 10, 0x25C7);	/* dec 9671/hex 25C7 some dot */
      break;
  }
  u8g2.sendBuffer();
}

void readTemp() {
  if(currentSensorState.tempSensor) {
    //TODO: sensor delta detection
    currentSensorState.temp = bme.readTemperature();
    currentSensorState.pressure = bme.readPressure();
    currentSensorState.humidity = bme.readHumidity();
  } else {
    currentSensorState.temp = 10;
  }
  //TODO: is this the correct thing? I think it makes it publish a lot
  displayDirty = true;
  stateDirty = true;
}

void reportStatus(PubSubClient *client) {
  if(controlState.mode != mqttControlState.mode) {
    mqttControlState.mode = controlState.mode;
    sprintf(topicBuf, "stat/%s/mode", TOPIC);
    client->publish(topicBuf, mqttControlState.mode == OFF ? "off" : mqttControlState.mode == HEAT ? "heat" : "cool");
  }
  if(controlState.fan != mqttControlState.fan) {
    mqttControlState.fan = controlState.fan;
    sprintf(topicBuf, "stat/%s/fan", TOPIC);
    client->publish(topicBuf, mqttControlState.fan ? "on" : "auto");
  }
  if(controlState.target != mqttControlState.target) {
    mqttControlState.target = controlState.target;
    sprintf(topicBuf, "stat/%s/target", TOPIC);
    sprintf(buf, "%d", mqttControlState.target);
    client->publish(topicBuf, buf);
  }
  if(controlState.swing != mqttControlState.swing) {
    mqttControlState.swing = controlState.swing;
    sprintf(topicBuf, "stat/%s/swing", TOPIC);
    sprintf(buf, "%d", mqttControlState.swing);
    client->publish(topicBuf, buf);
  }
  if(controlState.timeout.timeout != mqttControlState.timeout.timeout) {
    mqttControlState.timeout.timeout = controlState.timeout.timeout;
    sprintf(topicBuf, "stat/%s/timeout", TOPIC);
    sprintf(buf, "%d", mqttControlState.timeout.timeout);
    client->publish(topicBuf, buf);
  }
}

void reportTelemetry(PubSubClient *client) {
  if(currentSensorState.tempSensor) {
    sprintf(topicBuf, "tele/%s/bme280", TOPIC);
    sprintf(buf, "{\"temperature\":");
    dtostrf(currentSensorState.temp, 0, 2, buf + strlen(buf));
    sprintf(buf + strlen(buf), ", \"pressure\":");
    dtostrf(currentSensorState.pressure, 0, 1, buf + strlen(buf));
    sprintf(buf + strlen(buf), ", \"humidity\":");
    dtostrf(currentSensorState.humidity, 0, 1, buf + strlen(buf));
    sprintf(buf + strlen(buf), "}");
    client->publish(topicBuf, buf);
  } else {
    sprintf(topicBuf, "tele/%s/error", TOPIC);
    client->publish(topicBuf, "bme280 DISCONNECTED");
  }
}

void reportOutput(PubSubClient *client) {
  sprintf(topicBuf, "tele/%s/output", TOPIC);
  sprintf(buf, "{\"state\": \"%s\", \"fan\": \"%s\"}", outputState.mode == OFF ? "off" : outputState.mode == HEAT ? "heat" : "cool", outputState.fan ? "on" : "off");
  client->publish(topicBuf, buf);
}

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  if(length == 0) {
    reportStatus(client);
    return;
  }
  //Topics
  //    target temp
  //    mode: heat|cool|off
  //    fan: auto|on
  //    swing: uint8_t
  //    TODO: add set timeout
  //    TODO: add run
  if (strcmp(topic, "target") == 0) {
    payload[length] = '\0';
    sprintf(topicBuf, "stat/%s/target", TOPIC);
    controlState.target = atoi((char*)payload);
    if(controlState.target < MIN_TEMP) {
      controlState.target = MIN_TEMP;
    }
    if(controlState.target > MAX_TEMP) {
      controlState.target = MAX_TEMP;
    }
    nextTimeout = millis() + controlState.timeout.timeout;
    stateDirty = true;
    displayDirty = true;
    itoa(controlState.target, buf, 10);
    client->publish(topicBuf, buf);
  } else if (strncmp(topic, "mode", length - 1) == 0) {
    stateDirty = true;
    sprintf(topicBuf, "stat/%s/mode", TOPIC);
    if(strncmp((char*)payload, "heat", length-1) == 0) {
      controlState.mode = HEAT;
      EEPROM.write(1, HEAT);
      EEPROM.commit();
      nextTimeout = millis() + controlState.timeout.timeout;
      client->publish(topicBuf, "heat");
    } else if(strncmp((char*)payload, "cool", length-1) == 0) {
#ifndef HEAT_ONLY
      controlState.mode = COOL;
      EEPROM.write(1, COOL);
      EEPROM.commit();
      nextTimeout = millis() + controlState.timeout.timeout;
      client->publish(topicBuf, "cool");
#else
      client->publish(topicBuf, "cool not supported");
#endif
    } else if(strncmp((char*)payload, "off", length-1) == 0) {
      controlState.mode = OFF;
      EEPROM.write(1, OFF);
      EEPROM.commit();
      nextTimeout = millis() + controlState.timeout.timeout;
      client->publish(topicBuf, "off");
    } else {
      client->publish(topicBuf, "bad command");
    }
  } else if (strncmp(topic, "fan", length - 1) == 0) {
    stateDirty = true;
    sprintf(topicBuf, "stat/%s/fan", TOPIC);
    if(strncmp((char*)payload, "auto", 4) == 0) {
      controlState.fan = false;
      nextTimeout = millis() + controlState.timeout.timeout;
      client->publish(topicBuf, "auto");
    } else if(strncmp((char*)payload, "on", 2) == 0) {
      controlState.fan = true;
      nextTimeout = millis() + controlState.timeout.timeout;
      client->publish(topicBuf, "on");
    } else {
      client->publish(topicBuf, "bad command");
    }
  } else if (strncmp(topic, "swing", length - 1) == 0) {
    payload[length] = '\0';
    uint8_t newSwing = atoi((char*)payload);
    sprintf(topicBuf, "stat/%s/swing", TOPIC);
    if(newSwing <= 3 && newSwing >= 0) {
      controlState.swing = newSwing;
      EEPROM.write(2, controlState.swing);
      EEPROM.commit();
      sprintf(buf, "%d", controlState.swing);
      client->publish(topicBuf, buf);
    } else {
      client->publish(topicBuf, "new swing out of range 0-3");
    }
  } else {
    sprintf(topicBuf, "stat/%s/what", TOPIC);
    client->publish(topicBuf, "bad command");
  }
}

void connectionEvent(PubSubClient* client, enum ConnState state, int reason) {
  connState = state;
  //Skipping displayDirty because sometimes this is called with no looping
  //like for wifi connect and then mqtt connect
  display();
}

void connectSuccess(PubSubClient* client, char* ip) {
  //Are subscribed to cmnd/fullTopic/+
  // u8g2.clearBuffer();
  // u8g2.drawStr(0,10,ip);
  // u8g2.sendBuffer();
  readTemp();
  reportTelemetry(client);
  reportStatus(client);
}


void setup() {
  Serial.begin(115200);
  Serial.println("Starting");

  EEPROM.begin(8);
  if(EEPROM.read(0) != MAGIC_EEPROM_NUMBER) {

      EEPROM.write(0, MAGIC_EEPROM_NUMBER);
      EEPROM.write(1, OFF);
      EEPROM.write(2, 1);//swing
      controlState.timeout.timeout = DEFAULT_TIMEOUT_INTERVAL;
      controlState.timeout.timeout = DEFAULT_TIMEOUT_INTERVAL;
      for(int i=0; i<4; ++i) {
        EEPROM.write(3+i, controlState.timeout.octets[i]);
      }

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
  mqtt_options.connectionEvent = connectionEvent;
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
    currentSensorState.tempSensor = false;
    controlState.mode = OFF;
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
  if(controlDirty) {
    controlDirty = false;
    reportStatus(client);
  }
  //set in doControl, the outputs changed
  if(outputDirty) {
    outputDirty = false;
    reportOutput(client);
  }
  if( (long)( millis() - nextStatus ) >= 0) {
    nextStatus = millis() + statusInterval;
    reportTelemetry(client);
    reportOutput(client);
  }
}


void loop() {
  loop_mqtt();
  if( (long)( millis() - nextRead ) >= 0) {
    nextRead = millis() + readInterval;
    readTemp();
  }
  if( controlState.timeout.timeout != 0 && (long)( millis() - nextTimeout ) >= 0) {
    nextTimeout = millis() + controlState.timeout.timeout;
    resetState();
  }
  if( outputState.fanDelayEnd != 0 && (long)( millis() - outputState.fanDelayEnd ) >= 0) {
    outputState.fanDelayEnd = 0;
    stateDirty = true;
    //TODO: turn fan on
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
