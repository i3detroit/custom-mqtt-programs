/*
 * Based on https://github.com/i3detroit/mqtt-wrapper/commit/78ad5fba9084906cc02cff3403d9b388b0d5127a
 *
 */
#include "mqtt-wrapper.h"

#ifndef NAME
#define NAME "bumblebee_timer"
#endif

#ifndef TOPIC
#define TOPIC "i3/inside/laser-zone/bumblebee-timer"
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

struct mqtt_wrapper_options mqtt_options;

char topicBuf[1024];
const int bufferSize = 500;
char timerInputBuffer[bufferSize];
char outputBuffer[bufferSize];
int i = 0;
int j = 0;
bool isTime = 0;
bool isOnline = 0;
String tubeTime;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  
}
void connectSuccess(PubSubClient* client, char* ip) {
  
}
void setup() {
  pinMode(2, OUTPUT);
  digitalWrite(0, HIGH);
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
  isOnline = 1;
}
// Do the mqtt stuff:
void connectedLoop(PubSubClient* client) {
  if (isTime == 1){
    sprintf(topicBuf, "stat/%s/laser-tube-time", TOPIC);
    client->publish(topicBuf, outputBuffer);
  }
  if (isOnline == 1){
    sprintf(topicBuf, "stat/%s/status", TOPIC);
    client->publish(topicBuf, "Laser_Timer_Online");
  }
  isTime = 0;
  isOnline = 0;
}
bool isNumeric(String str, int index) {
  for (byte i = index; i < str.length(); i++) {
    if(!isDigit(str.charAt(i))) {
      return false;
    }
  }
  return true;
}

void loop() {
  digitalWrite(2, HIGH);
  // Watch serial for various values sent from timer
  // Serial read code borrowed from http://robotic-controls.com/learn/arduino/arduino-arduino-serial-communication
  j = 0;
  String timerInput = "\0";
  
  if (Serial.available()){
    delay(100);
    while(Serial.available() && j < bufferSize) {
      timerInputBuffer[j++] = Serial.read();
    }
    timerInputBuffer[j++] = '\0';
    timerInput = String(timerInputBuffer);
    /*for (byte j = 0; j<timerInput.length(); j++) {
      Serial.print("\nCharacter\n");
      Serial.println(timerInput.charAt(j)); 
      Serial.print("\nValue\n");
      Serial.print(timerInput.charAt(j));
    }*/
    Serial.println(timerInput);
    timerInput.trim(); // get rid of tailing carriage return that mucks up isNumeric()
    // Logical checks for trigger characters
    if (timerInput.charAt(0) == '&') {
      if (isNumeric(timerInput,1) && timerInput.length() > 1) {
        isTime = 1;
        tubeTime = timerInput.substring(1);
        tubeTime.toCharArray(outputBuffer,bufferSize);
        Serial.print("Time: ");
        Serial.println(tubeTime);
      }
      else Serial.print("(&) Input not recognized\n");
    }
    else if (timerInput.charAt(0) == '#') {
      isOnline = 1;
      Serial.print("Laser timer online\n");
    }
    else Serial.print("(other) Input not recognized\n");
  }
  // send mqtt stuff
  loop_mqtt();
}
