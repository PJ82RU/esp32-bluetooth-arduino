#include <Arduino.h>
#include "bluetooth_low_energy.h"

#define BTS_NAME                        "PJCAN"
#define BLUETOOTH_SERVICE_UUID          "cc9e7b30-9834-488f-b762-aa62f5022dd4"
#define BLUETOOTH_CHARACTERISTIC_UUID   "cc9e7b31-9834-488f-b762-aa62f5022dd4"

hardware::BluetoothLowEnergy ble;

size_t on_bluetooth_receive(uint8_t id, uint8_t *bytes, size_t size) {
    bytes[0] = 1;
    bytes[1] = 2;
    bytes[2] = 3;
    bytes[3] = 4;
    return size + 4;
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    ble.cb_receive = on_bluetooth_receive;
    ble.start(BTS_NAME, BLUETOOTH_SERVICE_UUID, BLUETOOTH_CHARACTERISTIC_UUID);
}

void loop() {
    ble.handle();
//    if (millis() > 15000) ble.stop();
}