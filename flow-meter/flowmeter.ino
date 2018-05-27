/* 
 MQTT enabled flow meter
 Based on https://github.com/abzman/esp-stuff/blob/master/esp_mqtt_flow/esp_mqtt_flow.ino
 Mike Fink
*/
#include "mqtt-wrapper.h"

#ifndef WIFI_SSID
#define WIFI_SSID "i3detroit-iot"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "securityrisk"
#endif

#ifndef MQTT_SERVER
#define MQTT_SERVER "10.13.0.22"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef NAME
#define NAME "water-cooler-flow-meter"
#endif

#ifndef TOPIC
#define TOPIC "i3/inside/commons/water-cooler"
#endif

struct mqtt_wrapper_options mqtt_options;
char buf[1024];

int flowPin = 5;
unsigned long flowCount = 0;
unsigned long flowCountOut = 0;
unsigned long time = 0;
unsigned long lastTime = 0;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
}

void connectSuccess(PubSubClient* client, char* ip) {
}

void setup() {
  Serial.begin(115200);
  Serial.println("Water cooler setup...");
  pinMode(flowPin, INPUT_PULLUP);
  attachInterrupt(flowPin, flow, CHANGE);
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
}

void connectedLoop(PubSubClient* client) {
  time = millis();
  if (time - lastTime > 30000) {
    lastTime = time;
    flowCountOut = flowCount;
    flowCount = flowCount - flowCountOut;
    Serial.print("Flow in pulses: ");
    Serial.println(flowCountOut);
    sprintf(buf, "%lu", flowCountOut);
    client->publish("stat/i3/inside/commons/water-cooler/flow", buf);
  }
}

void loop() {
  loop_mqtt();
}

void flow()
{
  flowCount +=1;
}
