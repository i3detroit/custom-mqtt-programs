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

struct mqtt_wrapper_options mqtt_options;

char buf[1024];
char topicBuf[1024];

bool control_state;


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
  for (int i=0; i < numButtons; ++i) {
    pinMode(button_pins[i], INPUT_PULLUP);
  }
}

void connectedLoop(PubSubClient* client) {
  if( (long)( millis() - offCheck ) >= 0) {
    offCheck = millis() + offCheckInterval;
    //Turn off if needed
    if (millis() - lastMotion > motionCooldown) {
      sprintf(topicBuf, "stat/%s/motion", TOPIC);
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
          sprintf(topicBuf, "stat/%s/motion", TOPIC);
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

