#ifndef __THERMOSTAT_H_
#define __THERMOSTAT_H_

#include "pins.h"
#ifdef ESP8266
#include <Arduino.h>
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifndef NAME
#define NAME "NEW-thermostat"
#endif

#ifndef TOPIC
#define TOPIC "i3/program-me/NEW-thermostat"
#endif

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

//Set if no cool
#ifndef HEAT_ONLY
//#define HEAT_ONLY
#endif


#if TRUE
#ifdef !defined(ARDUINO_ESP8266_WEMOS_D1MINI) && !defined(ESP8266)
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#include <stdio.h>
#define DEBUG_PRINT(x) printf(x)
#define DEBUG_PRINTLN(x) printf(x); printf("\n")
#endif
#else
#define DEBUG_PRINT(x) do {} while (0)
#define DEBUG_PRINTLN(x) do {} while (0)
#endif


/*################################ EEPROM Section ##########################*/
#define EEPROM_MAGIC 0
#define EEPROM_MODE 1
#define EEPROM_SWING 2
#define EEPROM_TIMEOUT 3

#define EEPROM_SIZE 8

//magic numbers stored in eeprom to set boot state
#define MAGIC_EEPROM_NUMBER 0x49

/*################## parameter limits and defaults #########################*/
#define MIN_TIMEOUT 60*1000
//Like a day
#define MAX_TIMEOUT 100000000
#define DEFAULT_TIMEOUT_INTERVAL 3 * 60 * 60 * 1000

//default temps to go to based on state
#define DEFAULT_HEAT_TEMP 10
#define DEFAULT_COOL_TEMP 25

//limits for target temp; and will always heat below min temp
#define MIN_TEMP 5
#define MAX_TEMP 30

/*####################### state structures #################################*/
union TimeoutInterval {
    uint32_t timeout;
    uint8_t octets[4];
};

enum Mode { OFF = 0, HEAT, COOL };
struct ControlState {
  enum Mode mode;
  bool fan;
  int target;
  int swing;
  union TimeoutInterval timeout;
};
struct SensorState {
  float temp;
  float pressure;
  float humidity;
  bool batteryPower;
  bool tempSensor; //if sensor connected
};
struct OutputState {
  enum Mode mode;
  uint32_t fanDelayEnd;
  bool fan;
  bool mqttOutputDirty;
};

#endif
