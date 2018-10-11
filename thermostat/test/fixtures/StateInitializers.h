#ifndef STATEINITIALIZERS_H
#define STATEINITIALIZERS_H
#endif

#include <string>
#include <stdbool.h>
#include "../../thermostat.h"

using namespace std;

class StateInitializers
{
	public:
		static struct ControlState initializeControlState(enum Mode modeToSet, bool fanToSet, int targetToSet, int swingToSet, int timeoutToSet);
		static struct SensorState initializeSensorState(float tempToSet, float pressureToSet, float humidityToSet, bool batteryPowerToSet, bool tempSensorToSet);
		static struct OutputState initializeOutputState(enum Mode currentMode, uint32_t timeLeftUntilNewCycleStarts, bool currentFanState, bool currentMqttOutputDirty);
};
