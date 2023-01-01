#include "bluetooth_callbacks.h"
using namespace hardware;

void bluetooth_server_callbacks::onConnect(BLEServer* pServer) {
    device_connected++;
    ESP_LOGI(TAG, "Device connected");
}

void bluetooth_server_callbacks::onDisconnect(BLEServer* pServer) {
    device_connected--;
    ESP_LOGI(TAG, "Device disconnected");
}

void bluetooth_characteristic_callbacks::onWrite(BLECharacteristic *pCharacteristic) {
    if (!event_receive) {
        ESP_LOGW(TAG, "Event receive is missing");
        return;
    }

    std::string value = pCharacteristic->getValue();
    size_t size = value.length();
    if (size == 0 || size > BLUETOOTH_WRITE_SIZE) {
        ESP_LOGW(TAG, "Receive data size is outside");
        return;
    }

    // записываем полученные данные
    net_frame_t buffer;
    memcpy(buffer.bytes, value.c_str(), size);
    size--;

    ESP_LOGI(TAG, "Receive data: id: %02x,  size:%zu", buffer.value.id, size);
    ESP_LOG_BUFFER_HEXDUMP(TAG, buffer.bytes, size, ESP_LOG_DEBUG);

    event_receive(buffer.value.id, buffer.value.data, size);
}
