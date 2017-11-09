/*
 * ESP8266 mqtt controlled 16 segment display
 *
 * Version 1.1  9/23/2015  Evan Allen
 * Version 2    2017-09-11 Mark Furland
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";

WiFiClient espClient;
PubSubClient client(espClient);



char displayString[1024];
int character = 0;
int d = 500;
long lastMsg = 0;
//char valid[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789- ";

void custom_setup() {
}

void custom_loop() {
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

char validateChar(char toValid) {
  if((65 <= toValid && toValid <= 90) || (48 <= toValid && toValid <= 57) || toValid == 32 || toValid == 45) {
    return toValid;
  } else {
    return '@';
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
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

void setup_wifi() {
  delay(10);
  Serial.print("CONNECTING.....");
  // We start by connecting to a WiFi network

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  randomSeed(micros());
  for(int i=0; i<16-4; ++i) {
    Serial.print(' ');
  }
  Serial.print(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    if (client.connect("16seg")) {
      // Once connected, publish an announcement...
      client.publish("stat/i3/commons/16seg/status", "online");
      // ... and resubscribe
      client.subscribe("cmnd/i3/commons/16seg/display");
      client.subscribe("cmnd/i3/commons/16seg/delay");
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {

  Serial.begin(4800);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  custom_setup();
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  custom_loop();
}
