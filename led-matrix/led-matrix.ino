/*
 * led dot matrix sign
 */
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
#define NAME "led-matrix-controller"
#endif

#ifndef TOPIC
#define TOPIC "i3/inside/commons/led-matrix"
#endif

struct mqtt_wrapper_options mqtt_options;

const char reset_packet[] = { 0x03, 0x7F, 0x07, 0x41, 0x10, 0x00, 0x26 };


char buf[1024];
byte packetBuf[44]; //TODO: shortern

int calculateChecksum(byte* packet, int len) {
  int sum = 0;
  for(int i = 0; i < len; i++) {
    sum += packet[i];
  }
  return (~sum % 256) + 1;
}

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {

  sprintf(buf, "stat/%s/status", TOPIC);
  if (strcmp(topic, "display") == 0) {

    for(int i = 0; i < ARRAY_SIZE(reset_packet); i++) {
      Serial.write(reset_packet[i]);
      //Serial.println(reset_packet[i],HEX);
    }
    delay(100);
    //iterate over payload in 14 byte chunks

    for(int i = 0; i <= length / 14; i++) {
      int pLength;
      if(14*i + 14 < length) {
        pLength = 14;
      } else {
        pLength = length - 14*i;
      }


      packetBuf[0] = 0x03; //message ID
      packetBuf[1] = 0x7F; // 7F
      packetBuf[2] = 7 + pLength; //number of bytes in packet
      packetBuf[3] = 0; // transmission ID
      //dunno how this one works.
      //txtChunks is the array of 14 character chunks, count is which one they are on
      //p.append(chr((len(txtChunks))*16 + count))   #number of packets in this transmission + this packet's position
      packetBuf[4] = (length/14 + (length%14 == 0 ? 0 : 1))*16 + i; // number of packets in transmission, shifted to the upper half | this pakcets position
      for(int j=0; j < pLength; ++j) {
        packetBuf[5+j] = payload[14*i + j];
      }
      packetBuf[5+pLength] = 0;
      packetBuf[6+pLength] = calculateChecksum(packetBuf,5+pLength);
      for(int k=0; k<7+pLength; k++) {
        Serial.write(packetBuf[k]);
        //Serial.println(packetBuf[k],HEX);
      }
    }


    client->publish(buf, "probably drew");
  } else {
    client->publish(buf, "Wot?");
  }

}

void connectSuccess(PubSubClient* client, char* ip) {
}

void setup() {
  Serial.begin(9600);
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

void connectedLoop(PubSubClient* client) {
}

void loop() {
  loop_mqtt();

}

