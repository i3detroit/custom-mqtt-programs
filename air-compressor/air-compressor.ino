/*
 * Air Compressor
 **/
#include "mqtt-wrapper.h"

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
#define NAME "air-compressor"
#endif

#ifndef TOPIC
#define TOPIC "i3/inside/infrastructure/air-compressor"
#endif

#define ON_BUTTON 4
#define OFF_BUTTON 5

char buf[1024];

struct mqtt_wrapper_options mqtt_options;

const int button_pins[] = {12};

//Debounce setup
int button_state[] = {1};
int button_state_last[] = {-1};
int debounce[] = {0};
const int debounce_time = 1000;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  if (strcmp(topic, "power") == 0) {
    Serial.println("Got command to power");
    if((char)payload[0] == '0' || (char)payload[1] == 'F') {
      Serial.println("off");
      if(button_state_last[1]) {
        //If attempt to set to current state, just reply with current state
        client->publish("stat/i3/inside/infrastructure/air-compressor/POWER", button_state_last[1] ? "OFF" : "ON");
      }
      digitalWrite(OFF_BUTTON, 1);
      delay(750);
      digitalWrite(OFF_BUTTON, 0);
    } else if((char)payload[0] == '1' || (char)payload[1] == 'N') {
      Serial.println("on");
      if(!button_state_last[1]) {
        //If attempt to set to current state, just reply with current state
        client->publish("stat/i3/inside/infrastructure/air-compressor/POWER", button_state_last[1] ? "OFF" : "ON");
      }
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
}


void setup() {
  Serial.begin(115200);
  mqtt_options.connectedLoop = connectedLoop;
  mqtt_options.callback = callback;
  mqtt_options.connectSuccess = connectSuccess;
  mqtt_options.ssid = WIFI_SSID;
  mqtt_options.password = WIFI_PASSWORD;
  mqtt_options.mqtt_server = MQTT_SERVER;
  mqtt_options.mqtt_port = MQTT_PORT;
  mqtt_options.host_name = NAME;
  mqtt_options.fullTopic = TOPIC;
  mqtt_options.debug_print = true;
  setup_mqtt(&mqtt_options);

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
