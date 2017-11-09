/*
 * Air Compressor
 **/
#include "mqtt-wrapper.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define ON_BUTTON 4
#define OFF_BUTTON 5

char buf[1024];

const char* host_name = "air-compressor";
const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

const int button_pins[] = {12};

//Debounce setup
int button_state[] = {1};
int button_state_last[] = {-1};
int debounce[] = {0};
const int debounce_time = 1000;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  if (strcmp(topic, "cmnd/i3/inside/infrastructure/air-compressor/POWER") == 0) {
    Serial.println("Got command to power");
    if((char)payload[0] == '0' || (char)payload[1] == 'F') {
      Serial.println("off");
      digitalWrite(OFF_BUTTON, 1);
      delay(750);
      digitalWrite(OFF_BUTTON, 0);
    } else if((char)payload[0] == '1' || (char)payload[1] == 'N') {
      Serial.println("on");
      digitalWrite(ON_BUTTON, 1);
      delay(750);
      digitalWrite(ON_BUTTON, 0);
    } else {
      //A query
      client->publish("stat/i3/inside/infrastructure/air-compressor/POWER", button_state_last[1] ? "OFF" : "ON");
    }
  }
}

void connectSuccess(PubSubClient* client, char* ip) {
  //subscribe and shit here
  sprintf(buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", host_name, ip);
  client->publish("tele/i3/inside/infrastructure/air-compressor/INFO2", buf);
  client->subscribe("cmnd/i3/inside/infrastructure/air-compressor/POWER");
}


void setup() {
  Serial.begin(115200);
  setup_mqtt(connectedLoop, callback, connectSuccess, ssid, password, mqtt_server, mqtt_port, host_name);

  //input pins
  for (int i=0; i < ARRAY_SIZE(button_pins); ++i) {
    pinMode(button_pins[i], INPUT_PULLUP);
  }
  pinMode(ON_BUTTON, OUTPUT);
  pinMode(OFF_BUTTON, OUTPUT);
}

void connectedLoop(PubSubClient* client) {
  for(int i=0; i < ARRAY_SIZE(button_pins); ++i) {
    button_state[i] = digitalRead(button_pins[i]);//Read current state
    //If the current state does not equal the last state, AND it's been long enough since the last change
    if (button_state[i] != button_state_last[i] && millis() - debounce[i] > debounce_time) {

      if(i == 0) {
        client->publish("stat/i3/inside/infrastructure/air-compressor/POWER", button_state[i] ? "OFF" : "ON");
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
