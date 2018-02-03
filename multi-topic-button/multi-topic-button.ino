#include "mqtt-wrapper.h"
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifndef NAME
#define NAME "NEW-multi-topic-button"
#endif

#ifndef TOPIC
#define TOPIC "i3/program-me/NEW-multi-topic-button"
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


#ifndef TOPIC_0_ON_PIN
#define TOPIC_0_ON_PIN 0
#endif

#ifndef TOPIC_0_OFF_PIN
#define TOPIC_0_OFF_PIN 2
#endif

#ifndef TOPIC_0_TOGGLE_PIN
#define TOPIC_0_TOGGLE_PIN 4
#endif

#ifndef TOPIC_0_LED_ON
#define TOPIC_0_LED_ON 5
#endif

#ifndef TOPIC_0_LED_OFF
#define TOPIC_0_LED_OFF 12
#endif

#ifndef TOPIC_0_TOPIC
#define TOPIC_0_TOPIC "i3/program-me/not-a-topic"
#endif

//Assume if topic 2-whatever is defined at all, everything is defined.

const char* host_name = NAME;
const char* fullTopic = TOPIC;
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;

char buf[1024];
char topicBuf[1024];


struct mqtt_wrapper_options mqtt_options;

// button pins
#ifndef TOPIC_1_TOPIC
const int button_pins[] = {TOPIC_0_ON_PIN, TOPIC_0_OFF_PIN, TOPIC_0_TOGGLE_PIN};
#else
#ifndef TOPIC_2_TOPIC
const int button_pins[] = {TOPIC_0_ON_PIN, TOPIC_0_OFF_PIN, TOPIC_0_TOGGLE_PIN, TOPIC_1_ON_PIN, TOPIC_1_OFF_PIN, TOPIC_1_TOGGLE_PIN};
#else
const int button_pins[] = {TOPIC_0_ON_PIN, TOPIC_0_OFF_PIN, TOPIC_0_TOGGLE_PIN, TOPIC_1_ON_PIN, TOPIC_1_OFF_PIN, TOPIC_1_TOGGLE_PIN, TOPIC_2_ON_PIN, TOPIC_2_OFF_PIN, TOPIC_2_TOGGLE_PIN};
#endif
#endif

//Debounce setup
int button_state[ARRAY_SIZE(button_pins)];
int button_state_last[ARRAY_SIZE(button_pins)];
int debounce[ARRAY_SIZE(button_pins)];
int debounce_time = 80;

bool control_state[ARRAY_SIZE(button_pins)/3];

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  //Serial.print("Message arrived [");
  //Serial.print(topic);
  //Serial.print("] ");
  //for (int i = 0; i < length; i++) {
  //  Serial.print((char)payload[i]);
  //}
  //Serial.println();

  topic += 5;

  int topicIndex = -1;
  if(strncmp(topic, TOPIC_0_TOPIC, strlen(TOPIC_0_TOPIC)) == 0) {
    topicIndex = 0;
  }
#ifdef TOPIC_1_TOPIC
  if(strncmp(topic, TOPIC_1_TOPIC, strlen(TOPIC_1_TOPIC)) == 0) {
    topicIndex = 1;
  }
#endif
#ifdef TOPIC_2_TOPIC
  if(strncmp(topic, TOPIC_2_TOPIC, strlen(TOPIC_2_TOPIC)) == 0) {
    topicIndex = 2;
  }
#endif

  if((char)payload[0] == 'O' && (char)payload[1] == 'N') {
    control_state[topicIndex] = true;
  } else if((char)payload[0] == 'O' && (char)payload[1] == 'F') {
    control_state[topicIndex] = false;
  }

  switch(topicIndex) {
    case 0:
      digitalWrite(TOPIC_0_LED_ON, control_state[topicIndex]);
      digitalWrite(TOPIC_0_LED_OFF, !control_state[topicIndex]);
      break;
#ifdef TOPIC_1_TOPIC
    case 1:
      digitalWrite(TOPIC_1_LED_ON, control_state[topicIndex]);
      digitalWrite(TOPIC_1_LED_OFF, !control_state[topicIndex]);
      break;
#endif
#ifdef TOPIC_2_TOPIC
    case 2:
      digitalWrite(TOPIC_2_LED_ON, control_state[topicIndex]);
      digitalWrite(TOPIC_2_LED_OFF, !control_state[topicIndex]);
      break;
#endif
  }

}

