/*
 * ESP8266 light button
 *
 * debouncing from https://github.com/mtfurlan/arduinoKeypad
 * mqtt from PubSubClient Example
*/

#include "mqtt-wrapper.h"
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifndef NAME
#define NAME "NEW-middle-east-light-switches"
#endif

#ifndef TOPIC
#define TOPIC "i3/program-me/NEW-middle-east-light-switches"
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

// button pins
const int button_pins[] = { 4, 5, 0, 2 };
const int numButtons = sizeof(button_pins)/sizeof(button_pins[0]);

//This is an array of function pointers. It relates to the button on the same pin in the button_pins array.
void (*button_functions[])(PubSubClient* client) = {&westOn, &westOff, &eastOn, &eastOff};

char buf[1024];
char topicBuf[1024];

//Debounce setup
int button_state[] = {1,1,1,1,1,1,1};
int button_state_last[] = {1,1,1,1,1,1,1};
int debounce[] = {0,0,0,0,0,0,0};
const int debounce_time = 50;

void westOn(PubSubClient* client) { // pin 4
  sprintf(buf, "info/%s/pressed", TOPIC);
  client->publish(buf,"westOn");
  client->publish("cmnd/i3/inside/lights/020/POWER", "1");
  client->publish("cmnd/i3/inside/lights/021/POWER", "1");
  client->publish("cmnd/i3/inside/lights/022/POWER", "1");
  client->publish("cmnd/i3/inside/lights/023/POWER", "1");
  client->publish("cmnd/i3/inside/lights/024/POWER", "1");
  client->publish("cmnd/i3/inside/lights/025/POWER", "1");
  client->publish("cmnd/i3/inside/lights/026/POWER", "1");
}
void westOff(PubSubClient* client) { // pin 5
  sprintf(buf, "info/%s/pressed", TOPIC);
  client->publish(buf,"westOff");
  client->publish("cmnd/i3/inside/lights/020/POWER", "0");
  client->publish("cmnd/i3/inside/lights/021/POWER", "0");
  client->publish("cmnd/i3/inside/lights/022/POWER", "0");
  client->publish("cmnd/i3/inside/lights/023/POWER", "0");
  client->publish("cmnd/i3/inside/lights/024/POWER", "0");
  client->publish("cmnd/i3/inside/lights/025/POWER", "0");
  client->publish("cmnd/i3/inside/lights/026/POWER", "0");
}
void eastOn(PubSubClient* client) { // pin 0
  sprintf(buf, "info/%s/pressed", TOPIC);
  client->publish(buf,"eastOn");
  client->publish("cmnd/i3/inside/lights/015/POWER", "1");
  client->publish("cmnd/i3/inside/lights/016/POWER", "1");
  client->publish("cmnd/i3/inside/lights/017/POWER", "1");
  client->publish("cmnd/i3/inside/lights/018/POWER", "1");
  client->publish("cmnd/i3/inside/lights/019/POWER", "1");
}
void eastOff(PubSubClient* client) { // pin 2
  sprintf(buf, "info/%s/pressed", TOPIC);
  client->publish(buf,"eastOff");
  client->publish("cmnd/i3/inside/lights/015/POWER", "0");
  client->publish("cmnd/i3/inside/lights/016/POWER", "0");
  client->publish("cmnd/i3/inside/lights/017/POWER", "0");
  client->publish("cmnd/i3/inside/lights/018/POWER", "0");
  client->publish("cmnd/i3/inside/lights/019/POWER", "0");
}

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
  for (int i=0; i < ARRAY_SIZE(button_pins); ++i) {
    pinMode(button_pins[i], INPUT_PULLUP);
  }

}

void connectedLoop(PubSubClient* client) {
  for(int i=0; i < ARRAY_SIZE(button_pins); ++i) {
    button_state[i] = digitalRead(button_pins[i]);//Read current state
    //If the current state does not equal the last state, AND it's been long enough since the last change
    if (button_state[i] != button_state_last[i] && millis() - debounce[i] > debounce_time) {
      if (button_state[i] == LOW) {
        button_functions[i](client);
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
