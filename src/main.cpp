#include <Arduino.h>
#include "bluetooth.h"

#define BTS_NAME		            	"PJCAN"
#define BLUETOOTH_SERVICE_UUID          "cc9e7b30-9834-488f-b762-aa62f5022dd4"
#define BLUETOOTH_CHARACTERISTIC_UUID   "cc9e7b31-9834-488f-b762-aa62f5022dd4"

void setup() {
    hardware::bluetooth::begin(BTS_NAME, BLUETOOTH_SERVICE_UUID, BLUETOOTH_CHARACTERISTIC_UUID);
}

void loop() {
    hardware::bluetooth::handle();
}