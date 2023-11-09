#include <Arduino.h>
#include "bluetooth_low_energy.h"

#define BTS_NAME                        "PJCAN"
#define BLUETOOTH_SERVICE_UUID          "cc9e7b30-9834-488f-b762-aa62f5022dd4"
#define BLUETOOTH_CHARACTERISTIC_UUID   "cc9e7b31-9834-488f-b762-aa62f5022dd4"

hardware::BluetoothLowEnergy ble;

bool on_bluetooth_receive(void *p_value, void *p_parameters) {
    auto frame = (hardware::net_frame_t *) p_value;

    Serial.printf("In id = 0x%02x, size = %zu\n", frame->value.id, frame->value.size);
    frame->value.data[0] = 1;
    frame->value.data[1] = 2;
    frame->value.data[2] = 3;
    frame->value.size = 3;
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    ble.callback.init(1);
    ble.callback.set(on_bluetooth_receive);
    ble.begin(BTS_NAME, BLUETOOTH_SERVICE_UUID, BLUETOOTH_CHARACTERISTIC_UUID);
}

void loop() {
    Serial.println(ble.callback.task_stack_depth());
    delay(500);
}