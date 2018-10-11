#include "testDoControl.h"
#include <stdio.h>
#include <stdint.h>
#include <string>
#include "../doControl.h"
#include "fixtures/MillisFixture.h"
#include "fixtures/StateInitializers.h"
#include "fixtures/PrintFixtures.h"
#include "fixtures/AssertionFixtures.h"
#include "../thermostat.h"
#include "../pins.h"
#include "googleTest/googletest/include/gtest/gtest.h"

/*
 * =============================================================================
 * Verify that the fan can be turned on and off.
 * =============================================================================
 */


/*
 * =============================================================================
 * Verify the fan table for the cases where the fan starts from off.
 * =============================================================================
 */


TEST(FanOutputState, 0000OFFtoOFF)
{
//	//                fan last state,     enabled,    force fan,  fandelay long enough
//	//0               0,                  0,          0,          0
	bool currentFanOn = 					false;
	bool fanToSet = 						false;
	bool expectedFanState = 				false;
	enum Mode currentMode = 				Mode::OFF;
	enum Mode modeToSet = 					Mode::OFF;
	int currentTemp = 						24;
	int tempToSet = 						26;

	SECRETMILLIS =							3000;
	float currentFanDelayEnd = 				60000;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = string("After ") +
			"\n\t - not changing the mode (which started OFF)" +
			"\n\t - leaving the fan unforced (starting from off)" +
			"\n\t - and with an insufficiently long delay"
			"\n\t --- the fan should not have been on.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
};
//
/*
 * Test the fan table.
 */
TEST(FanOutputState, 0001OFFtoOFF)
{
	//                fan last state,     enabled,    force fan,  fandelay long enough
	//0               0,                  0,          0,          1
	bool currentFanOn = 					false;
	bool fanToSet = 						false;
	bool expectedFanState = 				false;
	enum Mode currentMode = 				Mode::OFF;
	enum Mode modeToSet = 					Mode::OFF;
	int currentTemp = 						24;
	int tempToSet = 						26;

	SECRETMILLIS =							3000;
	float currentFanDelayEnd = 				0;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = string("After ") +
			"\n\t - not changing the mode (which started OFF)" +
			"\n\t - leaving the fan unforced (starting from on)" +
			"\n\t - and the delay long enough"
			"\n\t --- the fan should not have been on.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
	//AssertionFixtures::assertModeLightMatchesMode(controls, sensors, status, lights);
	//AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(controls, sensors, status, lights);
};

//TODO: Ask Mark about this one.
TEST(FanOutputState, 0010OFFtoHEAT)
{
	//                fan last state,     enabled,    force fan,  fandelay long enough
	//1t              0,                  0,          1,          0
  	//From Mark's own words, if millis() is greater than fandelayend after running doControl, the delay is long enough.  If it's LTE, this is false.
	bool currentFanOn = 					false;
	bool fanToSet = 						true;
	bool expectedFanState = 				true;
	enum Mode currentMode = 				Mode::OFF;
	enum Mode modeToSet = 					Mode::OFF;
	int currentTemp = 						24;
	int tempToSet = 						26;

	SECRETMILLIS =							3000;
	float currentFanDelayEnd = 				60000;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = string("After ") +
			"\n\t - not changing the mode (which started OFF)" +
			"\n\t - forcing the fan on (starting from the fan off)" +
			"\n\t - and the delay not long enough"
			"\n\t --- the fan should still transition on.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
	//AssertionFixtures::assertModeLightMatchesMode(controls, sensors, status, lights);
	//AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(controls, sensors, status, lights);
};

TEST(FanOutputState, 0011OFFtoOFF)
{
//	//                fan last state,     enabled,    force fan,  fandelay long enough
//	//1t              0,                  0,          1,          1
	bool currentFanOn = false;
	bool fanToSet = true;
	bool expectedFanState = true;
	enum Mode currentMode = Mode::OFF;
	enum Mode modeToSet = Mode::OFF;
	int currentTemp = 24;
	int tempToSet = 26;

	SECRETMILLIS=0;
	float currentFanDelayEnd = 0;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = string("After ") +
			"\n\t - starting from the fan off and the mode OFF" +
			"\n\t - forcing the fan with the mode remaining OFF" +
			"\n\t - and the delay long enough" +
			"\n\t --- the fan should have transitioned on.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
	//AssertionFixtures::assertModeLightMatchesMode(controls, sensors, status, lights);
	//AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(controls, sensors, status, lights);
};

