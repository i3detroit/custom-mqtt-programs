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

#ifndef NAME
#define NAME "commons-light-switches"
#endif

#ifndef TOPIC
#define TOPIC "i3/inside/commons/light-switches"
#endif

struct mqtt_wrapper_options mqtt_options;

const int button_pins[] = { 4, 14, 12, 5, 0, 2 };

void (*button_functions[])(PubSubClient* client) = {&eastOn, &eastOff, &centerOn, &centerOff, &westOn, &westOff};

char buf[1024];
//Debounce setup
int button_state[] = {1,1,1,1,1,1,1};
int button_state_last[] = {1,1,1,1,1,1,1};
int debounce[] = {0,0,0,0,0,0,0};
const int debounce_time = 50;


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
  client->publish("cmnd/i3/inside/lights/010/POWER", "1");
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
  client->publish("cmnd/i3/inside/lights/010/POWER", "0");
  client->publish("cmnd/i3/inside/lights/011/POWER", "0");
  client->publish("cmnd/i3/inside/lights/012/POWER", "0");
  client->publish("cmnd/i3/inside/lights/013/POWER", "0");
  client->publish("cmnd/i3/inside/lights/014/POWER", "0");
}


void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
}

void connectSuccess(PubSubClient* client, char* ip) {
}


void setup() {
  mqtt_options.connectedLoop = connectedLoop;
  mqtt_options.callback = callback;
  mqtt_options.connectSuccess = connectSuccess;
  mqtt_options.ssid = WIFI_SSID;
  mqtt_options.password = WIFI_PASSWORD;
  mqtt_options.mqtt_server = MQTT_SERVER;
  mqtt_options.mqtt_port = MQTT_PORT;
  mqtt_options.host_name = NAME;
  mqtt_options.fullTopic = TOPIC;
  mqtt_options.debug_print = false;
  setup_mqtt(&mqtt_options);

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
