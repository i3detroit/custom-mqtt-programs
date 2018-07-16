#include "doControl.h"

uint8_t change_bit(uint8_t val, uint8_t num, bool bitval) {
  return (val & ~(1<<num)) | (bitval << num);
}

uint8_t setHeat(uint8_t toWrite, struct OutputState outputState) {
  if(outputState.mode != HEAT) {
    outputState.mqttOutputDirty = true;
    DEBUG_PRINTLN("heat changed");
    outputState.fanDelayEnd = millis() + 30*1000;
    //We use if it's 0 to mean we're done waiting;
    if(outputState.fanDelayEnd == 0) {
      outputState.fanDelayEnd = 1;
    }
  }
  outputState.mode = HEAT;
  toWrite = change_bit(toWrite, 7-ENABLE_CONTROL, 1);
  toWrite = change_bit(toWrite, 7-SELECT_CONTROL, 0);
  return toWrite;
}

uint8_t setCool(uint8_t toWrite, struct OutputState outputState) {
  if(outputState.mode != COOL) {
    DEBUG_PRINTLN("cool changed");
    outputState.mqttOutputDirty = true;
    outputState.fanDelayEnd = millis() + 30*1000;
    //We use if it's 0 to mean we're done waiting;
    if(outputState.fanDelayEnd == 0) {
      outputState.fanDelayEnd = 1;
    }
  }
  outputState.mode = COOL;
  toWrite = change_bit(toWrite, 7-ENABLE_CONTROL, 1);
  toWrite = change_bit(toWrite, 7-SELECT_CONTROL, 1);
  return toWrite;
}
uint8_t setOff(uint8_t toWrite, struct OutputState outputState) {
  if(outputState.mode != OFF) {
    DEBUG_PRINTLN("off changed");
    outputState.mqttOutputDirty = true;
  }
  outputState.mode = OFF;
  return change_bit(toWrite, 7-ENABLE_CONTROL, 0);
}

