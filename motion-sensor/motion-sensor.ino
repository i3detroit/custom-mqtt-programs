/*
 * Fablab vent control button
 */
#include "mqtt-wrapper.h"
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifndef NAME
#define NAME "NEW-motion-sensor"
#endif

#ifndef TOPIC
#define TOPIC "i3/program-me/NEW-motion-sensor"
#endif

char buf[1024];
char topicBuf[1024];

bool control_state;


const char* host_name = NAME;
const char* topic = TOPIC;
const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

// button pins
const int button_pins[] = {1};
const int numButtons = sizeof(button_pins)/sizeof(button_pins[0]);

//Debounce setup
int button_state[] = {1};
int button_state_last[] = {1};
int debounce[] = {0};
const int debounce_time = 50;

int durationBetweenPublishes = 1000*60;
int motionCooldown = 1000*60;
int lastPublish = -durationBetweenPublishes;
int lastMotion = millis();

unsigned long offCheck = 0UL;
unsigned long offCheckInterval = 60000UL;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {

}

void connectSuccess(PubSubClient* client, char* ip) {
  //subscribe and shit here
  //TODO:
  sprintf(buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", host_name, ip);
  sprintf(topicBuf, "stat/%s/INFO2", topic);
  client->publish(topicBuf, buf);
}

void setup() {
  setup_mqtt(connectedLoop, callback, connectSuccess, ssid, password, mqtt_server, mqtt_port, host_name, true);

  //input pins
  for (int i=0; i < numButtons; ++i) {
    pinMode(button_pins[i], INPUT_PULLUP);
  }
}

void connectedLoop(PubSubClient* client) {
  if( (long)( millis() - offCheck ) >= 0) {
    offCheck = millis() + offCheckInterval;
    //Turn off if needed
    if (millis() - lastMotion > motionCooldown) {
      sprintf(topicBuf, "stat/%s/motion", topic);
      client->publish(topicBuf, "OFF");
    }
  }

  for(int i=0; i < numButtons; ++i) {
    button_state[i] = digitalRead(button_pins[i]);
    if (button_state[i] != button_state_last[i] && millis() - debounce[i] > debounce_time) {
      //If motion
      if (button_state[i] == LOW) {
        lastMotion = millis();
        if(millis() - lastPublish > durationBetweenPublishes) {
          sprintf(topicBuf, "stat/%s/motion", topic);
          client->publish(topicBuf, "ON");
          lastPublish = millis();
        }
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

