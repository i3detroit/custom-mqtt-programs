#ifndef PRINTFIXTURES_H
#define PRINTFIXTURES_H

	#include <string>
	#include "../../thermostat.h"

	using namespace std;

	class PrintFixtures
	{
		public:
			static const string eightySeperator;
			static const string debugSeperator;
			static const string newline;
			static string centerEighty(string toCenter);
			static string modeToString(enum Mode modeToConvert);
			static string boolToOnOff(bool b);
			static string boolToString(bool b);
			static string buildFanVariablesStatusMessage(
					bool fanLastState,
					bool enabled,
					bool forceFan,
					uint8_t fanDelayEnd);
			static string buildPinDebuggingMessage(
					struct ControlState* controlStateBefore,
					struct OutputState* outputStateBefore,
					string message,
					struct ControlState* controlStateAfter,
					struct SensorState* sensorStateAfter,
					struct OutputState* outputStateAfter,
					uint8_t pins);
			static string buildFanDebuggingMessage(
					struct ControlState* controlStateAfter,
					struct SensorState* sensorStateAfter,
					struct OutputState* outputStateAfter,
					string expectedState,
					string calculatedVariables,
					struct OutputState* outputStateToPrint,
					uint8_t pins);
			static string pinsToString(uint8_t pins, string tag);
			static string controlStateToString(struct ControlState* state, string tag);
			static string sensorStateToString(struct SensorState* state, string tag);
			static string outputStateToString(struct OutputState* state, string tag);
	};
#endif
