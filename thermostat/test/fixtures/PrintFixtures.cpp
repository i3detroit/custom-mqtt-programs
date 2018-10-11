#include "PrintFixtures.h"
#include "MillisFixture.h"
#include <stdio.h>
#include <stdint.h>
#include <math.h>

//TODO Add method to print the detailed dump to temp file.
const string PrintFixtures::eightySeperator = "===============================================================================";
const string PrintFixtures::debugSeperator = "=================================DEBUG INFO====================================";
const string PrintFixtures::newline = "\n";

string PrintFixtures::modeToString(enum Mode modeToConvert)
{
	switch(modeToConvert)
	{
		case Mode::HEAT:
			return "HEAT";
		case Mode::COOL:
			return "COOL";
		case Mode::OFF:
			return "OFF";
		default:
			return "OFF";
	};
}
string PrintFixtures::centerEighty(string toCenter)
{
	int toCenterLength = toCenter.length();
	if (toCenterLength <= 80)
	{
		int sideLength = floor((80 - toCenterLength)/2);
		toCenter = toCenter.insert(0, sideLength, '=').insert(sideLength + toCenterLength, sideLength, '=');
		while(toCenter.length() < 80)
		{
			toCenter = toCenter + "=";
		}
	}

	return toCenter;
}

string PrintFixtures::boolToOnOff(bool b)
{
	return b == true ? "ON" : "OFF";
}

string PrintFixtures::boolToString(bool b)
{
	return b == true ? "true" : "false";
}

string PrintFixtures::buildFanVariablesStatusMessage(bool fanLastState, bool enabled, bool forceFan, uint8_t fanDelayEnd)
{
	return string("\n" + centerEighty("Calculated Fan Table Variables After doControl Runs")) +
		"\n\t" + "- The fan's last state was " 										+ boolToOnOff(fanLastState) +
		"\n\t" + "- The state requested by the controller ('force fan') was " 		+ boolToOnOff(forceFan) +
		"\n\t" + "- Enabled was " 													+ boolToString(enabled) +
		"\n\t" + "- Fan delay length requirement met ('fandelay long enough')? " 	+
		"\n\t" + "-- millis(): " 													+ to_string(millis()) +
		"\n\t" + "-- Fan delay end: "												+ to_string(fanDelayEnd) +
		"\n\t" + "-- millis() >= fanDelayEnd => " 									+ boolToString(millis() >= fanDelayEnd) + ".";
}

string PrintFixtures::buildPinDebuggingMessage(
		struct ControlState* controlStateBefore,
		struct OutputState* outputStateBefore,
		string message,
		struct ControlState* controlStateAfter,
		struct SensorState* sensorStateAfter,
		struct OutputState* outputStateAfter,
		uint8_t pins)
{
	return "\n" + debugSeperator +
			"\n" + message +
			controlStateToString(controlStateBefore, "Before") +
			outputStateToString(outputStateBefore, "Before") +
			pinsToString(pins, " After") +
			controlStateToString(controlStateAfter, " After") +
			sensorStateToString(sensorStateAfter, " After") +
			outputStateToString(outputStateAfter, " After") +
			"\n" + eightySeperator;
}

string PrintFixtures::buildFanDebuggingMessage(
		struct ControlState* controlStateBefore,
		struct SensorState* sensorStateBefore,
		struct OutputState* outputStateBefore,
		string expectedState,
		string calculatedVariables,
		struct OutputState* outputStateAfter,
		uint8_t pins)
{
	return "\n" + debugSeperator +
			"\n" + expectedState +
			controlStateToString(controlStateBefore, " Before") +
			outputStateToString(outputStateBefore, " Before") +
			sensorStateToString(sensorStateBefore, " Before") +
			pinsToString(pins, " After") +
			outputStateToString(outputStateAfter, " After") +
			calculatedVariables +
			"\n" + eightySeperator;
}

string PrintFixtures::pinsToString(uint8_t pins, string tag)
{
	return string("\n") + centerEighty("Pins" + tag) +
			"\n\tLED_COOL: "       + boolToOnOff(((int) pins & 128) > 0) +
			"\n\tLED_OFF: "        + boolToOnOff(((int) pins &  64) > 0) +
			"\n\tLED_HEAT: "       + boolToOnOff(((int) pins &  32) > 0) +
			"\n\tLED_FAN_AUTO: "   + boolToOnOff(((int) pins &  16) > 0) +
			"\n\tLED_FAN_ON: "     + boolToOnOff(((int) pins &   8) > 0) +
			"\n\tFAN_CONTROL: "    + boolToOnOff(((int) pins &   4) > 0) +
			"\n\tENABLE_CONTROL: " + boolToOnOff(((int) pins &   2) > 0) +
			"\n\tSELECT_CONTROL: " + boolToOnOff(((int) pins &   1) > 0);
}

string PrintFixtures::controlStateToString(struct ControlState* state, string tag)
{
	//Not sure why, but the compiler complains that these are char* unless the first argument is explicitly typed as a string.
	return string("\n") + centerEighty("Control State" + tag) +
		"\n\tMode: " 					+ modeToString(state->mode) +
		"\n\tFan Override: " 			+ boolToOnOff(state->fan) +
		"\n\tTargeted Temperature: " 	+ to_string(state->target) +
		"\n\tSwing: " 				+ to_string(state->swing) +
		"\n\tTargeted timeout: " 		+ to_string(state->timeout.timeout) + ".";
}
	
string PrintFixtures::sensorStateToString(struct SensorState* state, string tag)
{
	return string("\n") + centerEighty("Sensor State" + tag) +
		"\n\tAmbient Temperature: " 		+ to_string(state->temp) +
		"\n\tAmbient Pressure: " 			+ to_string(state->pressure) +
		"\n\tAmbient Humidity: "	 		+ to_string(state->humidity) +
		"\n\tPowered Battery Present: " 	+ boolToString(state->batteryPower) +
		"\n\tTemperature Sensor Present: "+ boolToString(state->tempSensor) + ".";
}
		
string PrintFixtures::outputStateToString(struct OutputState* state, string tag)
{
	return string("\n") + centerEighty("Output State " + tag) +
		"\n\tMode: " 				+ modeToString(state->mode) +
		"\n\tFan Delay End: " 		+ to_string(state->fanDelayEnd) +
		"\n\tFan Output Status: "	+ boolToOnOff(state->fan) +
		"\n\tMQTT Dirty?: " 		+ boolToString(state->mqttOutputDirty);
}
