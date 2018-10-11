#include "StateInitializers.h"

struct ControlState StateInitializers::initializeControlState(enum Mode modeToSet, bool fanToSet, int targetToSet, int swingToSet, int timeoutToSet)
{
	struct ControlState controlState;
	controlState.mode = modeToSet;
	controlState.fan = fanToSet;
	controlState.target = targetToSet;
	controlState.swing = swingToSet;
	controlState.timeout.timeout = timeoutToSet;
	return controlState;
}

//batteryPowerToSet: Is there a charged battery present?
//tempSensorToSet: Is there a temperature sensor reporting?
struct SensorState StateInitializers::initializeSensorState(float tempToSet, float pressureToSet, float humidityToSet, bool batteryPowerToSet, bool tempSensorToSet)
{
	struct SensorState sensorState;
	sensorState.temp = tempToSet;
	sensorState.pressure = pressureToSet;
	sensorState.humidity = humidityToSet;
	sensorState.batteryPower = batteryPowerToSet;
	sensorState.tempSensor = tempSensorToSet;
	return sensorState;
}
		
struct OutputState StateInitializers::initializeOutputState(enum Mode currentMode, uint32_t timeLeftUntilNewCycleStarts, bool currentFanState, bool currentMqttOutputDirty)
{
	struct OutputState outputState;
	outputState.mode = currentMode;
	outputState.fanDelayEnd = timeLeftUntilNewCycleStarts;
	outputState.fan = currentFanState;
	outputState.mqttOutputDirty = currentMqttOutputDirty;
	return outputState;
}
