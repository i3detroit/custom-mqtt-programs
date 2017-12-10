/*
 * led dot matrix sign
 */
#include "mqtt-wrapper.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))


const char reset_packet[] = { 0x03, 0x7F, 0x07, 0x41, 0x10, 0x00, 0x26 };


char buf[1024];
byte packetBuf[44]; //TODO: shortern

const char* host_name = "led-matrix-controller";
const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

int calculateChecksum(byte* packet, int len) {
  int sum = 0;
  for(int i = 0; i < len; i++) {
    sum += packet[i];
  }
  return (~sum % 256) + 1;
}

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {

  if (strcmp(topic, "cmnd/i3/commons/ledMatrix/display") == 0) {

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


    client->publish("stat/i3/commons/ledMatrix/status", "probably drew");
  } else {
    client->publish("stat/i3/commons/ledMatrix/status", "Wot?");
  }

}

void connectSuccess(PubSubClient* client, char* ip) {
  //subscribe and shit here
  sprintf(buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", host_name, ip);
  client->publish("tele/i3/commons/ledMatrix/INFO2", buf);
  client->subscribe("cmnd/i3/commons/ledMatrix/display");
  client->subscribe("cmnd/i3/commons/ledMatrix/reset");
}

void setup() {
  Serial.begin(9600);
  setup_mqtt(connectedLoop, callback, connectSuccess, ssid, password, mqtt_server, mqtt_port, host_name, false);
}

void connectedLoop(PubSubClient* client) {
}

void loop() {
  loop_mqtt();

}

