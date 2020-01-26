#include <stdio.h>
#include <math.h>
//#include <errno.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include "Adafruit_MCP23017.h"
#include <mqtt-wrapper.h>

#include "thermostat.h"
#include "doControl.h"
#include "i3Logo.h"

uint32_t nextStatus = 0UL;
uint32_t statusInterval = 60000UL;

uint32_t nextRead = 0UL;
uint32_t readInterval = 1000UL;

uint32_t nextTimeout = 0UL;

struct ControlState controlState;
struct ControlState mqttControlState;

struct SensorState sensorState;

struct OutputState outputState;

uint8_t button_state;
int button_state_last[] = {1,1,1,1,1,1,1,1,-1};
int debounce[] = {0,0,0,0,0,0,0,0,0};
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
bool mqttDirty;


bool isValidEEPROM() {
  return EEPROM.read(EEPROM_MAGIC) == MAGIC_EEPROM_NUMBER && EEPROM.read(EEPROM_SWING) != 0;
}

void saveTimeoutToEEPROM() {
  DEBUG_PRINTLN("SAVING TIMEOUT");
  for(int i=0; i<4; ++i) {
    EEPROM.write(EEPROM_TIMEOUT+i, controlState.timeout.octets[i]);
  }
}

//Also used for init
void resetState() {
  Serial.println("reset");
  if(!isValidEEPROM()) {
    resetEEPROM();
    if(!isValidEEPROM()) {
      //panic
      Serial.println("EEPROM broken?");
      while(true) ;
    }
  }

  byte mode = EEPROM.read(EEPROM_MODE);

  controlState.swing = EEPROM.read(EEPROM_SWING);
  for(int i=0; i<4; ++i) {
    controlState.timeout.octets[i] = EEPROM.read(EEPROM_TIMEOUT+i);
  }

  switch((int)mode) {
#ifndef HEAT_ONLY
    case COOL:
      Serial.println("COOL from eeprom");
      controlState.target = DEFAULT_COOL_TEMP;
      controlState.mode = COOL;
      break;
#endif
    case HEAT:
      Serial.println("HEAT from eeprom");
      controlState.target = DEFAULT_HEAT_TEMP;
      controlState.mode = HEAT;
      break;
    case OFF:
    default:
      Serial.println("OFF from eeprom");
      controlState.target = DEFAULT_HEAT_TEMP;
      controlState.mode = OFF;
      break;
  }
  if(!sensorState.tempSensor) {
    Serial.println("OFF cause no temp sensor");
    controlState.mode = OFF;
  }
  controlState.fan = false;
  displayDirty = true;
  stateDirty = true;
}


void handleButton(int button) {
  nextTimeout = millis() + controlState.timeout.timeout;

  stateDirty = true;
  controlDirty = true;
  displayDirty = true;

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
      EEPROM.write(EEPROM_MODE, HEAT);
      EEPROM.commit();
      controlState.mode = HEAT;
      break;
    case BTN_OFF:
      DEBUG_PRINTLN("button OFF");
      EEPROM.write(EEPROM_MODE, OFF);
      EEPROM.commit();
      controlState.mode = OFF;
      break;
#ifndef HEAT_ONLY
    case BTN_COOL:
      DEBUG_PRINTLN("button COOL");
      EEPROM.write(EEPROM_MODE, COOL);
      EEPROM.commit();
      controlState.mode = COOL;
      break;
#endif
  }
}

void display() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_inr38_mf);

  // display target temp
  if (controlState.mode != OFF) {
    itoa(controlState.target, buf, 10);
    u8g2.drawStr(0, 116, buf);
  }

  //display current temp
  if (sensorState.tempSensor) {
    itoa(sensorState.temp, buf, 10);
    u8g2.drawStr(0, 50, buf);
  } else {
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.drawStr(0,30,"No BME");
    u8g2.drawStr(0,50,"temp 10");
  }
  if(sensorState.batteryPower) {
    // https://github.com/olikraus/u8g2/wiki/fntgrpu8g#battery19
    u8g2.setFont(u8g2_font_battery19_tn);
    u8g2.setFontDirection(3); //down to up
    u8g2.drawGlyph(20, 10, 0x0033);
    u8g2.setFontDirection(0); //left to right
  }
  u8g2.setFont(u8g2_font_unifont_t_symbols);
  // https://github.com/olikraus/u8g2/wiki/fntgrpunifont
  // bottom of page
  switch(connState) {
    case F_MQTT_CONNECTED:
      u8g2.drawGlyph(55, 10, 0x25C6);	/* dec 9670/hex 25C6 solid diamond */
      break;
    case F_MQTT_DISCONNECTED:
      u8g2.drawGlyph(55, 10, 0x25C8);	/* dec 9672/hex 25C8 dot inside diamond */
      break;
    case WIFI_DISCONNECTED:
      u8g2.drawGlyph(55, 10, 0x25C7);	/* dec 9671/hex 25C7 empty diamond */
      break;
  }
  u8g2.sendBuffer();
}

