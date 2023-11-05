#include <Arduino.h>
#include "bluetooth_low_energy.h"

#define BTS_NAME                        "PJCAN"
#define BLUETOOTH_SERVICE_UUID          "cc9e7b30-9834-488f-b762-aa62f5022dd4"
#define BLUETOOTH_CHARACTERISTIC_UUID   "cc9e7b31-9834-488f-b762-aa62f5022dd4"

hardware::BluetoothLowEnergy ble;

size_t on_bluetooth_receive(uint8_t id, uint8_t *data, size_t size) {
    Serial.printf("In id = 0x%02x, size = %zu\n", id, size);
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    return size + 4;
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    ble.cb_receive = on_bluetooth_receive;
    ble.begin(BTS_NAME, BLUETOOTH_SERVICE_UUID, BLUETOOTH_CHARACTERISTIC_UUID);
}

void loop() {
}