TEST(FanOutputState, 0100OFFtoHEAT)
{
	//                fan last state,     enabled,    force fan,  fandelay long enough
	//0               0,                  1,          0,          0
	bool currentFanOn = false;
	bool fanToSet = false;
	bool expectedFanState = false;
	enum Mode currentMode = Mode::OFF;
	enum Mode modeToSet = Mode::HEAT;
	int currentTemp = 24;
	int tempToSet = 26;

	SECRETMILLIS=0;
	float currentFanDelayEnd = 0;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = string("After ") +
			"\n\t - turning on the heat" +
			"\n\t - starting from the fan off" +
			"\n\t - with the fan not forced" +
			"\n\t - and the delay not long enough" +
			"\n\t --- the fan should have been off.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
	//AssertionFixtures::assertModeLightMatchesMode(controls, sensors, status, lights);
	//AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(controls, sensors, status, lights);
};


//TODO Uncomment and correct pin checks.
TEST(FanOutputState, 0101HEATtoHEAT)
{
//	//                fan last state,     enabled,    force fan,  fandelay long enough
//	//1               0,                  1,          0,          1
	bool currentFanOn =             false;
	bool fanToSet =                 true;
	bool expectedFanState =         true;
	//If we go from OFF to HEAT or COLD to HEAT, the fan delay end will be set in the future and the fan delay won't be long enough.
	enum Mode currentMode =         Mode::HEAT;
	enum Mode modeToSet =           Mode::HEAT;
	int currentTemp =               24;
	int tempToSet =                 26;

	//From Mark's own words, if millis() is greater than fandelayend, this is true.  If it's LTE, this is false.
	SECRETMILLIS =                  6000;
	float currentFanDelayEnd =      0;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = string("     After ") +
			"\n\t - turning on the heat from HEAT" +
			"\n\t - with the fan NOT forced, starting from OutputStatus.fan = false" +
			"\n\t - and with the fandelay being long enough," +
			"\n\t --- the fan should have (but did not) turn on.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
	//AssertionFixtures::assertModeLightMatchesMode(controls, sensors, status, lights);
	//AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(controls, sensors, status, lights);
};

//
TEST(FanOutputState, 0110OFFtoHEAT)
{
//	//                fan last state,     enabled,    force fan,  fandelay long enough
//	//0               0,                  1,          1,          0
	bool currentFanOn = false;
	bool fanToSet = true;
	bool expectedFanState = false;
	enum Mode currentMode = Mode::OFF;
	enum Mode modeToSet = Mode::HEAT;
	int currentTemp = 24;
	int tempToSet = 26;

	SECRETMILLIS=0;
	float currentFanDelayEnd = 0;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = "After \n\t - turning on the heat\n\t - starting from the fan off\n\t - with the fan forced\n\t - and the delay not long enough\n\t --- the fan should be on.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
	//AssertionFixtures::assertModeLightMatchesMode(controls, sensors, status, lights);
	//AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(controls, sensors, status, lights);
};


TEST(FanOutputState, 0111HEATtoHEAT)
{
	//                fan last state,     enabled,    force fan,  fandelay long enough
	//1               0,                  1,          1,          1
  	//From Mark's own words, if millis() is greater than fandelayend after running doControl, the delay is long enough.  If it's LTE, this is false.
	bool currentFanOn = 					false;
	bool fanToSet = 						true;
	bool expectedFanState = 				true;
	enum Mode currentMode = 				Mode::HEAT;
	enum Mode modeToSet = 					Mode::HEAT;
	int currentTemp = 						24;
	int tempToSet = 						26;

	SECRETMILLIS =							0;
	float currentFanDelayEnd = 				0;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = string("After ") +
			"\n\t - turning on the heat" +
			"\n\t - starting from the fan off" +
			"\n\t - with the fan forced" +
			"\n\t - and the delay long enough" +
			"\n\t --- the fan should be on.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
	//AssertionFixtures::assertModeLightMatchesMode(controls, sensors, status, lights);
	//AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(controls, sensors, status, lights);
};

/*
 * ===============================================================================
 * Fan table tests where the fan starts on.
 * ===============================================================================
 * Fan table
 * fan state      fan last state      enabled     force fan   fandelay long enough
 * 0t             1                   0           0           0
 * 0t             1                   0           0           1
 * 1              1                   0           1           0
 * 1              1                   0           1           1
 * 1              1                   1           0           0
 * 1              1                   1           0           1
 * 1              1                   1           1           0
 * 1              1                   1           1           1
 * ===============================================================================
 */

