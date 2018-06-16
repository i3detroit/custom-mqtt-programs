/*
 * exit indicator
 */
#include "mqtt-wrapper.h"
#include <pcf8574_esp.h>
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

//pcf8574 i2c breakout pins
#define GLASS_DOOR_LOCK_RED 1
#define GLASS_DOOR_LOCK_GREEN 0

#define GARAGE_RED 3
#define GARAGE_GREEN 2

#define ARGON_RED 5
#define ARGON_GREEN 4

#define AIR_COMPRESSOR_RED 7
#define AIR_COMPRESSOR_GREEN 6


//actual pins
#define SHUTDOWN_BUTTON 12
#define GARAGE_DOOR_SWITCH 13
#define NORMAL_DOORBELL 15
#define NORMAL_DOORBELL_OUT 14
#define EMERGENCY_LIGHTS_ON_BUTTON 3
#define FRONT_DOOR_SWITCH 1

//Battery
//818 is 13.55
#define BATTERY_SCALAR 13.55/818
#define BATTERY_PIN A0

unsigned long ledRefresh = 0UL;
unsigned long ledRefreshInterval = 60000UL;

unsigned long status = 0UL;
unsigned long statusInterval = 60000UL;

char topicBuf[1024];
char buf[1024];

TwoWire testWire;
// Initialize a PCF8574 at I2C-address 0x20, using GPIO5, GPIO4 and testWire for the I2C-bus
PCF857x pcf8574(0b00111000, &testWire);

const char* host_name = "front-door-indicator";
const char* fullTopic = "i3/inside/commons/front-door-indicator";
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;

struct mqtt_wrapper_options mqtt_options;

// button pins 4 is sthudown button, 5 is garage door
const int button_pins[] = {NORMAL_DOORBELL, SHUTDOWN_BUTTON, EMERGENCY_LIGHTS_ON_BUTTON, GARAGE_DOOR_SWITCH, FRONT_DOOR_SWITCH};

//Debounce setup
int button_state[] = {1,1,1,1,1};
int button_state_last[] = {-1,-1,-1,-1,-1};
int debounce[] = {0,0,0,0,0};
const int debounce_time = 50;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  if (strcmp(topic, "glass") == 0) {
    //is LOCKED or UNLOCKED so comparing first char is sufficient
    pcf8574.write(GLASS_DOOR_LOCK_GREEN, (char)payload[0] == 'L' ? 1 : 0);
    pcf8574.write(GLASS_DOOR_LOCK_RED, (char)payload[0] == 'L' ? 0 : 1);
  } else if (strcmp(topic, "air") == 0) {
    //OFF or ON
    pcf8574.write(AIR_COMPRESSOR_GREEN, (char)payload[1] == 'F' ? 1 : 0);
    pcf8574.write(AIR_COMPRESSOR_RED,   (char)payload[1] == 'F' ? 0 : 1);
  } else if (strcmp(topic, "argon") == 0) {
    //CLOSED or OPEN
    pcf8574.write(ARGON_GREEN, (char)payload[0] == 'C' ? 1 : 0);
    pcf8574.write(ARGON_RED, (char)payload[0] == 'C' ? 0 : 1);
  } else if (strcmp(topic, "cmnd/i3/inside/commons/garage-door/lock") == 0) {
    client->publish("stat/i3/inside/commons/garage-door/lock", button_state[1] ? "UNLOCKED" : "LOCKED");
  } else if (strcmp(topic, "cmnd/i3/inside/commons/front-door/lock") == 0) {
    client->publish("stat/i3/inside/commons/front-door/lock", button_state[1] ? "UNLOCKED" : "LOCKED");
  } else if (strcmp(topic, "cmnd/i3/inside/commons/normal-doorbell/press") == 0) {
    client->publish("stat/i3/inside/commons/normal-doorbell/press", "command");
    digitalWrite(NORMAL_DOORBELL_OUT, 1);
    delay(750);
    digitalWrite(NORMAL_DOORBELL_OUT, 0);
  }
}

void connectSuccess(PubSubClient* client, char* ip) {
  //client->subscribe("cmnd/i3/inside/commons/exit-indicator/shutdown-LED");
  client->subscribe("cmnd/i3/inside/commons/normal-doorbell/press");
  client->subscribe("cmnd/i3/inside/commons/+/lock");
  client->publish("cmnd/i3/inside/commons/front-door-funnel/query", "");
}


void setup() {
  //input pins
  for (int i=0; i < ARRAY_SIZE(button_pins); ++i) {
    if(i==0 || i==4) {
      //doorbell in
      pinMode(button_pins[i], INPUT);
    } else {
      pinMode(button_pins[i], INPUT_PULLUP);
    }
  }
  pinMode(NORMAL_DOORBELL_OUT, OUTPUT);
  digitalWrite(NORMAL_DOORBELL_OUT, 0);

  mqtt_options.connectedLoop = connectedLoop;
  mqtt_options.callback = callback;
  mqtt_options.connectSuccess = connectSuccess;
  mqtt_options.ssid = ssid;
  mqtt_options.password = password;
  mqtt_options.mqtt_server = mqtt_server;
  mqtt_options.mqtt_port = mqtt_port;
  mqtt_options.host_name = host_name;
  mqtt_options.fullTopic = fullTopic;
  mqtt_options.debug_print = false;
  setup_mqtt(&mqtt_options);

  testWire.begin(5, 4);
  testWire.setClock(10000L);
  pcf8574.begin();
}

void connectedLoop(PubSubClient* client) {
  if( (long)( millis() - status ) >= 0) {
    status = millis() + statusInterval;
    float level = ((float)analogRead(BATTERY_PIN))*BATTERY_SCALAR;
    sprintf(topicBuf, "tele/%s/batteryVoltage", fullTopic);
    dtostrf(level, 6, 3, buf);
    client->publish(topicBuf, buf);
  }
  for(int i=0; i < ARRAY_SIZE(button_pins); ++i) {
    button_state[i] = digitalRead(button_pins[i]);//Read current state
    //If the current state does not equal the last state, AND it's been long enough since the last change
    if (button_state[i] != button_state_last[i] && millis() - debounce[i] > debounce_time) {

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

  button_state[3] = digitalRead(button_pins[3]);//Read current state
  pcf8574.write(button_state[3] ? GARAGE_RED : GARAGE_GREEN, HIGH);
  pcf8574.write(!button_state[3] ? GARAGE_RED : GARAGE_GREEN, LOW);

  if( (long)( millis() - ledRefresh ) >= 0) {
    ledRefresh = millis() + ledRefreshInterval;
    pcf8574.read8();
  }
}
