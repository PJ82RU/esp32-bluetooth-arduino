#include "bluetooth_callbacks.h"
#include "esp32-hal-log.h"

using namespace hardware;

void bluetooth_server_callbacks::onConnect(BLEServer *pServer) {
    device_connected++;
    log_i("Device connected");
}

void bluetooth_server_callbacks::onDisconnect(BLEServer *pServer) {
    device_connected--;
    log_i("Device disconnected");

    if (device_connected == 0 && pServer) {
        // время на подготовку стека bluetooth
        delay(500);
        pServer->startAdvertising();
    }
}

void bluetooth_characteristic_callbacks::onWrite(BLECharacteristic *pCharacteristic) {
    if (!pCharacteristic) {
        log_w("Characteristic not found");
        return;
    }
    if (!p_event_receive) {
        log_w("Event receive not found");
        return;
    }

    std::string value = pCharacteristic->getValue();
    size_t size = value.length();
    if (size == 0 || size > BLUETOOTH_WRITE_SIZE) {
        log_w("Receive data size is outside");
        return;
    }

    p_event_receive(value.c_str(), size);
}
