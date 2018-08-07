#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "../thermostat.h"
#include "../doControl.h"

uint32_t m3illis() { return 42; }

// struct ControlState {
//   enum Mode mode;
//   bool fan;
//   int target;
//   int swing;
//   union TimeoutInterval timeout;
// };
// struct SensorState {
//   float temp;
//   float pressure;
//   float humidity;
//   bool batteryPower;
//   bool tempSensor; //if sensor connected
// };
// struct OutputState {
//   enum Mode mode;
//   uint32_t fanDelayEnd;
//   bool fan;
//   bool mqttOutputDirty;
// };
void print(struct ControlState controlState, struct SensorState sensorState, struct OutputState outputState) {
    printf("\tmode: %s\n\tfan: %d\n\ttarget: %d\n\ttimeout: %l\n",
            controlState.mode == OFF ? "off" : controlState.mode == HEAT ? "heat" : "cool",
            controlState.fan ? "on" : "off",
            controlState.target,
            controlState.timeout.timeout);

    printf("###Output State\n");
    printf("\tmode: %s\n\tfan: %d\n",
            outputState.mode == OFF ? "off" : outputState.mode == HEAT ? "heat" : "cool",
            outputState.fan ? "on" : "off");
}
int main() {
    struct ControlState controlState;
    struct SensorState sensorState;
    struct OutputState outputState;
    controlState.mode = OFF;
    controlState.fan = true;
    controlState.target = 10;
    controlState.swing = 1;
    controlState.timeout.timeout = 0;//no timeout

    sensorState.temp = 10;

    outputState.mode = OFF;
    outputState.fanDelayEnd=0;
    outputState.fan = OFF;
    outputState.mqttOutputDirty = false;
    uint8_t toWrite = doControl(controlState, sensorState, outputState);

    return 0;

}
