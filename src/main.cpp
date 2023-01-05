#include <Arduino.h>
#include "bluetooth.h"

#define BTS_NAME                        "PJCAN"
#define BLUETOOTH_SERVICE_UUID          "cc9e7b30-9834-488f-b762-aa62f5022dd4"
#define BLUETOOTH_CHARACTERISTIC_UUID   "cc9e7b31-9834-488f-b762-aa62f5022dd4"

void setup() {
    Serial.begin(115200);
    delay(1000);

    bluetooth.begin(BTS_NAME, BLUETOOTH_SERVICE_UUID, BLUETOOTH_CHARACTERISTIC_UUID);
}

void loop() {
    // эхо
    if (bluetooth.is_receive()) {
        uint8_t id = 0;
        uint8_t data[BLUETOOTH_WRITE_SIZE] = {};
        size_t size = 0;
        bluetooth.receive(id, data, size);
        bluetooth.send(id, data, size);
    }
}