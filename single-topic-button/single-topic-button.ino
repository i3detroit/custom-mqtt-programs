#include "mqtt-wrapper.h"

#define ON_PIN 0
#define OFF_PIN 2
#define TOGGLE_PIN 4
#define LED_PIN 5

#ifndef NAME
#define NAME "NEW-single-topic-button"
#endif

#ifndef TOPIC
#define TOPIC "i3/program-me/NEW-single-topic-button"
#endif

#ifndef CONTROL_TOPIC
#define CONTROL_TOPIC "i3/program-me/not-a-topic"
#endif

const char* host_name = NAME;
const char* fullTopic = TOPIC;
const char* controlTopic = CONTROL_TOPIC;
const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

char buf[1024];
char topicBuf[1024];

bool control_state;

struct mqtt_wrapper_options mqtt_options;

// button pins
const int button_pins[] = {ON_PIN, OFF_PIN, TOGGLE_PIN};
const int numButtons = sizeof(button_pins)/sizeof(button_pins[0]);

//Debounce setup
int button_state[] = {1,1,1};
int button_state_last[] = {1,1,1};
int debounce[] = {0,0,0};
int debounce_time = 50;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  //Serial.print("Message arrived [");
  //Serial.print(topic);
  //Serial.print("] ");
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
  }
  //Serial.println();

  if((char)payload[0] == 'O' && (char)payload[1] == 'N') {
    control_state = true;
  } else if((char)payload[0] == 'O' && (char)payload[1] == 'F') {
    control_state = false;
  }


  digitalWrite(LED_PIN, control_state);

}

void connectSuccess(PubSubClient* client, char* ip) {
  //Subscribe to the result topic from the thing we control
  sprintf(topicBuf, "stat/%s/POWER", controlTopic);
  client->subscribe(topicBuf);
  sprintf(topicBuf, "cmnd/%s/POWER", controlTopic);
  client->publish(topicBuf, "");
}

void setup() {
  //Serial.begin(115200);
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
    unsigned long now = millis() + (unsigned long)-3000;
    if (button_state[i] != button_state_last[i] && now - debounce[i] > debounce_time) {
      //If the button is pressed, call funciton
      if (button_state[i] == LOW) {
        sprintf(topicBuf, "cmnd/%s/POWER", controlTopic);
        if(i < 2) { //1 and 2 are set on and off
          //Serial.println("on or off");
          client->publish(topicBuf, i == 0 ? "ON" : "OFF");
        } else {
          //Serial.println("toggle");
          client->publish(topicBuf, control_state == false ? "ON" : "OFF");
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