void readTemp() {
  if(sensorState.tempSensor) {
    //TODO: sensor delta detection
    sensorState.temp = bme.readTemperature();
    sensorState.pressure = bme.readPressure();
    sensorState.humidity = bme.readHumidity();
  } else {
    sensorState.temp = 10;
  }
  //TODO: is this the correct thing? I think it makes it publish a lot
  displayDirty = true;
  stateDirty = true;
}

void reportControlState(PubSubClient *client, bool force) {
  bool change = false;
  if(controlState.mode != mqttControlState.mode) {
    change = true;
    mqttControlState.mode = controlState.mode;
    sprintf(topicBuf, "stat/%s/mode", TOPIC);
    client->publish(topicBuf, mqttControlState.mode == OFF ? "off" : mqttControlState.mode == HEAT ? "heat" : "cool");
  }
  if(controlState.fan != mqttControlState.fan) {
    change = true;
    mqttControlState.fan = controlState.fan;
    sprintf(topicBuf, "stat/%s/fan", TOPIC);
    client->publish(topicBuf, mqttControlState.fan ? "on" : "auto");
  }
  if(controlState.target != mqttControlState.target) {
    change = true;
    mqttControlState.target = controlState.target;
    sprintf(topicBuf, "stat/%s/target", TOPIC);
    sprintf(buf, "%d", mqttControlState.target);
    client->publish(topicBuf, buf);
  }
  if(controlState.swing != mqttControlState.swing) {
    change = true;
    mqttControlState.swing = controlState.swing;
    sprintf(topicBuf, "stat/%s/swing", TOPIC);
    sprintf(buf, "%d", mqttControlState.swing);
    client->publish(topicBuf, buf);
  }
  if(controlState.timeout.timeout != mqttControlState.timeout.timeout) {
    change = true;
    mqttControlState.timeout.timeout = controlState.timeout.timeout;
    sprintf(topicBuf, "stat/%s/timeout", TOPIC);
    sprintf(buf, "%d", mqttControlState.timeout.timeout);
    client->publish(topicBuf, buf);
  }
  if(change || force) {
    sprintf(topicBuf, "tele/%s/input", TOPIC);
    sprintf(buf, "{\"target\":%d, \"mode\":\"%s\", \"fan\":\"%s\", \"swing\":%d, \"timeout\":%d}", controlState.target, controlState.mode == OFF ? "off" : controlState.mode == HEAT ? "heat" : "cool", controlState.fan ? "on" : "auto", controlState.swing, controlState.timeout.timeout);
    client->publish(topicBuf, buf);
  }
}


void reportTelemetry(PubSubClient *client) {
  sprintf(topicBuf, "tele/%s/sensors", TOPIC);
  // Removed powerSource printing, publishing fails with it enabled as is
  // sprintf(buf, "{\"powerSource\": \"%s\"", sensorState.batteryPower ? "battery" : "mains");
  sprintf(buf, "{");
  if(sensorState.tempSensor) {
    // sprintf(buf + strlen(buf), ", \"temperature\":");
    sprintf(buf + strlen(buf), "\"temperature\":");
    dtostrf(sensorState.temp, 0, 2, buf + strlen(buf));
    sprintf(buf + strlen(buf), ", \"pressure\":");
    dtostrf(sensorState.pressure, 0, 1, buf + strlen(buf));
    sprintf(buf + strlen(buf), ", \"humidity\":");
    dtostrf(sensorState.humidity, 0, 1, buf + strlen(buf));
  } else {
    sprintf(buf + strlen(buf), ", \"temperature\": \"no sensor\", \"pressure\": \"no sensor\", \"humidity\": \"no sensor\"");
  }
  sprintf(buf + strlen(buf), "}");
  client->publish(topicBuf, buf);
}

