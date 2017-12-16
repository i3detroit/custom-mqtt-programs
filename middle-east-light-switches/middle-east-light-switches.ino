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

#ifndef NAME
#define NAME "NEW-middle-east-light-switches"
#endif

#ifndef TOPIC
#define TOPIC "i3/program-me/NEW-middle-east-light-switches"
#endif

// button pins
const int button_pins[] = { 4, 5, 0, 2 };
const int numButtons = sizeof(button_pins)/sizeof(button_pins[0]);

//This is an array of function pointers. It relates to the button on the same pin in the button_pins array.
void (*button_functions[])(PubSubClient* client) = {&westOn, &westOff, &eastOn, &eastOff};

char buf[1024];
char topicBuf[1024];

//Debounce setup
int button_state[] = {1,1,1,1,1,1,1};
int button_state_last[] = {1,1,1,1,1,1,1};
int debounce[] = {0,0,0,0,0,0,0};
const int debounce_time = 50;

const char* host_name = NAME;
const char* fullTopic = TOPIC;
const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

void westOn(PubSubClient* client) { // pin 4
  client->publish("cmnd/i3/inside/lights/020/POWER", "1");
  client->publish("cmnd/i3/inside/lights/021/POWER", "1");
  client->publish("cmnd/i3/inside/lights/022/POWER", "1");
  client->publish("cmnd/i3/inside/lights/023/POWER", "1");
  client->publish("cmnd/i3/inside/lights/024/POWER", "1");
  client->publish("cmnd/i3/inside/lights/026/POWER", "1");
}
void westOff(PubSubClient* client) { // pin 5
  client->publish("cmnd/i3/inside/lights/020/POWER", "0");
  client->publish("cmnd/i3/inside/lights/021/POWER", "0");
  client->publish("cmnd/i3/inside/lights/022/POWER", "0");
  client->publish("cmnd/i3/inside/lights/023/POWER", "0");
  client->publish("cmnd/i3/inside/lights/024/POWER", "0");
  client->publish("cmnd/i3/inside/lights/026/POWER", "0");
}
void eastOn(PubSubClient* client) { // pin 0
  client->publish("cmnd/i3/inside/lights/015/POWER", "1");
  client->publish("cmnd/i3/inside/lights/016/POWER", "1");
  client->publish("cmnd/i3/inside/lights/017/POWER", "1");
  client->publish("cmnd/i3/inside/lights/018/POWER", "1");
  client->publish("cmnd/i3/inside/lights/019/POWER", "1");
}
void eastOff(PubSubClient* client) { // pin 2
  client->publish("cmnd/i3/inside/lights/015/POWER", "0");
  client->publish("cmnd/i3/inside/lights/016/POWER", "0");
  client->publish("cmnd/i3/inside/lights/017/POWER", "0");
  client->publish("cmnd/i3/inside/lights/018/POWER", "0");
  client->publish("cmnd/i3/inside/lights/019/POWER", "0");
}



void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  // Serial.print("Message arrived [");
  // Serial.print(topic);
  // Serial.print("] ");
  for (int i = 0; i < length; i++) {
    // Serial.print((char)payload[i]);
  }
  // Serial.println();
}

void connectSuccess(PubSubClient* client, char* ip) {
  //subscribe and shit here
  sprintf(topicBuf, "tele/%s/INFO2", fullTopic);
  sprintf(buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", host_name, ip);
  client->publish(topicBuf, buf);
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
