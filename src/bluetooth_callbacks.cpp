#include "bluetooth_callbacks.h"
#include "esp32-hal-log.h"

using namespace hardware;

void bluetooth_server_callbacks::onConnect(BLEServer *pServer) {
    device_connected++;
    log_i("Device connected");

    if (p_event_connect) p_event_connect();
}

void bluetooth_server_callbacks::onDisconnect(BLEServer *pServer) {
    device_connected--;
    // время на подготовку стека bluetooth
    delay(500);
    log_i("Device disconnected");

    if (pServer) pServer->startAdvertising();
    if (p_event_disconnect) p_event_disconnect();
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

    net_frame_t frame{};
    memcpy(frame.bytes, value.c_str(), size);
    size--;
    log_i("Receive data: id: %d, size: %zu, data: %s", frame.value.id, size, frame.value.data);

    p_event_receive(frame.value.id, frame.value.data, size);
}
