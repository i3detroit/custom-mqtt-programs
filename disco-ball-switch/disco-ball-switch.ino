#include "mqtt-wrapper.h"


// 15 12 13 RGB
#define LED_PIN 12

const char* topic = "i3/inside/commons/disco";

char buf[1024];
char topicBuf[1024];

bool control_state;


const char* host_name = "disco-ball-switch";
const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

// button pins
const int button_pins[] = {1, 0, 3};
const int numButtons = sizeof(button_pins)/sizeof(button_pins[0]);

//Debounce setup
int button_state[] = {1,1,1};
int button_state_last[] = {1,1,1};
int debounce[] = {0,0,0};
const int debounce_time = 50;

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
  //Serial.println("win");
  //subscribe and shit here
  //TODO:
  sprintf(topicBuf, "stat/%s-button/INFO2", topic);
  sprintf(buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", host_name, ip);
  client->publish(topicBuf, buf);
  sprintf(topicBuf, "stat/%s/POWER", topic);
  client->subscribe(topicBuf);
}

void setup() {
  //Serial.begin(115200);
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
      //If the button is pressed, call funciton
      if (button_state[i] == LOW) {
        sprintf(topicBuf, "cmnd/%s/POWER", topic);
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

