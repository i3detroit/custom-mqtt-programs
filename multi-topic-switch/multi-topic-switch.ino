#include "mqtt-wrapper.h"
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifndef NAME
#define NAME "NEW-multi-topic-switch"
#endif

#ifndef TOPIC
#define TOPIC "i3/program-me/NEW-multi-topic-switch"
#endif

#ifndef TOPIC_0_PIN
#define TOPIC_0_PIN 4
#endif

#ifndef TOPIC_0_LED
#define TOPIC_0_LED 0
#endif

#ifndef TOPIC_0_POLARITY
#define TOPIC_0_POLARITY 1
#endif

#ifndef TOPIC_0_TOPIC
#define TOPIC_0_TOPIC "i3/program-me/not-a-topic"
#endif

//Assume if topic 2-whatever is defined at all, everything is defined.

const char* host_name = NAME;
const char* fullTopic = TOPIC;
const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

char buf[1024];
char topicBuf[1024];


struct mqtt_wrapper_options mqtt_options;

// button pins
#ifndef TOPIC_1_TOPIC
const int button_pins[] = {TOPIC_0_PIN};
#else
#ifndef TOPIC_2_TOPIC
const int button_pins[] = {TOPIC_0_PIN, TOPIC_1_PIN};
#else
const int button_pins[] = {TOPIC_0_PIN, TOPIC_1_PIN, TOPIC_2_PIN};
#endif
#endif

//Debounce setup
int button_state[ARRAY_SIZE(button_pins)];
int button_state_last[ARRAY_SIZE(button_pins)];
int debounce[ARRAY_SIZE(button_pins)];
int debounce_time = 80;


void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  //Serial.print("Message arrived [");
  //Serial.print(topic);
  //Serial.print("] ");
  //for (int i = 0; i < length; i++) {
  //  Serial.print((char)payload[i]);
  //}
  //Serial.println();

  topic += 5;

  int polarity;
  int topicIndex = -1;
  if(strncmp(topic, TOPIC_0_TOPIC, strlen(TOPIC_0_TOPIC)) == 0) {
    topicIndex = 0;
    sprintf(topicBuf, "stat/%s", TOPIC_0_TOPIC);
    polarity = TOPIC_0_POLARITY;
  }
#ifdef TOPIC_1_TOPIC
  if(strncmp(topic, TOPIC_1_TOPIC, strlen(TOPIC_1_TOPIC)) == 0) {
    topicIndex = 1;
    sprintf(topicBuf, "stat/%s", TOPIC_1_TOPIC);
    polarity = TOPIC_1_POLARITY;
  }
#endif
#ifdef TOPIC_2_TOPIC
  if(strncmp(topic, TOPIC_2_TOPIC, strlen(TOPIC_2_TOPIC)) == 0) {
    sprintf(topicBuf, "stat/%s", TOPIC_2_TOPIC);
    topicIndex = 2;
    polarity = TOPIC_2_POLARITY;
  }
#endif

  client->publish(topicBuf, button_state[topicIndex] != polarity ? "OPEN" : "CLOSED");
}

void connectSuccess(PubSubClient* client, char* ip) {
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

  memset(button_state, -1, ARRAY_SIZE(button_pins));
  memset(button_state_last, -1, ARRAY_SIZE(button_pins));
  memset(debounce, 1, ARRAY_SIZE(button_pins));

  //input pins
  for (int i=0; i < ARRAY_SIZE(button_pins); ++i) {
    pinMode(button_pins[i], INPUT_PULLUP);
  }

  pinMode(TOPIC_0_LED, OUTPUT);
  digitalWrite(TOPIC_0_LED, digitalRead(TOPIC_0_PIN) != TOPIC_0_POLARITY);
#ifdef TOPIC_1_TOPIC
  pinMode(TOPIC_1_LED, OUTPUT);
  digitalWrite(TOPIC_1_LED, digitalRead(TOPIC_1_PIN) != TOPIC_1_POLARITY);
#endif
#ifdef TOPIC_2_TOPIC
  pinMode(TOPIC_2_LED, OUTPUT);
  digitalWrite(TOPIC_2_LED, digitalRead(TOPIC_2_PIN) != TOPIC_2_POLARITY);
#endif
}

void connectedLoop(PubSubClient* client) {
  //Foreach pin run debouncing, and if it's a real press call the function
  for(int i=0; i < ARRAY_SIZE(button_pins); ++i) {
    if(button_pins[i] == -1) continue;
    button_state[i] = digitalRead(button_pins[i]);//Read current state
    //If the current state does not equal the last state, AND it's been long enough since the last change
    unsigned long now = millis() + (unsigned long)-3000;
    if (button_state[i] != button_state_last[i] && now - debounce[i] > debounce_time) {

      bool polarity;
      int led;
      switch(i) {
        case 0:
          sprintf(topicBuf, "stat/%s", TOPIC_0_TOPIC);
          polarity = TOPIC_0_POLARITY;
          led = TOPIC_0_LED;
          break;
        case 1:
#ifdef TOPIC_1_TOPIC
          sprintf(topicBuf, "stat/%s", TOPIC_1_TOPIC);
          polarity = TOPIC_1_POLARITY;
          led = TOPIC_1_LED;
#else
          //Serial.println("Should not be?");
#endif
          break;
        case 2:
#ifdef TOPIC_2_TOPIC
          sprintf(topicBuf, "stat/%s", TOPIC_2_TOPIC);
          polarity = TOPIC_2_POLARITY;
          led = TOPIC_2_LED;
#else
          //Serial.println("Should not be?");
#endif
          break;
      }
      client->publish(topicBuf, button_state[i] != polarity ? "OPEN" : "CLOSED");
      digitalWrite(led, button_state[i] != polarity);

      //If the button was pressed or released, we still need to reset the debounce timer.
      button_state_last[i] = button_state[i];
      debounce[i] = millis();
    }
  }
}

void loop() {
  loop_mqtt();
}

