#include "../googleTest/googletest/include/gtest/gtest.h"

#include "AssertionFixtures.h"
#include "MillisFixture.h"
#include "PrintFixtures.h"

using namespace std;

const string AssertionFixtures::horLine = PrintFixtures::eightySeperator + PrintFixtures::newline;

// These positions read from the right--that is, position 0 is the lowest bit and 7 is the highest.
bool AssertionFixtures::readBit(uint8_t byteToReadFrom,
		int bitPositionToRead)
{
	switch (bitPositionToRead)
	{
		case 7:
			return ((int) byteToReadFrom & 128) > 0;
		case 6:
			return ((int) byteToReadFrom &  64) > 0;
		case 5:
			return ((int) byteToReadFrom &  32) > 0;
		case 4:
			return ((int) byteToReadFrom &  16) > 0;
		case 3:
			return ((int) byteToReadFrom &   8) > 0;
		case 2:
			return ((int) byteToReadFrom &   4) > 0;
		case 1:
			return ((int) byteToReadFrom &   2) > 0;
		default:
			return ((int) byteToReadFrom &   1) > 0;
	}
};

void AssertionFixtures::assertModeLightMatchesMode(
		struct ControlState* controlStateBefore,
		struct OutputState* outputStateBefore,
		struct ControlState* controlStateAfter,
		struct SensorState* sensorStateAfter,
		struct OutputState* outputStateAfter,
		uint8_t bitsToVerify)
{
	if (outputStateAfter->mode == Mode::COOL)
	{

		ASSERT_EQ(readBit(bitsToVerify, 7-LED_COOL),  true) <<
				PrintFixtures::buildPinDebuggingMessage(
						controlStateBefore,
						outputStateBefore,
						(horLine +
								"With OutputState::Mode set to COOL after doControl finished" +
								"\n, the COOL led control bit should have been high." +
								PrintFixtures::newline + horLine),
						controlStateAfter,
						sensorStateAfter,
						outputStateAfter,
						bitsToVerify);
		ASSERT_EQ(readBit(bitsToVerify, 7-LED_OFF),  false) <<
				PrintFixtures::buildPinDebuggingMessage(
						controlStateBefore,
						outputStateBefore,
						(horLine +
								"With OutputState::Mode set to COOL after doControl finished" +
								"\n, the OFF led control bit should have been low."
								+ PrintFixtures::newline + horLine),
						controlStateAfter,
						sensorStateAfter,
						outputStateAfter,
						bitsToVerify);
		ASSERT_EQ(readBit(bitsToVerify, 7-LED_HEAT), false) <<
				PrintFixtures::buildPinDebuggingMessage(
						controlStateBefore,
						outputStateBefore,
						(horLine +
								"With OutputState::Mode set to COOL after doControl finished" +
								"\n, the HEAT led control bit should have been low."
								+ PrintFixtures::newline + horLine),
						controlStateAfter,
						sensorStateAfter,
						outputStateAfter,
						bitsToVerify);
	}
	else if (outputStateAfter->mode == Mode::HEAT)
	{
		ASSERT_EQ(readBit(bitsToVerify, 7-LED_COOL),  false) <<
				PrintFixtures::buildPinDebuggingMessage(
						controlStateBefore,
						outputStateBefore,
						(horLine +
								"With OutputState::Mode set to HEAT after doControl finished" +
								"\n, the COOL led control bit should have been low." +
								PrintFixtures::newline + horLine),
						controlStateAfter,
						sensorStateAfter,
						outputStateAfter,
						bitsToVerify);
		ASSERT_EQ(readBit(bitsToVerify, 7-LED_OFF),  false) <<
				PrintFixtures::buildPinDebuggingMessage(
						controlStateBefore,
						outputStateBefore,
						(horLine +
								"With OutputState::Mode set to HEAT after doControl finished" +
								"\n, the OFF led control bit should have been low."
								+ PrintFixtures::newline + horLine),
						controlStateAfter,
						sensorStateAfter,
						outputStateAfter,
						bitsToVerify);
		ASSERT_EQ(readBit(bitsToVerify, 7-LED_HEAT), true) <<
				PrintFixtures::buildPinDebuggingMessage(
						controlStateBefore,
						outputStateBefore,
						(horLine +
								"With OutputState::Mode set to HEAT after doControl finished" +
								"\n, the HEAT led control bit should have been high."
								+ PrintFixtures::newline + horLine),
						controlStateAfter,
						sensorStateAfter,
						outputStateAfter,
						bitsToVerify);
	}
	else
	{
		ASSERT_EQ(readBit(bitsToVerify, 7-LED_COOL),  false) <<
				PrintFixtures::buildPinDebuggingMessage(
						controlStateBefore,
						outputStateBefore,
						(horLine +
								"With OutputState::Mode set to OFF after doControl finished" +
								"\n, the COOL led control bit should have been low." +
								PrintFixtures::newline + horLine),
						controlStateAfter,
						sensorStateAfter,
						outputStateAfter,
						bitsToVerify);
		ASSERT_EQ(readBit(bitsToVerify, 7-LED_OFF), true) <<
				PrintFixtures::buildPinDebuggingMessage(
						controlStateBefore,
						outputStateBefore,
						(horLine +
								"With OutputState::Mode set to OFF after doControl finished" +
								"\n, the OFF led control bit should have been high."
								+ PrintFixtures::newline + horLine),
						controlStateAfter,
						sensorStateAfter,
						outputStateAfter,
						bitsToVerify);
		ASSERT_EQ(readBit(bitsToVerify, 7-LED_HEAT), false) <<
				PrintFixtures::buildPinDebuggingMessage(
						controlStateBefore,
						outputStateBefore,
						(horLine +
								"With OutputState::Mode set to OFF after doControl finished" +
								"\n, the HEAT led control bit should have been low."
								+ PrintFixtures::newline + horLine),
						controlStateAfter,
						sensorStateAfter,
						outputStateAfter,
						bitsToVerify);
	}
};


