#ifndef __DOCONTROL_H_
#define __DOCONTROL_H_

#if !defined(ARDUINO_ESP8266_WEMOS_D1MINI) && !defined(ESP8266)
#include <stdint.h>
#include <stdbool.h>
uint32_t millis() { return 42; }
#else
#include <Arduino.h>
#endif

#include "thermostat.h"
#include "pins.h"

uint8_t doControl(struct ControlState* controlState, struct SensorState* sensorState, struct OutputState* outputState);

#endif
