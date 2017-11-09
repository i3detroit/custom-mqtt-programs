/*
 * ESP8266 mqtt controlled 16 segment display
 *
 * https://abzman2k.wordpress.com/2015/09/23/esp8266-led-display/
 *
 * Version 1.1  9/23/2015  Evan Allen
 * Version 2    2017-09-11 Mark Furland
*/

#include "mqtt-wrapper.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

const char* host_name = "16-segment-display";
const char* ssid = "i3detroit-iot";
const char* password = "securityrisk";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

char buf[1024];
char displayString[1024];
int character = 0;
int d = 500;
long lastMsg = 0;

//char valid[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789- ";
char validateChar(char toValid) {
  if((65 <= toValid && toValid <= 90) || (48 <= toValid && toValid <= 57) || toValid == 32 || toValid == 45) {
    return toValid;
  } else {
    return '@';
  }
}


void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {

  if(strcmp(topic, "cmnd/i3/commons/16seg/delay") == 0) {
    char buf[5];
    for(int i=0; i<length && i<sizeof(buf)/sizeof(char); ++i){
      buf[i] = (char)payload[i];
    }
    d = atoi(buf);
  } else if (strcmp(topic, "cmnd/i3/commons/16seg/display") == 0) {
    //Serial.println("Got payload");
    //for (int i = 0; i < length; i++) {
    //  Serial.print((char)payload[i]);
    //}
    //Serial.println();
    //Serial.println(length);

    for (int k = 0; k < 16; k++) {
     // Serial.print('.');
      displayString[k] = '!';
    }
    displayString[16] = '\0';
    //Serial.println();

    int i=0;//displayString iterator
    int j=0;//payload iterator
    if(length < 16) {
      for (; i < 16 - length; i++) {
        displayString[i] = ' ';
      }
    }
    //Serial.print("'");
    //Serial.print(displayString);
    //Serial.println("'");

    for(; j<length && i<sizeof(displayString)/sizeof(char); ++i, ++j) {
      displayString[i] = validateChar(payload[j]);
    }
    displayString[i] = '\0';

    //Serial.print("'");
    //Serial.print(displayString);
    //Serial.println("'");


    for (int i = 0; i < 16; i++) {
      Serial.print(displayString[i]);
    }
    character = 16;
  }

}

void connectSuccess(PubSubClient* client, char* ip) {
  for(int i=0; i<16-4; ++i) {
      Serial.print(' ');
  }
  Serial.print(ip);

  //subscribe and shit here
  sprintf(buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", host_name, ip);
  client->publish("tele/i3/commons/16seg/INFO2", buf);
  client->subscribe("cmnd/i3/commons/16seg/display");
  client->subscribe("cmnd/i3/commons/16seg/delay");
}

void setup() {
  Serial.begin(4800);
  Serial.print("CONNECTING.....");

  setup_mqtt(connectedLoop, callback, connectSuccess, ssid, password, mqtt_server, mqtt_port, host_name, false);
}

void connectedLoop(PubSubClient* client) {
}

void loop() {
  loop_mqtt();

  if(strlen(displayString) > 16) {
    long now = millis();
    if (now - lastMsg > d) {
      lastMsg = now;
      //print next char
      Serial.print(displayString[character]);
      character = (character + 1) % strlen(displayString);
    }
  }
}