void reportOutput(PubSubClient *client) {
  sprintf(topicBuf, "tele/%s/output", TOPIC);
  sprintf(buf, "{\"state\": \"%s\", \"fan\": \"%s\"}", outputState.mode == OFF ? "off" : outputState.mode == HEAT ? "heating" : "cooling", outputState.fan ? "on" : "off");
  client->publish(topicBuf, buf);
}

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  //Topics
  //    target temp
  //    mode: heat|cool|off
  //    fan: auto|on
  //    swing: uint8_t
  //    add set timeout
  //    add run
  //    query

  /**
   * Ideas for how this function operates
   * Return on stats/bad command
   * At end of function, report new state to MQTT
   * At end of function, increase timeout
   **/
  //This helps if we try to parse it as a number. Or strcmp
  payload[length] = '\0';
  if (strcmp(topic, "target") == 0) {
    sprintf(topicBuf, "stat/%s/target", TOPIC);
    if(length == 0) {
      itoa(controlState.target, buf, 10);
      client->publish(topicBuf, buf);
      return;
    }
    char* endptr;
    int newTarget = (int)round(strtof((char*)payload, &endptr));
    if(*endptr != '\0' || endptr == (char*)payload) {
      //didn't parse full string
      sprintf(topicBuf, "stat/%s/error", TOPIC);
      sprintf(buf, "target: non-parsable value '%.*s'", length, (char*)payload);
      client->publish(topicBuf, buf);
      return;
    }
    controlState.target = newTarget;
    if(controlState.target < MIN_TEMP) {
      controlState.target = MIN_TEMP;
    }
    if(controlState.target > MAX_TEMP) {
      controlState.target = MAX_TEMP;
    }
    stateDirty = true;
    displayDirty = true;
  } else if (strcmp(topic, "mode") == 0) {
    stateDirty = true;
    sprintf(topicBuf, "stat/%s/mode", TOPIC);
    if(length == 0) {
      switch(controlState.mode) {
        case HEAT:
          client->publish(topicBuf, "HEAT");
          break;
        case COOL:
          client->publish(topicBuf, "COOL");
          break;
        case OFF:
          client->publish(topicBuf, "OFF");
          break;
      }
      return;
    }
    if(strcmp((char*)payload, "heat") == 0) {
      controlState.mode = HEAT;
      EEPROM.write(EEPROM_MODE, HEAT);
      EEPROM.commit();
    } else if(strcmp((char*)payload, "cool") == 0) {
#ifndef HEAT_ONLY
      controlState.mode = COOL;
      EEPROM.write(EEPROM_MODE, COOL);
      EEPROM.commit();
#else
      client->publish(topicBuf, "cool not supported");
#endif
    } else if(strcmp((char*)payload, "off") == 0) {
      controlState.mode = OFF;
      EEPROM.write(EEPROM_MODE, OFF);
      EEPROM.commit();
    } else {
      sprintf(topicBuf, "stat/%s/error", TOPIC);
      client->publish(topicBuf, "bad mode command");
      return;
    }
  } else if (strcmp(topic, "fan") == 0) {
    stateDirty = true;
    sprintf(topicBuf, "stat/%s/fan", TOPIC);
    if(length == 0) {
      if(controlState.fan) {
        client->publish(topicBuf, "on");
      } else {
        client->publish(topicBuf, "auto");
      }
      return;
    }
    if(strcmp((char*)payload, "auto") == 0) {
      controlState.fan = false;
    } else if(strncmp((char*)payload, "on", 2) == 0) {
      controlState.fan = true;
    } else {
      sprintf(topicBuf, "stat/%s/error", TOPIC);
      client->publish(topicBuf, "bad fan command");
      return;
    }
  } else if (strcmp(topic, "swing") == 0) {
    if(length == 0) {
      sprintf(buf, "%d", controlState.swing);
      sprintf(topicBuf, "stat/%s/swing", TOPIC);
      client->publish(topicBuf, buf);
      return;
    }
    char* endptr;
    int newSwing = strtol((char*)payload, &endptr, 10);
    if(*endptr != '\0' || endptr == (char*)payload) {
      //didn't parse full string
      sprintf(topicBuf, "stat/%s/error", TOPIC);
      sprintf(buf, "swing: non-parsable value '%.*s'", length, (char*)payload);
      client->publish(topicBuf, buf);
      return;
    }
    if(newSwing <= 3 && newSwing >= 1) {
      controlState.swing = newSwing;
      EEPROM.write(EEPROM_SWING, controlState.swing);
      EEPROM.commit();
      sprintf(buf, "%d", controlState.swing);
    } else {
      sprintf(topicBuf, "stat/%s/error", TOPIC);
      client->publish(topicBuf, "new swing out of range 1-3");
    }
  } else if (strcmp(topic, "timeout") == 0) {
    if(length == 0) {
      sprintf(topicBuf, "stat/%s/timeout", TOPIC);
      sprintf(buf, "%ld", controlState.timeout.timeout);
      client->publish(topicBuf, buf);
      return;
    }
    char* endptr;
    Serial.println("called timeout");
    uint32_t timeout = strtoul((char*)payload, &endptr, 10);
    if(*endptr != '\0' || endptr == (char*)payload) {
      //didn't parse full string
      sprintf(topicBuf, "stat/%s/error", TOPIC);
      sprintf(buf, "timeout: non-parsable value '%.*s'", length, (char*)payload);
      client->publish(topicBuf, buf);
      return;
    }

    if(timeout == 0 || (MIN_TIMEOUT < timeout && timeout < MAX_TIMEOUT)) {
      //Serial.println("Good timeout value");
      //Serial.println(timeout);
      //Valid number from payload
      controlState.timeout.timeout = timeout;
      saveTimeoutToEEPROM();
    } else {
      //We didn't read anything// or error
      sprintf(topicBuf, "stat/%s/error", TOPIC);
      sprintf(buf, "timeout: bad value '%.*s'", length, (char*)payload);
      client->publish(topicBuf, buf);
      return;
    }
  } else if (strcmp(topic, "run") == 0) {
    resetState();
    sprintf(topicBuf, "stat/%s/runReason", TOPIC);
    client->publish(topicBuf, "mqtt");
  } else if (strcmp(topic, "query") == 0) {
    reportTelemetry(client);
    reportOutput(client);
    reportControlState(client, true);
  } else {
    sprintf(topicBuf, "stat/%s/what", TOPIC);
    client->publish(topicBuf, "bad command");
    return;
  }
  //If we got here, it wasn't a failure
  nextTimeout = millis() + controlState.timeout.timeout;
  reportControlState(client, false);
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
  reportControlState(client, false);
}


