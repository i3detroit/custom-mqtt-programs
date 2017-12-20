/*
 * mqtt glass door status reporter
 */
#include "mqtt-wrapper.h"

#define LED_PIN 2

//const char* topic = "i3/classroom/glass-door";

char buf[1024];


const char* host_name = "glass-door-reporter";
const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

// button pins 4 is lock, 5 is open
const int button_pins[] = {4, 5};
const int numButtons = sizeof(button_pins)/sizeof(button_pins[0]);

//Debounce setup
int button_state[] = {1,1};
int button_state_last[] = {-1,-1};
int debounce[] = {0,0};
const int debounce_time = 80;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  if (strcmp(topic, "cmnd/i3/classroom/glass-door/lock") == 0) {
    client->publish("stat/i3/classroom/glass-door/lock", button_state_last[0] ? "LOCKED" : "UNLOCKED");
  } else if (strcmp(topic, "cmnd/i3/classroom/glass-door/open") == 0) {
    client->publish("stat/i3/classroom/glass-door/open", button_state_last[1] ? "OPEN" : "CLOSED");
  }
}

void connectSuccess(PubSubClient* client, char* ip) {
  //subscribe and shit here
  sprintf(buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", host_name, ip);
  client->publish("tele/i3/classroom/glass-door/INFO2", buf);
  client->subscribe("cmnd/i3/classroom/glass-door/lock");
  client->subscribe("cmnd/i3/classroom/glass-door/open");
}

void setup() {
  setup_mqtt(connectedLoop, callback, connectSuccess, ssid, password, mqtt_server, mqtt_port, host_name, false);

  //input pins
  for (int i=0; i < numButtons; ++i) {
    pinMode(button_pins[i], INPUT_PULLUP);
  }

  pinMode(LED_PIN, OUTPUT);

}

void connectedLoop(PubSubClient* client) {
  //Foreach pin run debouncing, and if it's a real press call the function
  for(int i=0; i < numButtons; ++i) {
    button_state[i] = digitalRead(button_pins[i]);//Read current state
    //If the current state does not equal the last state, AND it's been long enough since the last change
    if (button_state[i] != button_state_last[i] && millis() - debounce[i] > debounce_time) {

      if(i == 0) {
        client->publish("stat/i3/classroom/glass-door/lock", button_state[i] ? "LOCKED" : "UNLOCKED");
        digitalWrite(LED_PIN, button_state[i] ? LOW : HIGH);
      } else if(i == 1) {
        client->publish("stat/i3/classroom/glass-door/open", button_state[i] ? "OPEN" : "CLOSED");
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

