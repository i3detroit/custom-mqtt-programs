/*
 * exit indicator
 */
#include "mqtt-wrapper.h"
#include "Adafruit_MCP23017.h"
#include <Wire.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

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


//DOES:
//  button publish to cmnd/i3/openhab/shutdown
//  LED listening to cmnd/i3/exitIndicator/shutdownLED RED/GREEN
//  input from garage door publishing to stat/i3/commons/garageDoor LOCKED/UNLOCKED
//  LED for glass door listening to cmnd/i3/classroom/glassDoor/lock
//  LED for garage door directly controlled
//  LED for argon stat/i3/inside/weld-zone/tank-sensors/argon
//  Publish analog pin battery read
//  publish front door switch state

//port A is LEDs
//  0,1 glass door green, red
//  2,3 roll-up door green red
//  45, argon
//  67, air compressor
//port B
//  8,9 is buttons
//  10: yellow door lock status, active high
//  11: purple: doorbell input, active high
//  12: green doorbell output, active high
//  13: red: garage door lock, active low

#define GLASS_DOOR_LOCK_GREEN 0
#define GLASS_DOOR_LOCK_RED 1

#define GARAGE_GREEN 2
#define GARAGE_RED 3

#define ARGON_GREEN 4
#define ARGON_RED 5

#define AIR_COMPRESSOR_GREEN 6
#define AIR_COMPRESSOR_RED 7

#define EMERGENCY_LIGHTS_ON_BUTTON 8//active low
#define SHUTDOWN_BUTTON 9//active low
#define FRONT_DOOR_SWITCH 10//acive high
#define NORMAL_DOORBELL 11//active high
#define NORMAL_DOORBELL_OUT 12
#define GARAGE_DOOR_SWITCH 13//active low

//Battery
//818 is 13.55
#define BATTERY_SCALAR 13.00/811
#define BATTERY_PIN A0

unsigned long status = 0UL;
unsigned long statusInterval = 60000UL;

char topicBuf[1024];
char buf[1024];

Adafruit_MCP23017 mcp;

const char* host_name = "front-door-indicator";
const char* fullTopic = "i3/inside/commons/front-door-indicator";
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;

struct mqtt_wrapper_options mqtt_options;

const int button_pins[] = {NORMAL_DOORBELL, SHUTDOWN_BUTTON, EMERGENCY_LIGHTS_ON_BUTTON, GARAGE_DOOR_SWITCH, FRONT_DOOR_SWITCH};

//Debounce setup
int button_state[] = {1,1,1,1,1};
int button_state_last[] = {-1,-1,-1,-1,-1};
int debounce[] = {0,0,0,0,0};
const int debounce_time = 50;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  if (strcmp(topic, "stat/i3/inside/classroom/glass-door/lock") == 0) {
    //is LOCKED or UNLOCKED so comparing first char is sufficient
    mcp.digitalWrite(GLASS_DOOR_LOCK_GREEN, (char)payload[0] == 'L' ? 1 : 0);
    mcp.digitalWrite(GLASS_DOOR_LOCK_RED, (char)payload[0] == 'L' ? 0 : 1);

  } else if (strcmp(topic, "stat/i3/inside/infrastructure/air-compressor/POWER") == 0) {
    //OFF or ON
    mcp.digitalWrite(AIR_COMPRESSOR_GREEN, (char)payload[1] == 'F' ? 1 : 0);
    mcp.digitalWrite(AIR_COMPRESSOR_RED,   (char)payload[1] == 'F' ? 0 : 1);
  } else if (strcmp(topic, "stat/i3/inside/weld-zone/tank-sensors/argon") == 0) {
    //CLOSED or OPEN
    mcp.digitalWrite(ARGON_GREEN, (char)payload[0] == 'C' ? 1 : 0);
    mcp.digitalWrite(ARGON_RED, (char)payload[0] == 'C' ? 0 : 1);
  } else if (strcmp(topic, "cmnd/i3/inside/commons/garage-door/lock") == 0) {
    client->publish("stat/i3/inside/commons/garage-door/lock", button_state[3] ? "UNLOCKED" : "LOCKED");
  } else if (strcmp(topic, "cmnd/i3/inside/commons/front-door/lock") == 0) {
    client->publish("stat/i3/inside/commons/front-door/lock", !button_state[4] ? "UNLOCKED" : "LOCKED");
  } else if (strcmp(topic, "cmnd/i3/inside/commons/normal-doorbell/press") == 0) {
    client->publish("stat/i3/inside/commons/normal-doorbell/press", "command");
    mcp.digitalWrite(NORMAL_DOORBELL_OUT, 1);
    delay(750);
    mcp.digitalWrite(NORMAL_DOORBELL_OUT, 0);
  }
}

