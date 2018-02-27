#include <Wire.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include "mqtt-wrapper.h"

#ifndef NAME
#define NAME "NEW-sensor-test"
#endif

#ifndef TOPIC
#define TOPIC "i3/program-me/NEW-sensor-test"
#endif

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

char buf[1024];
char topicBuf[1024];
char floatBuf[16];

Adafruit_BME280 bme;
bool bmeConnected = true;
BH1750 lightMeter(0x23);

unsigned long status = 0UL;
unsigned long statusInterval = 60000UL;

struct mqtt_wrapper_options mqtt_options;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
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

  Wire.begin(4, 5);

  if (!bme.begin(0x76)) {
    Serial.println("Could not find BME 280 sensor");
    bmeConnected = false;
  }
  lightMeter.begin(BH1750_CONTINUOUS_HIGH_RES_MODE);
}

void connectedLoop(PubSubClient* client) {
  if( (long)( millis() - status ) >= 0) {
    status = millis() + statusInterval;

    if(bmeConnected) {
      sprintf(topicBuf, "tele/%s/bme280", TOPIC);
      dtostrf(bme.readTemperature(), 0, 2,floatBuf);
      sprintf(buf, "{\"Temperature\":%s, \"Pressure\":", floatBuf);
      dtostrf(bme.readPressure(), 0, 2,floatBuf);
      sprintf(buf + strlen(buf), "%s, \"Humidity\":", floatBuf);
      dtostrf(bme.readHumidity(), 0, 2,floatBuf);
      sprintf(buf + strlen(buf), "%s}", floatBuf);
      client->publish(topicBuf, buf);
    } else {
      sprintf(buf, "bme280 DISCONNECTED");
      sprintf(topicBuf, "tele/%s/error", TOPIC);
      client->publish(topicBuf, buf);
    }

    Serial.println("bme280");
    Serial.println(buf);

    sprintf(topicBuf, "tele/%s/lux", TOPIC);
    sprintf(buf, "{\"Lux\":%d}", lightMeter.readLightLevel());
    client->publish(topicBuf, buf);
    Serial.println(buf);
  }
}

void loop() {
  loop_mqtt();
}