//TODO Print detailed dump.
//TODO Add \n between assertion message and horLine.
//TODO Change all 'light' and 'bit' to 'pin'.
void AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(
		struct ControlState* controlStateBefore,
		struct OutputState* outputStateBefore,
		struct ControlState* controlStateAfter,
		struct SensorState* sensorStateAfter,
		struct OutputState* outputStateAfter,
		uint8_t bitsToVerify)
{
	ASSERT_EQ(readBit(bitsToVerify, 7-LED_FAN_AUTO), true) <<
			PrintFixtures::buildPinDebuggingMessage(
					controlStateBefore,
					outputStateBefore,
					(horLine +
							"Since the OutputState::Mode went from OFF to HEAT or COOL," +
							"\n, the fan AUTO pin should have been high."
							+ PrintFixtures::newline + horLine),
					controlStateAfter,
					sensorStateAfter,
					outputStateAfter,
					bitsToVerify);
	ASSERT_EQ(readBit(bitsToVerify, 7-LED_FAN_ON),  false) << horLine + "Since the OutputState::Mode went from OFF to HEAT or COOL, the fan manual light control bit should have been low." + PrintFixtures::newline + horLine;
	ASSERT_EQ(readBit(bitsToVerify, 7-FAN_CONTROL), false) << horLine + "Since the OutputState::Mode went from OFF to HEAT or COOL, the fan's output bit should have been low.  (We changed from OFF to HEAT or COOL, so the fanDelayEnd was reset to the future, and millis() still has to catch up.)" + PrintFixtures::newline + horLine;
};

void AssertionFixtures::assertFanLightsAsExpectedWhenModeWasInvalidAndPriorStateWasAutoAndOff(
		struct ControlState* controlStateBefore,
		struct OutputState* outputStateBefore,
		struct ControlState* controlStateAfter,
		struct SensorState* sensorStateAfter,
		struct OutputState* outputStateAfter,
		uint8_t bitsToVerify)
{
	ASSERT_EQ(readBit(bitsToVerify, 7-LED_FAN_AUTO), true) << "Because the mode was invalid, the fan 'auto' control bit should have remained in its prior state, high." + PrintFixtures::newline + horLine;
	ASSERT_EQ(readBit(bitsToVerify, 7-LED_FAN_ON),  false) << "Because the mode was invalid, the fan manual light control bit should have remained in its prior state, low." + PrintFixtures::newline + horLine;
	ASSERT_EQ(readBit(bitsToVerify, 7-FAN_CONTROL), false) << "The fan's output bit should have remained low." + PrintFixtures::newline + horLine;
};

