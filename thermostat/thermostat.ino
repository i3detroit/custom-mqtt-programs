#include <stdio.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <pcf8574_esp.h>
#include <mqtt-wrapper.h>
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

#define DEFAULT_TEMP 10
#define MIN_TEMP 5
#define MAX_TEMP 30

#define FAN_CONTROL 5 //turn on fan, 0 is on
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
char tempBuf[16];

struct mqtt_wrapper_options mqtt_options;

PCF857x i2cButtons(0x21, &Wire);
PCF857x i2cLEDs(0x38, &Wire);

unsigned long nextStatus = 0UL;
unsigned long statusInterval = 60000UL;

unsigned long nextRead = 0UL;
unsigned long readInterval = 1000UL;

unsigned long nextTimeout = 0UL;
unsigned long timeoutInterval = 1000*3*60*60;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R3, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 5, /* data=*/ 4);
Adafruit_BMP280 bmp;


int targetTemp;
int currentTemp;
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

char *ftoa(char *a, double f, int precision) {
  long p[] = {
    0,10,100,1000,10000,100000,1000000,10000000,100000000  };

  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long decimal = abs((long)((f - heiltal) * p[precision]));
  itoa(decimal, a, 10);
  return ret;
}

uint8_t change_bit(uint8_t val, uint8_t num, bool bitval) {
  return (val & ~(1<<num)) | (bitval << num);
}




//Also used for init
void resetState() {
  targetTemp = DEFAULT_TEMP;
  fanForced = false;
  heat = false;
  cool = false;
  displayDirty = true;
  stateDirty = true;
}
void doControl() {
  uint8_t toWrite = 0;
  // LED_FAN_ON 4
  // LED_FAN_AUTO 3

  //LED_HEAT 2
  //LED_OFF 1
  //LED_COOL 0
  if(currentFanForced != fanForced) {
    mqttDirty = true;
    currentFanForced = fanForced;
  }
  toWrite = change_bit(toWrite, 7-LED_FAN_ON, fanForced);
  toWrite = change_bit(toWrite, 7-LED_FAN_AUTO, !fanForced);
  toWrite = change_bit(toWrite, 7-LED_HEAT, heat);
  toWrite = change_bit(toWrite, 7-LED_COOL, cool);
  toWrite = change_bit(toWrite, 7-LED_OFF, !heat && !cool);
  toWrite = change_bit(toWrite, 7-FAN_CONTROL, fanForced);

  if(heat && currentTemp < targetTemp) {
    if(!enabled || heatCool) {
      mqttDirty = true;
    }
    enabled = true;
    heatCool = 0;
    toWrite = change_bit(toWrite, 7-ENABLE_CONTROL, 1);
    toWrite = change_bit(toWrite, 7-SELECT_CONTROL, 1);//0 is heat but this is inverted
  } else if(cool && currentTemp > targetTemp) {
    if(!enabled || !heatCool) {
      mqttDirty = true;
    }
    enabled = true;
    heatCool = 1;
    toWrite = change_bit(toWrite, 7-ENABLE_CONTROL, 1);
    toWrite = change_bit(toWrite, 7-SELECT_CONTROL, 0);//0 is heat but this is inverted
  } else {
    if(enabled) {
      mqttDirty = true;
    }
    enabled = false;
    toWrite = change_bit(toWrite, 7-ENABLE_CONTROL, 0);
  }

  toWrite = ~toWrite; //I used 1 for on 0 for off but that's wrong.
  // Serial.println(fanForced);
  // Serial.println(heat);
  // Serial.println(cool);
  // Serial.println(toWrite, BIN);
  // Serial.println("\n");
  i2cLEDs.write8(toWrite);
}
void handleButton(int button) {
  nextTimeout = millis() + timeoutInterval;
  stateDirty = true;
  switch(button) {
    case RESET:
      resetState();
      break;
    case FAN_ON:
      fanForced = true;
      break;
    case FAN_AUTO:
      fanForced = false;
      break;
    case TEMP_UP:
      if(++targetTemp > MAX_TEMP) {
        targetTemp = MAX_TEMP;
      }
      displayDirty = true;
      break;
    case TEMP_DOWN:
      if(--targetTemp < MIN_TEMP) {
        targetTemp = MIN_TEMP;
      }
      displayDirty = true;
      break;
    case HEAT:
      heat = true;
      cool = false;
      break;
    case OFF:
      heat = false;
      cool = false;
      break;
    case COOL:
      heat = false;
      cool = true;
      break;
  }
}

