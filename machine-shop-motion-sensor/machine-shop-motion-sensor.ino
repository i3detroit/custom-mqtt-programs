/*
 * Fablab vent control button
 */
#include "mqtt-wrapper.h"
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))


char buf[1024];
char topicBuf[1024];

bool control_state;


const char* host_name = "machine-shop-motion-sensor";
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

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {

}

void connectSuccess(PubSubClient* client, char* ip) {
  //subscribe and shit here
  //TODO:
  sprintf(buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", host_name, ip);
  client->publish( "stat/i3/inside/machineShop/motion-sensor/INFO2", buf);
}

void setup() {
  setup_mqtt(connectedLoop, callback, connectSuccess, ssid, password, mqtt_server, mqtt_port, host_name, true);

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

      if (button_state[i] == LOW && i == 0) {
        client->publish("stat/i3/inside/machineShop/motion-sensor/motion", "There You Are.");
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