void connectSuccess(PubSubClient* client, char* ip) {
  client->subscribe("cmnd/i3/inside/commons/normal-doorbell/press");
  client->subscribe("cmnd/i3/inside/commons/+/lock");
  client->subscribe("stat/i3/inside/#");
  client->publish("cmnd/i3/inside/infrastructure/air-compressor/POWER", "query");
  client->publish("cmnd/i3/inside/classroom/glass-door/lock", "query");
  client->publish("cmnd/i3/inside/weld-zone/tank-sensors/argon", "query");
}


void setup() {
  Serial.begin(115200);

  mqtt_options.connectedLoop = connectedLoop;
  mqtt_options.callback = callback;
  mqtt_options.connectSuccess = connectSuccess;
  mqtt_options.ssid = ssid;
  mqtt_options.password = password;
  mqtt_options.mqtt_server = mqtt_server;
  mqtt_options.mqtt_port = mqtt_port;
  mqtt_options.host_name = host_name;
  mqtt_options.fullTopic = fullTopic;
  mqtt_options.debug_print = true;
  setup_mqtt(&mqtt_options);

  mcp.begin();

  for(int i=0; i<8; ++i) {
    mcp.pinMode(i, OUTPUT);
  }

  for(int i=8; i<10; ++i) {
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, HIGH);//100K pull
  }

  for(int i=10; i<12; ++i) {
    mcp.pinMode(i, INPUT);
    //pulled low
  }

  mcp.pinMode(12, OUTPUT);

  mcp.pinMode(13, INPUT);
  mcp.pullUp(13, HIGH);//100K pull

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

void connectedLoop(PubSubClient* client) {
  if( (long)( millis() - status ) >= 0) {
    status = millis() + statusInterval;
    Serial.println("STATUS");
    float level = ((float)analogRead(BATTERY_PIN))*BATTERY_SCALAR;
    sprintf(topicBuf, "tele/%s/batteryVoltage", fullTopic);
    dtostrf(level, 6, 3, buf);
    client->publish(topicBuf, buf);
  }
  for(int i=0; i < ARRAY_SIZE(button_pins); ++i) {
    button_state[i] = mcp.digitalRead(button_pins[i]);//Read current state
    //If the current state does not equal the last state, AND it's been long enough since the last change
    if (button_state[i] != button_state_last[i] && millis() - debounce[i] > debounce_time) {

      Serial.println(i);
      Serial.println(button_state[i]);
      Serial.println();
      if(i == 0 && button_state[i] == HIGH) {
        //normal doorbell
        client->publish("stat/i3/inside/commons/normal-doorbell/press", "ding\a");
      } else if(i == 1 && button_state[i] == LOW) {
        //shutdown button
        client->publish("cmnd/i3/automation/shutdown", "DOWNSHUT");
        sprintf(buf, "stat/%s/shutdown", fullTopic);
        client->publish(buf, "It was I who pressed the button");
      } else if(i == 2 && button_state[i] == LOW) {
        //emergency lights on
        client->publish("cmnd/i3/automation/emergencyLightsOn", "ON");
      } else if(i == 3) {
        //garage door
        client->publish("stat/i3/inside/commons/garage-door/lock", button_state[i] ? "UNLOCKED" : "LOCKED");
      } else if(i == 4) {
        //front door
        client->publish("stat/i3/inside/commons/front-door/lock", !button_state[i] ? "UNLOCKED" : "LOCKED");
      }

      //If the button was pressed or released, we still need to reset the debounce timer.
      button_state_last[i] = button_state[i];
      debounce[i] = millis();
    }
  }
}

void loop() {
  loop_mqtt();

  button_state[3] = mcp.digitalRead(button_pins[3]);//Read current state
  mcp.digitalWrite(button_state[3] ? GARAGE_RED : GARAGE_GREEN, HIGH);
  mcp.digitalWrite(!button_state[3] ? GARAGE_RED : GARAGE_GREEN, LOW);
}