TEST(FanOutputState, 1000HEATtoOFF)
{
	//               fan last state,     enabled,    force fan,  fandelay long enough
	//0t             1                   0           0           0
  	//From Mark's own words, if millis() is greater than fandelayend after running doControl, the delay is long enough.  If it's LTE, this is false.
	bool currentFanOn = 					true;
	bool fanToSet = 						false;
	bool expectedFanState = 				false;
	enum Mode currentMode = 				Mode::HEAT;
	enum Mode modeToSet = 					Mode::OFF;
	int currentTemp = 						24;
	int tempToSet = 						26;

	SECRETMILLIS =							3;
	float currentFanDelayEnd = 				18000;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = string("After ") +
			"\n\t - turning on the heat" +
			"\n\t - starting from the fan on" +
			"\n\t - with the fan unforced" +
			"\n\t - and the delay not long enough" +
			"\n\t --- the fan should be off.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
	//AssertionFixtures::assertModeLightMatchesMode(controls, sensors, status, lights);
	//AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(controls, sensors, status, lights);
};

TEST(FanOutputState, 1001OFFtoHEAT)
{
	//               fan last state,     enabled,    force fan,  fandelay long enough
	//0t             1                   0           0           1
  	//From Mark's own words, if millis() is greater than fandelayend after running doControl, the delay is long enough.  If it's LTE, this is false.
	bool currentFanOn = 					true;
	bool fanToSet = 						false;
	bool expectedFanState = 				false;
	enum Mode currentMode = 				Mode::HEAT;
	enum Mode modeToSet = 					Mode::OFF;
	int currentTemp = 						24;
	int tempToSet = 						26;

	SECRETMILLIS =							0;
	float currentFanDelayEnd = 				0;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = string("After ") +
			"\n\t - turning off the heat" +
			"\n\t - starting from the fan on" +
			"\n\t - with the fan unforced" +
			"\n\t - and the delay long enough" +
			"\n\t --- the fan should be off.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
	//AssertionFixtures::assertModeLightMatchesMode(controls, sensors, status, lights);
	//AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(controls, sensors, status, lights);
};

TEST(FanOutputState, 1010HEATtoOFF)
{
	//               fan last state,     enabled,    force fan,  fandelay long enough
	//1              1                   0           1           0
  	//From Mark's own words, if millis() is greater than fandelayend after running doControl, the delay is long enough.  If it's LTE, this is false.
	bool currentFanOn = 					true;
	bool fanToSet = 						true;
	bool expectedFanState = 				true;
	enum Mode currentMode = 				Mode::HEAT;
	enum Mode modeToSet = 					Mode::OFF;
	int currentTemp = 						24;
	int tempToSet = 						26;

	SECRETMILLIS =							3;
	float currentFanDelayEnd = 				29000;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = string("After ") +
			"\n\t - turning off the heat" +
			"\n\t - starting from the fan on" +
			"\n\t - with the fan forced on" +
			"\n\t - and the delay not long enough" +
			"\n\t --- the fan should be on.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
	//AssertionFixtures::assertModeLightMatchesMode(controls, sensors, status, lights);
	//AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(controls, sensors, status, lights);
};

TEST(FanOutputState, 1011HEATtoOFF)
{
	//               fan last state,     enabled,    force fan,  fandelay long enough
	//1              1                   0           1           1
  	//From Mark's own words, if millis() is greater than fandelayend after running doControl, the delay is long enough.  If it's LTE, this is false.
	bool currentFanOn = 					true;
	bool fanToSet = 						true;
	bool expectedFanState = 				true;
	enum Mode currentMode = 				Mode::HEAT;
	enum Mode modeToSet = 					Mode::OFF;
	int currentTemp = 						24;
	int tempToSet = 						26;

	SECRETMILLIS =							0;
	float currentFanDelayEnd = 				0;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = string("After ") +
			"\n\t - turning off the heat" +
			"\n\t - starting from the fan on" +
			"\n\t - with the fan forced on" +
			"\n\t - and the delay long enough" +
			"\n\t --- the fan should be on.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
	//AssertionFixtures::assertModeLightMatchesMode(controls, sensors, status, lights);
	//AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(controls, sensors, status, lights);
};


TEST(FanOutputState, 1100HEATtoOFF)
{
	//               fan last state,     enabled,    force fan,  fandelay long enough
	//1              1                   1           0           0
  	//From Mark's own words, if millis() is greater than fandelayend after running doControl, the delay is long enough.  If it's LTE, this is false.
	bool currentFanOn = 					true;
	bool fanToSet = 						false;
	bool expectedFanState = 				true;
	enum Mode currentMode = 				Mode::OFF;
	enum Mode modeToSet = 					Mode::HEAT;
	int currentTemp = 						24;
	int tempToSet = 						26;

	SECRETMILLIS =							0;
	float currentFanDelayEnd = 				0;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = string("After ") +
			"\n\t - turning on the heat" +
			"\n\t - starting from the fan on" +
			"\n\t - with the fan not forced on" +
			"\n\t - and the delay not long enough" +
			"\n\t --- the fan should be on.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
	//AssertionFixtures::assertModeLightMatchesMode(controls, sensors, status, lights);
	//AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(controls, sensors, status, lights);
};