void printEEPROM() {
  Serial.print("magic byte: 0x");
  Serial.println(EEPROM.read(EEPROM_MAGIC), HEX);
  Serial.print("mode: ");
  Serial.println(EEPROM.read(EEPROM_MODE));
  Serial.print("swing: ");
  Serial.println(EEPROM.read(EEPROM_SWING));

  union TimeoutInterval timeout;
  for(int i=0; i<4; ++i) {
    timeout.octets[i] = EEPROM.read(EEPROM_TIMEOUT+i);
  }
  Serial.print("timeout: ");
  Serial.println(timeout.timeout);
}

void resetEEPROM() {
  Serial.println("eeprom wrong; setting defaults");
  for(int i=0; i<EEPROM_SIZE; ++i) {
    EEPROM.write(i, 0);
  }

  EEPROM.write(EEPROM_MAGIC, MAGIC_EEPROM_NUMBER);
  EEPROM.write(EEPROM_MODE, OFF);
  EEPROM.write(EEPROM_SWING, 1);
  controlState.timeout.timeout = DEFAULT_TIMEOUT_INTERVAL;
  saveTimeoutToEEPROM();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting");

  pinMode(MAINS_TEST_PIN, INPUT_PULLUP);
  sensorState.batteryPower = 0;

  EEPROM.begin(EEPROM_SIZE);
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
    sensorState.tempSensor = false;
    controlState.mode = OFF;
  } else {
    sensorState.tempSensor = true;
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
    reportControlState(client, false);
  }
  //set in doControl, the outputs changed
  if(outputState.mqttOutputDirty) {
    outputState.mqttOutputDirty = false;
    reportOutput(client);
  }
  if(mqttDirty || (long)( millis() - nextStatus ) >= 0) {
    nextStatus = millis() + statusInterval;
    mqttDirty = false;
    reportTelemetry(client);
    reportOutput(client);
    reportControlState(client, true);
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

  //mqtt loop called before this
  if(displayDirty) {
    displayDirty = false;
    display();
  }

  if(stateDirty) {
    uint8_t toWrite = doControl(&controlState, &sensorState, &outputState);
    writeLow(toWrite);
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

  button_state = digitalRead(MAINS_TEST_PIN);
  if(button_state != button_state_last[8] && millis() - debounce[8] > debounce_time) {
    button_state_last[8] =  button_state;
    debounce[8] = millis();
    sensorState.batteryPower = !button_state;
    Serial.print("batteryPower ");
    Serial.println(sensorState.batteryPower);
    displayDirty = true;
    mqttDirty = true;
  }
}
