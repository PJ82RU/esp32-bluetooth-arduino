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
    if (!buffer) {
        log_w("The clipboard is missing");
        return;
    }

    std::string value = pCharacteristic->getValue();
    buffer->size = value.length();
    if (buffer->size == 0) {
        log_w("Receive data size is outside");
        return;
    }
    if (buffer->size > BLUETOOTH_WRITE_SIZE) {
        buffer->size = BLUETOOTH_WRITE_SIZE;
    }

    memcpy(buffer->frame.bytes, value.c_str(), buffer->size);
    buffer->is_data = true;

    log_d("Receive data: id: %d, size: %zu", buffer->frame.value.id, buffer->size - 1);
}

