#include <Arduino.h>
#include "bluetooth.h"

#define BTS_NAME                        "PJCAN"
#define BLUETOOTH_SERVICE_UUID          "cc9e7b30-9834-488f-b762-aa62f5022dd4"
#define BLUETOOTH_CHARACTERISTIC_UUID   "cc9e7b31-9834-488f-b762-aa62f5022dd4"

hardware::bluetooth_c bluetooth;

size_t bluetooth_receive_t(uint8_t id, uint8_t *bytes, size_t size) {
    bytes[0] = 1;
    bytes[1] = 2;
    bytes[2] = 3;
    bytes[3] = 4;
    return size + 4;
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    bluetooth.event_receive = bluetooth_receive_t;
    bluetooth.begin(BTS_NAME, BLUETOOTH_SERVICE_UUID, BLUETOOTH_CHARACTERISTIC_UUID);
}

void loop() {
    bluetooth.handle();
}