void display() {
  itoa(currentTemp, buf, 10);
  u8g2.clearBuffer();
  u8g2.drawStr(0, 50, buf);
  itoa(targetTemp, buf, 10);
  u8g2.drawStr(0, 116, buf);
  u8g2.sendBuffer();
}

void readTemp() {
  currentTemp = bmp.readTemperature();
  ftoa(tempBuf, currentTemp, 2);
  ftoa(pressureBuf, bmp.readPressure(), 2);
  displayDirty = true;
  stateDirty = true;
}
void reportState(PubSubClient *client) {
  sprintf(topicBuf, "tele/%s/bmp280", TOPIC);
  sprintf(buf, "{\"Temperature\":%s, \"Pressure\":%s}", tempBuf, pressureBuf);
  client->publish(topicBuf, buf);

  sprintf(topicBuf, "tele/%s/request", TOPIC);
  sprintf(buf, "{\"TargetTemp\":");
  itoa(targetTemp, buf + strlen(buf), 10);
  sprintf(buf + strlen(buf), "\", \"requested\":\"%s\", \"fan\":\"%s\"}", "targetTEmpTODO", (!heat && !cool) ? "OFF" : heat ? "HEAT" : "COOL", fanForced ? "ON" : "AUTO");
  client->publish(topicBuf, buf);

  sprintf(topicBuf, "tele/%s/output", TOPIC);
  sprintf(buf, "{\"output\":\"%s\", \"fan\":\"%s\"}", !enabled ? "OFF" : heatCool ? "COOL" : "HEAT", fanForced ? "ON" : "AUTO");
  client->publish(topicBuf, buf);
}

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  //Topics
  //    target temp
  //    mode: heat|cool|off
  //    fan: auto|on
  if (strcmp(topic, "target") == 0) {
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
  } else if (strcmp(topic, "mode") == 0) {
    stateDirty = true;
    if(strncmp((char*)payload, "heat", 4) == 0) {
      heat = true;
      cool = false;
      nextTimeout = millis() + timeoutInterval;
    } else if(strncmp((char*)payload, "cool", 4) == 0) {
      heat = false;
      cool = true;
      nextTimeout = millis() + timeoutInterval;
    } else if(strncmp((char*)payload, "off", 3) == 0) {
      heat = false;
      cool = false;
      nextTimeout = millis() + timeoutInterval;
    } else {
      sprintf(topicBuf, "stat/%s/mode", TOPIC);
      client->publish(topicBuf, "bad command");
    }
  } else if (strcmp(topic, "fan") == 0) {
    stateDirty = true;
    if(strncmp((char*)payload, "auto", 4) == 0) {
      fanForced = false;
      nextTimeout = millis() + timeoutInterval;
    } else if(strncmp((char*)payload, "on", 2) == 0) {
      fanForced = true;
      nextTimeout = millis() + timeoutInterval;
    } else {
      sprintf(topicBuf, "stat/%s/fan", TOPIC);
      client->publish(topicBuf, "bad command");
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
}


void setup() {
  Serial.begin(115200);

  resetState();

  // --- Display ---
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_inr38_mn);
  u8g2.drawStr(0,10,"Hello World!");
  u8g2.sendBuffer();

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
  if (!bmp.begin(0x76)) {
    Serial.println("Could not find BMP 280 sensor");
    while (1) {}
  }

  i2cButtons.begin();
  i2cLEDs.begin();
  i2cLEDs.write8(0b00000101);
  delay(100);
  i2cLEDs.write8(0xFF);
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

  button_state = i2cButtons.read8();
  for(int i=0; i < 8; ++i) {
    if((button_state >> (7-i) & 0x01) != button_state_last[i] && millis() - debounce[i] > debounce_time) {
      if((button_state >> (7-i) & 0x01) == LOW) {
        handleButton(i);
      }
      //If the button was pressed or released, we still need to reset the debounce timer.
      button_state_last[i] =  button_state >> (7-i) & 0x01;
      debounce[i] = millis();
    }
  }
}
