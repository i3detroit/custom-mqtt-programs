/*
 * exit indicator
 */
#include "mqtt-wrapper.h"
#include <pcf8574_esp.h>
#include <Wire.h>

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



char buf[1024];

const char* host_name = "front-door-funnel";
const char* fullTopic = "i3/inside/commons/front-door-funnel";
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;

struct mqtt_wrapper_options mqtt_options;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  for(int i=0; i<length; i++) {
    buf[i] = (char)payload[i];
  }
  buf[length] = '\0';
  if (strcmp(topic, "stat/i3/inside/classroom/glass-door/lock") == 0) {
    client->publish("cmnd/i3/inside/commons/front-door-indicator/glass", buf);
  } else if (strcmp(topic, "stat/i3/inside/infrastructure/air-compressor/POWER") == 0) {
    client->publish("cmnd/i3/inside/commons/front-door-indicator/air", buf);
  } else if (strcmp(topic, "stat/i3/inside/weld-zone/tank-sensors/argon") == 0) {
    client->publish("cmnd/i3/inside/commons/front-door-indicator/argon", buf);
  } else {
    //this device is being queried
    client->publish("cmnd/i3/inside/classroom/glass-door/lock", "query");
    client->publish("cmnd/i3/inside/infrastructure/air-compressor/POWER", "query");
    client->publish("cmnd/i3/inside/weld-zone/tank-sensors/argon", "query");
  }
}
void connectSuccess(PubSubClient* client, char* ip) {
  client->subscribe("stat/i3/inside/classroom/glass-door/lock");
  client->publish("cmnd/i3/inside/classroom/glass-door/lock", "query");
  client->subscribe("stat/i3/inside/infrastructure/air-compressor/POWER");
  client->publish("cmnd/i3/inside/infrastructure/air-compressor/POWER", "query");

  client->subscribe("stat/i3/inside/weld-zone/tank-sensors/argon");
  client->publish("cmnd/i3/inside/weld-zone/tank-sensors/argon", "query");
}


void setup() {
  Serial.begin(115200);
  mqtt_options.connectedLoop = connectedLoop;
  mqtt_options.callback = callback;
  mqtt_options.connectSuccess = connectSuccess;
  mqtt_options.ssid = ssid;
  mqtt_options.password = password;
  mqtt_options.mqtt_server = mqtt_server;
  mqtt_options.mqtt_port = mqtt_port;
  mqtt_options.host_name = host_name;
  mqtt_options.fullTopic = fullTopic;
  mqtt_options.debug_print = true;
  setup_mqtt(&mqtt_options);
}

void connectedLoop(PubSubClient* client) {
}

void loop() {
  loop_mqtt();
}