void connectSuccess(PubSubClient* client, char* ip) {
  //Subscribe to the result topic from the thing we control
  sprintf(topicBuf, "stat/%s/POWER", TOPIC_0_TOPIC);
  client->subscribe(topicBuf);
  sprintf(topicBuf, "cmnd/%s/POWER", TOPIC_0_TOPIC);
  client->publish(topicBuf, "");

#ifdef TOPIC_1_TOPIC
  sprintf(topicBuf, "stat/%s/POWER", TOPIC_1_TOPIC);
  client->subscribe(topicBuf);
  sprintf(topicBuf, "cmnd/%s/POWER", TOPIC_1_TOPIC);
  client->publish(topicBuf, "");
#endif
#ifdef TOPIC_2_TOPIC
  sprintf(topicBuf, "stat/%s/POWER", TOPIC_2_TOPIC);
  client->subscribe(topicBuf);
  sprintf(topicBuf, "cmnd/%s/POWER", TOPIC_2_TOPIC);
  client->publish(topicBuf, "");
#endif
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

  memset(button_state, 1, ARRAY_SIZE(button_pins));
  memset(button_state_last, 1, ARRAY_SIZE(button_pins));
  memset(debounce, 1, ARRAY_SIZE(button_pins));

  //input pins
  for (int i=0; i < ARRAY_SIZE(button_pins); ++i) {
    pinMode(button_pins[i], INPUT_PULLUP);
  }

  pinMode(TOPIC_0_LED_ON, OUTPUT);
  pinMode(TOPIC_0_LED_OFF, OUTPUT);
  digitalWrite(TOPIC_0_LED_ON, 0);
  digitalWrite(TOPIC_0_LED_OFF, 0);
#ifdef TOPIC_1_TOPIC
  pinMode(TOPIC_1_LED_ON, OUTPUT);
  pinMode(TOPIC_1_LED_OFF, OUTPUT);
  digitalWrite(TOPIC_1_LED_ON, 0);
  digitalWrite(TOPIC_1_LED_OFF, 0);
#endif
#ifdef TOPIC_2_TOPIC
  pinMode(TOPIC_2_LED_ON, OUTPUT);
  pinMode(TOPIC_2_LED_OFF, OUTPUT);
  digitalWrite(TOPIC_2_LED_ON, 0);
  digitalWrite(TOPIC_2_LED_OFF, 0);
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
      //If the button is pressed, call funciton
      if (button_state[i] == LOW) {
        sprintf(topicBuf, "info/%s/pressed", TOPIC);
        sprintf(buf, "pressed %d", i);
        client->publish(topicBuf,buf);
        if(i < 3) {
          sprintf(topicBuf, "cmnd/%s/POWER", TOPIC_0_TOPIC);
        } else if(i < 6) {
#ifdef TOPIC_1_TOPIC
          sprintf(topicBuf, "cmnd/%s/POWER", TOPIC_1_TOPIC);
#else
          //Serial.println("Should not be?");
#endif
        } else if(i < 9) {
#ifdef TOPIC_2_TOPIC
          sprintf(topicBuf, "cmnd/%s/POWER", TOPIC_2_TOPIC);
#else
          //Serial.println("Should not be?");
#endif
        } else {
          //Serial.println("Should not be, pt II?");
        }

        if(i%3 < 2) { //1 and 2 are set on and off
          //Serial.println("on or off");
          client->publish(topicBuf, i%3 == 0 ? "ON" : "OFF");
        } else {
          //Serial.println("toggle");
          client->publish(topicBuf, control_state[i/3] == false ? "ON" : "OFF");
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

