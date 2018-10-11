#ifndef THERMOSTAT_TEST_FIXTURES_ASSERTIONFIXTURES_H_
#define THERMOSTAT_TEST_FIXTURES_ASSERTIONFIXTURES_H_

#include <string.h>
#include <string>
#include <stdint.h>
#include "../../thermostat.h"

class AssertionFixtures {
	public:
		static const std::string horLine;
		static bool readBit(uint8_t byteToReadFrom,
				int bitPositionToRead);
		static void assertModeLightMatchesMode(
				struct ControlState* controlStateBefore,
				struct OutputState* outputStateBefore,
				struct ControlState* controlStateAfter,
				struct SensorState* sensorStateAfter,
				struct OutputState* outputStateAfter,
				uint8_t bitsToVerify);
		//TODO Change "Lights" to "Pins"
		static void assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(
				struct ControlState* controlStateBefore,
				struct OutputState* outputStateBefore,
				struct ControlState* controlStateAfter,
				struct SensorState* sensorStateAfter,
				struct OutputState* outputStateAfter,
				uint8_t bitsToVerify);
		//TODO Change "Lights" to "Pins"
		static void assertFanLightsAsExpectedWhenModeWasInvalidAndPriorStateWasAutoAndOff(
				struct ControlState* controlStateBefore,
				struct OutputState* outputStateBefore,
				struct ControlState* controlStateAfter,
				struct SensorState* sensorStateAfter,
				struct OutputState* outputStateAfter,
				uint8_t bitsToVerify);
};

#endif /* THERMOSTAT_TEST_FIXTURES_ASSERTIONFIXTURES_H_ */
