/*
 * mqtt laser cutter blast gate controller
 * 2018 Mike Fink
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
#define NAME "new-blast-gate-controller"
#endif

#ifndef TOPIC
#define TOPIC "i3/inside/laser-zone/new/blast-gate"
#endif

#ifndef DEVICE
#define DEVICE "new"
#endif

// input pins           // label on wemos D1 mini
#define OPENBUTTON   12 // D6 - Momentary push button
#define CLOSEBUTTON  13 // D7 - Momentary push button
#define GATESENSOR    4 // D2 - Limit switch on gate

//output pins
#define GATEOPEN      0 // D3 - Trigger actuator open
#define GATECLOSE     2 // D4 - Trigger actuator closed
#define OPENLED      14 // D5 - Button LED
#define CLOSEDLED    15 // D8 - Button LED

struct mqtt_wrapper_options mqtt_options;

char buf[1024];

const int button_pins[] = {OPENBUTTON, CLOSEBUTTON, GATESENSOR};
const int numButtons = sizeof(button_pins)/sizeof(button_pins[0]);

//Debounce setup
int button_state[] = {1,1,1};
int button_state_last[] = {-1,-1,-1};
int debounce[] = {0,0,0};
const int debounce_time = 80;

enum states{Open, Closed, Error, Off};
states requested_state = Off;
states requested_state_last = Off;
states actual_state = Off;
states actual_state_last = Off;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  if (strcmp((char*)payload, "open") == 0) {
    requested_state = Open;
  } else if (strcmp((char*)payload, "close") == 0) {
    requested_state = Closed;
  } else if (strcmp((char*)payload, "query") == 0) {
    client->publish("stat/i3/inside/laser-zone/"DEVICE"/blast-gate", button_state_last[2] ? "closed" : "open");
  } else {
    client->publish("stat/i3/inside/laser-zone/"DEVICE"/blast-gate", "bad command");
  }
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
  mqtt_options.debug_print = false;
  setup_mqtt(&mqtt_options);

  //input pins
  for (int i=0; i < numButtons; ++i) {
    pinMode(button_pins[i], INPUT_PULLUP);
  }
  //output pins
  pinMode(GATEOPEN, OUTPUT);
  pinMode(GATECLOSE, OUTPUT);
  pinMode(OPENLED, OUTPUT);
  pinMode(CLOSEDLED, OUTPUT);
  Serial.println("Setup");
}

void connectedLoop(PubSubClient* client) {
  // Read the inputs
  for(int i=0; i < numButtons; ++i) {
    button_state[i] = digitalRead(button_pins[i]);//Read current state
  }
  // Check for error state (open & closed both pressed)
  if (button_state[0] == LOW && button_state[1] == LOW) {
    requested_state = Error;
    Serial.println("Error");
  } else {
    // Check if the inputs have changed
    for(int i=0; i < numButtons; ++i) {
      // If the current state does not equal the last state, AND it's been long enough since the last change
      if (button_state[i] != button_state_last[i] && millis() - debounce[i] > debounce_time) {
        // Check the physical buttons
        if(i == 0 && button_state[i] == LOW) {
          requested_state = Open;
          Serial.println("Open pressed");
        } else if(i == 1 && button_state[i] == LOW) {
          requested_state = Closed;
          Serial.println("Close pressed");
        // Check the open/closed sensor
        } else if(i == 2) {
          if(button_state[i] == LOW) {
          	Serial.println("Limit pressed");
          	requested_state = Off;
            actual_state = Open;
          } else {
            actual_state = Closed;
          }
        }
        //If the button was pressed or released, we still need to reset the debounce timer.
        button_state_last[i] = button_state[i];
        debounce[i] = millis();
      }
    }
  }
  // Set the outputs if something has changed
  if (requested_state != requested_state_last) {
    requested_state_last = requested_state;
    if (requested_state == Open) {
      digitalWrite(GATECLOSE, HIGH);
      digitalWrite(GATEOPEN, HIGH);
      client->publish("stat/i3/inside/laser-zone/"DEVICE"/blast-gate", "opening");
      Serial.println("Pub opening");
    } else if (requested_state == Closed) {
      digitalWrite(OPENLED, LOW);
      digitalWrite(CLOSEDLED, HIGH);
      digitalWrite(GATEOPEN, LOW);
      digitalWrite(GATECLOSE, LOW);
      client->publish("stat/i3/inside/laser-zone/"DEVICE"/blast-gate", "closing");
      Serial.println("Pub closing");
    } else if (requested_state == Off) {
      digitalWrite(GATEOPEN, LOW);
      digitalWrite(GATECLOSE, HIGH);
    } else if (requested_state == Error) {
      client->publish("stat/i3/inside/laser-zone/"DEVICE"/blast-gate", "error");
      Serial.println("Pub error");
    }
  }
  if (actual_state != actual_state_last) {
    actual_state_last = actual_state;
    if (actual_state == Open) {
      client->publish("stat/i3/inside/laser-zone/"DEVICE"/blast-gate", "open");
      digitalWrite(CLOSEDLED, LOW);
      digitalWrite(OPENLED, HIGH);
      Serial.println("Is open");
    } else if (actual_state == Closed) {
      client->publish("stat/i3/inside/laser-zone/"DEVICE"/blast-gate", "closed");
      digitalWrite(OPENLED, LOW);
      digitalWrite(CLOSEDLED, HIGH);
      Serial.println("Is closed");
    }
  }
}

void loop() {
  loop_mqtt();
}
