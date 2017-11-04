/*
 * exit indicator
 */
#include "mqtt-wrapper.h"
#include <pcf8574_esp.h>
#include <Wire.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

//TODO:
//button publish to cmnd/i3/openhab/shutdown
//LED listening to cmnd/i3/exitIndicator/shutdownLED RED/GREEN
//input from garage door publishing to stat/i3/commons/garageDoor LOCKED/UNLOCKED
//LED for glass door listening to cmnd/i3/classroom/glassDoor/lock
//LED for garage door directly controlled

//pcf8574 i2c breakout pins
#define GLASS_DOOR_LOCK_RED 1
#define GLASS_DOOR_LOCK_GREEN 0

#define GARAGE_RED 3
#define GARAGE_GREEN 2

#define AIR_COMPRESSOR_RED 5
#define AIR_COMPRESSOR_GREEN 4

#define SHUTDOWN_RED 7
#define SHUTDOWN_GREEN 6




//actual pins
#define SHUTDOWN_BUTTON 12
#define GARAGE_DOOR_BUTTON 13
#define NORMAL_DOORBELL 2
#define NORMAL_DOORBELL_OUT 14

char buf[1024];

TwoWire testWire;
// Initialize a PCF8574 at I2C-address 0x20, using GPIO5, GPIO4 and testWire for the I2C-bus
PCF857x pcf8574(0b00111000, &testWire);

const char* host_name = "front-door-indicator";
const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

// button pins 4 is sthudown button, 5 is garage door
const int button_pins[] = {SHUTDOWN_BUTTON, GARAGE_DOOR_BUTTON, NORMAL_DOORBELL};

//Debounce setup
int button_state[] = {1,1,1};
int button_state_last[] = {-1,-1,-1};
int debounce[] = {0,0,0};
const int debounce_time = 50;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  if (strcmp(topic, "stat/i3/classroom/glassDoor/lock") == 0) {
    //is LOCKED or UNLOCKED so comparing first char is sufficient
    pcf8574.write(GLASS_DOOR_LOCK_GREEN, (char)payload[0] == 'L' ? 1 : 0);
    pcf8574.write(GLASS_DOOR_LOCK_RED, (char)payload[0] == 'L' ? 0 : 1);
  } else if (strcmp(topic, "cmnd/i3/inside/commons/exitIndicator/shutdown-LED") == 0) {
    //RED or GREEN
    pcf8574.write(SHUTDOWN_GREEN, (char)payload[0] == 'G' ? 1 : 0);
    pcf8574.write(SHUTDOWN_RED,   (char)payload[0] == 'G' ? 0 : 1);
  } else if (strcmp(topic, "stat/i3/inside/machineShop/air-compressor/POWER") == 0) {
    //OFF or ON
    pcf8574.write(SHUTDOWN_GREEN, (char)payload[1] == 'F' ? 1 : 0);
    pcf8574.write(SHUTDOWN_RED,   (char)payload[1] == 'F' ? 0 : 1);
  } else if (strcmp(topic, "cmnd/i3/inside/commons/normal-doorbell") == 0) {
    client->publish("stat/i3/inside/commons/normal-doorbell", "command");
    digitalWrite(NORMAL_DOORBELL_OUT, 1);
    delay(750);
    digitalWrite(NORMAL_DOORBELL_OUT, 0);
  }
}

void connectSuccess(PubSubClient* client, char* ip) {
  //subscribe and shit here
  sprintf(buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", host_name, ip);
  client->publish("tele/i3/inside/commons/exitIndicator/INFO2", buf);

  client->subscribe("stat/i3/classroom/glassDoor/lock");
  client->publish("cmnd/i3/classroom/glassDoor/lock", "query");

  client->subscribe("cmnd/i3/inside/commons/exitIndicator/shutdown-LED");
  client->subscribe("cmnd/i3/inside/commons/normal-doorbell");

  client->subscribe("stat/i3/inside/machineShop/air-compressor/POWER");
  client->publish("cmnd/i3/inside/machineShop/air-compressor/POWER", "query");
}


void setup() {
  setup_mqtt(connectedLoop, callback, connectSuccess, ssid, password, mqtt_server, mqtt_port, host_name, false);

  //input pins
  for (int i=0; i < ARRAY_SIZE(button_pins); ++i) {
    pinMode(button_pins[i], INPUT_PULLUP);
  }
  pinMode(NORMAL_DOORBELL_OUT, OUTPUT);

  testWire.begin(5, 4);
  testWire.setClock(10000L);
  pcf8574.begin();
}

void connectedLoop(PubSubClient* client) {
  for(int i=0; i < ARRAY_SIZE(button_pins); ++i) {
    button_state[i] = digitalRead(button_pins[i]);//Read current state
    //If the current state does not equal the last state, AND it's been long enough since the last change
    if (button_state[i] != button_state_last[i] && millis() - debounce[i] > debounce_time) {

      if(i == 0 && button_state[i] == HIGH) {
        //shutdown button
        client->publish("cmnd/i3/automation/shutdown", "DOWNSHUT");
      } else if(i == 1) {
        //garage door
        client->publish("stat/i3/inside/commons/garageDoor", button_state[i] ? "UNLOCKED" : "LOCKED");
        pcf8574.write(button_state[i] ? GARAGE_RED : GARAGE_GREEN, HIGH);
        pcf8574.write(!button_state[i] ? GARAGE_RED : GARAGE_GREEN, LOW);
      } else if(i == 2) {
        //normal doorbell
        if (button_state[i] == LOW) {
          client->publish("stat/i3/inside/commons/normal-doorbell", "ding\a");
        }
        digitalWrite(NORMAL_DOORBELL_OUT, !button_state[i]);
      }

      //If the button was pressed or released, we still need to reset the debounce timer.
      button_state_last[i] = button_state[i];
      debounce[i] = millis();
    }
  }
}

void loop() {
  loop_mqtt();
}
