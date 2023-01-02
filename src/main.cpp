#include <Arduino.h>
#include "bluetooth.h"

#define BTS_NAME		            	"PJCAN"
#define BLUETOOTH_SERVICE_UUID          "cc9e7b30-9834-488f-b762-aa62f5022dd4"
#define BLUETOOTH_CHARACTERISTIC_UUID   "cc9e7b31-9834-488f-b762-aa62f5022dd4"

void event_connect(uint8_t id, const uint8_t* data, size_t size) {
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    bluetooth.begin(BTS_NAME, BLUETOOTH_SERVICE_UUID, BLUETOOTH_CHARACTERISTIC_UUID, event_connect);
}

void loop() {
//    const uint8_t data[2] = { 0x00, 0x00 };
//    bluetooth.send(0x03, data, 2);
}