TEST(FanOutputState, 1101OFFtoHEAT)
{
	//               fan last state,     enabled,    force fan,  fandelay long enough
	//1              1                   1           0           1
  	//From Mark's own words, if millis() is greater than fandelayend after running doControl, the delay is long enough.  If it's LTE, this is false.
	bool currentFanOn = 					true;
	bool fanToSet = 						false;
	bool expectedFanState = 				true;
	enum Mode currentMode = 				Mode::HEAT;
	enum Mode modeToSet = 					Mode::HEAT;
	int currentTemp = 						24;
	int tempToSet = 						26;

	SECRETMILLIS =							0;
	float currentFanDelayEnd = 				0;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = string("After ") +
			"\n\t - leaving the mode set at HEAT" +
			"\n\t - starting from the fan on" +
			"\n\t - with the fan not forced on" +
			"\n\t - and the delay long enough" +
			"\n\t --- the fan should be on.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
	//AssertionFixtures::assertModeLightMatchesMode(controls, sensors, status, lights);
	//AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(controls, sensors, status, lights);
};

TEST(FanOutputState, 1110OFFtoHEAT)
{
	//               fan last state,     enabled,    force fan,  fandelay long enough
	//1              1                   1           1           0
  	//From Mark's own words, if millis() is greater than fandelayend after running doControl, the delay is long enough.  If it's LTE, this is false.
	bool currentFanOn = 					true;
	bool fanToSet = 						true;
	bool expectedFanState = 				true;
	enum Mode currentMode = 				Mode::OFF;
	enum Mode modeToSet = 					Mode::HEAT;
	int currentTemp = 						24;
	int tempToSet = 						26;

	SECRETMILLIS =							0;
	float currentFanDelayEnd = 				0;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = string("After ") +
			"\n\t - turning the heat on" +
			"\n\t - starting from the fan on" +
			"\n\t - with the fan forced on" +
			"\n\t - and the delay not long enough" +
			"\n\t --- the fan should be on.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
	//AssertionFixtures::assertModeLightMatchesMode(controls, sensors, status, lights);
	//AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(controls, sensors, status, lights);
};

TEST(FanOutputState, 1111HEATtoHEAT)
{
	//               fan last state,     enabled,    force fan,  fandelay long enough
	//1              1                   1           1           1
  	//From Mark's own words, if millis() is greater than fandelayend after running doControl, the delay is long enough.  If it's LTE, this is false.
	bool currentFanOn = 					true;
	bool fanToSet = 						true;
	bool expectedFanState = 				true;
	enum Mode currentMode = 				Mode::HEAT;
	enum Mode modeToSet = 					Mode::HEAT;
	int currentTemp = 						24;
	int tempToSet = 						26;

	SECRETMILLIS =							0;
	float currentFanDelayEnd = 				0;

	struct SensorState sensors =			StateInitializers::initializeSensorState(currentTemp, 0, 0, true, true); //temp, pressure, humidity, battery, temp to set
	struct ControlState controls =			StateInitializers::initializeControlState(modeToSet, fanToSet, tempToSet, 1, 0); //Need to try timeout cases with temp below current and timeout too short and sufficient.  Need to try both heating and cooling.
	struct OutputState status = 			StateInitializers::initializeOutputState(currentMode, currentFanDelayEnd, currentFanOn, false);
	struct SensorState sensorStateBefore = 	SensorState(sensors);
	struct ControlState controlStateBefore = ControlState(controls);
	struct OutputState outputStateBefore = 	OutputState(status);

	uint8_t lights = 						doControl(&controls, &sensors, &status);

	string testMessage = string("After ") +
			"\n\t - leaving the heat turned on" +
			"\n\t - starting from the fan on" +
			"\n\t - with the fan forced on" +
			"\n\t - and the delay long enough" +
			"\n\t --- the fan should be on.";
	string fanVariables = PrintFixtures::buildFanVariablesStatusMessage(
			currentFanOn,
			(status.mode != Mode::OFF),
			status.fan,
			status.fanDelayEnd);
	string fullAssertionMessage = PrintFixtures::buildFanDebuggingMessage(
			&controlStateBefore,
			&sensorStateBefore,
			&outputStateBefore,
			testMessage,
			fanVariables,
			&status,
			lights);
	ASSERT_EQ(status.fan, expectedFanState) << fullAssertionMessage;
	//AssertionFixtures::assertModeLightMatchesMode(controls, sensors, status, lights);
	//AssertionFixtures::assertFanLightsAsExpectedWhenChangingFromOFFtoHEATorCOOL(controls, sensors, status, lights);
};

int main(int argc, char** argv) {
  // The following line must be executed to initialize Google Test
  // ( before running the tests.
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
};
