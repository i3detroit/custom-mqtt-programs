/*
 * mqtt laser cutter blast gate status reporter
 * almost completely reused the glass door status reporter code
 */
#include "mqtt-wrapper.h"

#ifndef WIFI_SSID
#define WIFI_SSID "i3detroit-iot"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "securityrisk"
#endif

#ifndef MQTT_SERVER
#define MQTT_SERVER "10.13.0.22"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef NAME
#define NAME "laser-blast-gate-monitor"
#endif

#ifndef TOPIC
#define TOPIC "i3/inside/laser-zone/blast-gate-monitor"
#endif

struct mqtt_wrapper_options mqtt_options;

char buf[1024];

// switch pins 4 is bumblebee, 5 is wolverine
const int button_pins[] = {4, 5}; //D1 mini pins 2, 1
const int numButtons = sizeof(button_pins)/sizeof(button_pins[0]);

//Debounce setup
int button_state[] = {1,1};
int button_state_last[] = {-1,-1};
int debounce[] = {0,0};
const int debounce_time = 80;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  if (strcmp(topic, "query") == 0) {
    client->publish("stat/i3/inside/laser-zone/bumblebee/vent-fan-gate", button_state_last[0] ? "closed" : "open");
    client->publish("stat/i3/inside/laser-zone/wolverine/vent-fan-gate", button_state_last[1] ? "closed" : "open");
  }
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
  //Foreach pin run debouncing, and if it's a real press call the function
  for(int i=0; i < numButtons; ++i) {
    button_state[i] = digitalRead(button_pins[i]);//Read current state
    //If the current state does not equal the last state, AND it's been long enough since the last change
    if (button_state[i] != button_state_last[i] && millis() - debounce[i] > debounce_time) {
      if(i == 0) {
        client->publish("stat/i3/inside/laser-zone/bumblebee/vent-fan-gate", button_state[i] ? "closed" : "open");
      } else if(i == 1) {
        client->publish("stat/i3/inside/laser-zone/wolverine/vent-fan-gate", button_state[i] ? "closed" : "open ");
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
