#include "bluetooth_callbacks.h"
using namespace hardware;

void bluetooth_server_callbacks::onConnect(BLEServer* pServer) {
    device_connected++;
    ESP_LOGI(TAG, "Device connected");
    if (p_event_connect) p_event_connect();
}

void bluetooth_server_callbacks::onDisconnect(BLEServer* pServer) {
    device_connected--;
    ESP_LOGI(TAG, "Device disconnected");
    if (p_event_disconnect) p_event_disconnect();
}

void bluetooth_characteristic_callbacks::onWrite(BLECharacteristic *pCharacteristic) {
    if (!p_event_receive) {
        ESP_LOGW(TAG, "Event receive is missing");
        return;
    }

    std::string value = pCharacteristic->getValue();
    size_t size = value.length();
    if (size == 0 || size > BLUETOOTH_WRITE_SIZE) {
        ESP_LOGW(TAG, "Receive data size is outside");
        return;
    }

    net_frame_t frame;
    memcpy(frame.bytes, value.c_str(), size);
    size--;

    ESP_LOGI(TAG, "Receive data: id:%d, size:%zu, data:%s", frame.value.id, size, frame.value.data);
    ESP_LOG_BUFFER_HEXDUMP(TAG, frame.bytes, size, ESP_LOG_DEBUG);

    p_event_receive(frame.value.id, frame.value.data, size);
}
