#ifndef __DOCONTROL_H_
	#define __DOCONTROL_H_
	#include <stdint.h>
	#include <stdbool.h>
	#include "thermostat.h"
	#include "pins.h"

	uint8_t doControl(struct ControlState* controlState, struct SensorState* sensorState, struct OutputState* outputState);

#endif