uint8_t doControl(struct ControlState controlState, struct SensorState sensorState, struct OutputState outputState) {
  uint8_t toWrite = 0;

  //status lights
  toWrite = change_bit(toWrite, 7-LED_FAN_ON, controlState.fan);
  toWrite = change_bit(toWrite, 7-LED_FAN_AUTO, !controlState.fan);
  toWrite = change_bit(toWrite, 7-LED_HEAT, controlState.mode == HEAT);
  toWrite = change_bit(toWrite, 7-LED_COOL, controlState.mode == COOL);
  toWrite = change_bit(toWrite, 7-LED_OFF, controlState.mode == OFF);

  // heating logic
  /*
   * if heat
   *
   *    if(current + swing < target)
   *        below temp, heat
   *        start heat
   *    if(current - swing > target)
   *        above
   *        stop
   *    if(current > target-swing && current < target+swing && enabled)
   *        heating through swing
   *        heat
   *    if(current > target-swing && current < target+swing && !enabled)
   *        //cooling through swing
   *        stop
   *    else
   *        there is no else
   *
   * Fan table
   * fan state      fan last state      enabled     force fan   fandelay long enough
   * 0              0                   0           0           0
   * 0              0                   0           0           1
   * 1t             0                   0           1           0
   * 1t             0                   0           1           1
   * 0              0                   1           0           0
   * 1t             0                   1           0           1
   * 0              0                   1           1           0
   * 1t             0                   1           1           1
   * 0t             1                   0           0           0
   * 0t             1                   0           0           1
   * 1              1                   0           1           0
   * 1              1                   0           1           1
   * 1              1                   1           0           0
   * 1              1                   1           0           1
   * 1              1                   1           1           0
   * 1              1                   1           1           1
   * s/   \* \([01t]*\) *\([01]\) *\([01]\) *\([01]\) *\([01]\)/case 0b\2\3\4\5: \1/
   *
   *
   */
  if(sensorState.temp < MIN_TEMP) {
    DEBUG_PRINTLN("under min temp");
    toWrite = setHeat(toWrite, outputState);
  } else if(controlState.mode == HEAT) {
    if(sensorState.temp < controlState.target - controlState.swing) {
      toWrite = setHeat(toWrite, outputState);
      DEBUG_PRINTLN("heat on");
    } else  if(sensorState.temp >= controlState.target + controlState.swing) {
      toWrite = setOff(toWrite, outputState);
      DEBUG_PRINTLN("heat off");
    } else {
      DEBUG_PRINTLN("HEAT inside swing; continue");
      if(outputState.mode == HEAT) {
        toWrite = setHeat(toWrite, outputState);
      } else {
        toWrite = setOff(toWrite, outputState);
      }
    }
#ifndef HEAT_ONLY
  } else if(controlState.mode == COOL) {
    //DEBUG_PRINTLN("COOLING");
    //DEBUG_PRINTLN(sensorState.temp);
    //DEBUG_PRINTLN(controlState.target);

    //DEBUG_PRINTLN(sensorState.temp + controlState.swing <  controlState.target);
    //DEBUG_PRINTLN(abs(sensorState.temp-controlState.target) <= controlState.swing);
    //DEBUG_PRINTLN(outputState.mode != COOL);
    if(sensorState.temp >= controlState.target + controlState.swing) {
      toWrite = setCool(toWrite, outputState);
      DEBUG_PRINTLN("cool on");
    } else if(sensorState.temp < controlState.target - controlState.swing ) {
      DEBUG_PRINTLN("cool off");
      toWrite = setOff(toWrite, outputState);
    } else {
      DEBUG_PRINTLN("COOL inside swing; continue");
      if(outputState.mode == COOL) {
        toWrite = setCool(toWrite, outputState);
      } else {
        toWrite = setOff(toWrite, outputState);
      }
    }
#endif
  } else if(controlState.mode == OFF) {
    DEBUG_PRINTLN("off");
    //off
    toWrite = setOff(toWrite, outputState);
  } else {
    DEBUG_PRINTLN("holy fuck mode is broken");
    //TODO: error to mqtt
  }

  //DEBUG_PRINTLN("FAN");
  //DEBUG_PRINTLN(outputState.fan);
  //DEBUG_PRINTLN(controlState.fan);
  //DEBUG_PRINTLN(outputState.mode != OFF);
  uint8_t fanControl = (outputState.fan == true << 3) | (outputState.mode != OFF << 2) | (controlState.fan << 1) | ((int32_t)(millis() - outputState.fanDelayEnd) >= 0);
  //fan last state      enabled     force fan   fandelay == 0
  switch(fanControl) {
    case 0b1000:
    case 0b1001:
      //write 0, transition
      DEBUG_PRINTLN("Fan transition off");
      outputState.mqttOutputDirty = true;
    case 0b0000:
    case 0b0001:
    case 0b0100:
    case 0b0110:
      //write 0
      outputState.fan = 0;
      break;
    case 0b0101:
    case 0b0010:
    case 0b0011:
    case 0b0111:
      //write 1; transition
      DEBUG_PRINTLN("Fan transition on");
      outputState.mqttOutputDirty = true;
    case 0b1010:
    case 0b1011:
    case 0b1100:
    case 0b1101:
    case 0b1110:
    case 0b1111:
      //write 1
      outputState.fan = 1;
      break;
    default:
      //fuck?
      break;
  }
  toWrite = change_bit(toWrite, 7-FAN_CONTROL, outputState.fan);

  // //write control
  //Serial.println("control update");
  //Serial.println(fanForced);
  //Serial.println(heat);
  //Serial.println(cool);
  //Serial.println(toWrite, BIN);

  /*
   * EN  SEL | H  C
   * 0   0   | 0  1
   * 0   1   | 1  0
   * 1   0   | 0  0
   * 1   1   | 0  0
   */

  //writeLow(toWrite);
  //Serial.println(writeLow(toWrite), BIN);
  return toWrite;
}
