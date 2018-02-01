/*
 * Based on https://github.com/i3detroit/mqtt-wrapper/commit/78ad5fba9084906cc02cff3403d9b388b0d5127a
 *
 */
#include <Adafruit_NeoPixel.h>
#include "mqtt-wrapper.h"

#ifndef NAME
#define NAME "new-laser-status-lights"
#endif

#ifndef TOPIC
#define TOPIC "i3/inside/laser-zone/new/status_lights"
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

#ifndef DEVICE
#define DEVICE "new"
#endif

#define PIXEL_PIN     13 //nodemcu/d1 mini D7
#define NUM_PIXELS    8

#define TRUE    1
#define FALSE   0

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

int brightness = 5; // scale led brightness from 0 to 10
uint32_t ledRed = pixels.Color(25*brightness,0,0);
uint32_t ledBlue = pixels.Color(0,0,25*brightness);
uint32_t ledOff = pixels.Color(0,0,0);
int delayVal = 50;
int millisLast = 0;

struct mqtt_wrapper_options mqtt_options;

bool chillerOn = FALSE;
bool compressorOn = FALSE;
bool ventFanOn = FALSE;
bool ventFanGateOpen = FALSE;
bool laserOn = FALSE;

int x = 0;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  if (strcmp(topic, "stat/i3/inside/laser-zone/"DEVICE"/chiller/POWER") == 0) {
    if ((char)payload[0] == '0' || (char)payload[1] == 'F') {
      // chiller is off
      chillerOn = FALSE;
    } else if ((char)payload[0] == '1' || (char)payload[1] == 'N') {
      // chiller is on
      chillerOn = TRUE;
    }
  } else if (strcmp(topic, "stat/i3/inside/infrastructure/air-compressor/POWER") == 0) {
    if ((char)payload[0] == '0' || (char)payload[1] == 'F') {
      // compressor is off
      compressorOn = FALSE;
    } else if ((char)payload[0] == '1' || (char)payload[1] == 'N') {
      // compressor is on1
      compressorOn = TRUE;
    }
  } else if (strcmp(topic, "stat/i3/inside/laser-zone/vent-fan/POWER") == 0) {
    if ((char)payload[0] == '0' || (char)payload[1] == 'F') {
      // vent fan is off
      ventFanOn = FALSE;
    } else if ((char)payload[0] == '1' || (char)payload[1] == 'N') {
      // vent fan is on
      ventFanOn = TRUE;
    }
  } else if (strcmp(topic, "stat/i3/inside/laser-zone/"DEVICE"/vent-fan-gate") == 0) {
    if ((char)payload[0] == 'c') {
      // blast gate is closed
      ventFanGateOpen = FALSE;
    } else if ((char)payload[0] == 'o') {
      // blast gate is open
      ventFanGateOpen = TRUE;
    }
  } else if (strcmp(topic, "stat/i3/inside/laser-zone/"DEVICE"/laser/POWER") == 0) {
    if ((char)payload[0] == '0' || (char)payload[1] == 'F') {
      // chiller is off
      laserOn = FALSE;
      Serial.print("laser is on");
    } else if ((char)payload[0] == '1' || (char)payload[1] == 'N') {
      // chiller is on
      laserOn = TRUE;
      Serial.print("laser is off");
    }
  }
}

void connectSuccess(PubSubClient* client, char* ip) {
  client->subscribe("stat/i3/inside/laser-zone/"DEVICE"/#");
  client->subscribe("stat/i3/inside/laser-zone/vent-fan/POWER");
  client->subscribe("stat/i3/inside/infrastructure/air-compressor/POWER");
  client->publish("cmnd/i3/inside/laser-zone/vent-fan/POWER", "");
  client->publish("cmnd/i3/inside/infrastructure/air-compressor/POWER", "");
  client->publish("cmnd/i3/inside/laser-zone/"DEVICE"/chiller/POWER", "");
  client->publish("cmnd/i3/inside/laser-zone/blast-gate-monitor/query", "");
}

void setup() {
  pixels.begin();
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
}

// Do the mqtt stuff:
void connectedLoop(PubSubClient* client) {

}

void loop() {
  if (laserOn) {
    if (chillerOn) {
      pixels.setPixelColor(7,ledBlue);
      pixels.setPixelColor(6,ledOff);
    } else if (!chillerOn) {
      pixels.setPixelColor(7,ledOff);
      pixels.setPixelColor(6,ledRed);
    }
    if (compressorOn) {
      pixels.setPixelColor(5,ledBlue);
      pixels.setPixelColor(4,ledOff);
    } else if (!compressorOn) {
      pixels.setPixelColor(5,ledOff);
      pixels.setPixelColor(4,ledRed);
    }
    if (ventFanOn) {
      pixels.setPixelColor(3,ledBlue);
      pixels.setPixelColor(2,ledOff);
    } else if (!ventFanOn) {
      pixels.setPixelColor(3,ledOff);
      pixels.setPixelColor(2,ledRed);
    }
    if (ventFanGateOpen) {
      pixels.setPixelColor(1,ledBlue);
      pixels.setPixelColor(0,ledOff);
    } else if (!ventFanGateOpen) {
      pixels.setPixelColor(1,ledOff);
      pixels.setPixelColor(0,ledRed);
    }
  } else if (!laserOn) {
    for (x = 0; x < NUM_PIXELS; x++) {
      pixels.setPixelColor(x,ledOff);
    }
  }
  if (millis() - millisLast >= delayVal) {
    pixels.show();
    millisLast = millis();
  }

  loop_mqtt();
}
