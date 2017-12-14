/*
 * ESP8266 light button
 *
 * debouncing from https://github.com/mtfurlan/arduinoKeypad
 * mqtt from PubSubClient Example
*/

#include "mqtt-wrapper.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

const int button_pins[] = { 13, 14, 12, 5, 16, 2 };
const int numButtons = sizeof(button_pins)/sizeof(button_pins[0]);

void (*button_functions[])(PubSubClient* client) = {&eastOn, &eastOff, &centerOn, &centerOff, &westOn, &westOff};

char buf[1024];
//Debounce setup
int button_state[] = {1,1,1,1,1,1,1};
int button_state_last[] = {1,1,1,1,1,1,1};
int debounce[] = {0,0,0,0,0,0,0};
const int debounce_time = 50;

const char* host_name = "commons-light-switches";
const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

void eastOn(PubSubClient* client) {
  client->publish("cmnd/i3/inside/lights/001/POWER", "1");
  client->publish("cmnd/i3/inside/lights/002/POWER", "1");
  client->publish("cmnd/i3/inside/lights/003/POWER", "1");
  client->publish("cmnd/i3/inside/lights/004/POWER", "1");
}
void centerOn(PubSubClient* client) {
  client->publish("cmnd/i3/inside/lights/005/POWER", "1");
  client->publish("cmnd/i3/inside/lights/006/POWER", "1");
  client->publish("cmnd/i3/inside/lights/007/POWER", "1");
  client->publish("cmnd/i3/inside/lights/008/POWER", "1");
  client->publish("cmnd/i3/inside/lights/009/POWER", "1");
}
void westOn(PubSubClient* client) {
  client->publish("cmnd/i3/inside/lights/011/POWER", "1");
  client->publish("cmnd/i3/inside/lights/012/POWER", "1");
  client->publish("cmnd/i3/inside/lights/013/POWER", "1");
  client->publish("cmnd/i3/inside/lights/014/POWER", "1");
}
void eastOff(PubSubClient* client) {
  client->publish("cmnd/i3/inside/lights/001/POWER", "0");
  client->publish("cmnd/i3/inside/lights/002/POWER", "0");
  client->publish("cmnd/i3/inside/lights/003/POWER", "0");
  client->publish("cmnd/i3/inside/lights/004/POWER", "0");
}
void centerOff(PubSubClient* client) {
  client->publish("cmnd/i3/inside/lights/005/POWER", "0");
  client->publish("cmnd/i3/inside/lights/006/POWER", "0");
  client->publish("cmnd/i3/inside/lights/007/POWER", "0");
  client->publish("cmnd/i3/inside/lights/008/POWER", "0");
  client->publish("cmnd/i3/inside/lights/009/POWER", "0");
}
void westOff(PubSubClient* client) {
  client->publish("cmnd/i3/inside/lights/011/POWER", "0");
  client->publish("cmnd/i3/inside/lights/012/POWER", "0");
  client->publish("cmnd/i3/inside/lights/013/POWER", "0");
  client->publish("cmnd/i3/inside/lights/014/POWER", "0");
}


void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void connectSuccess(PubSubClient* client, char* ip) {
  //subscribe and shit here
  sprintf(buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", host_name, ip);
  client->publish("tele/i3/inside/commons/commons-light-switches/INFO2", buf);
}


void setup() {
  setup_mqtt(connectedLoop, callback, connectSuccess, ssid, password, mqtt_server, mqtt_port, host_name, false);

  //input pins
  for (int i=0; i < ARRAY_SIZE(button_pins); ++i) {
    pinMode(button_pins[i], INPUT_PULLUP);
  }

}

void connectedLoop(PubSubClient* client) {
  for(int i=0; i < ARRAY_SIZE(button_pins); ++i) {
    button_state[i] = digitalRead(button_pins[i]);//Read current state
    //If the current state does not equal the last state, AND it's been long enough since the last change
    if (button_state[i] != button_state_last[i] && millis() - debounce[i] > debounce_time) {
      if (button_state[i] == LOW) {
        button_functions[i](client);
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
