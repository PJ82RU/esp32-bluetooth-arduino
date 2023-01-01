#include "bluetooth.h"
using namespace hardware;
const char* bluetooth::TAG = BLUETOOTH_TAG;

BLEServer* bluetooth::p_server = nullptr;
BLEService* bluetooth::p_service = nullptr;
BLECharacteristic* bluetooth::p_characteristic = nullptr;
bluetooth_server_callbacks* bluetooth::_server_callbacks = nullptr;
bluetooth_characteristic_callbacks* bluetooth::_characteristic_callback = nullptr;
bluetooth_disconnect_t bluetooth::_event_disconnect = nullptr;

void bluetooth::begin(const char* name, const char* service_uuid, const char* characteristic_uuid, bluetooth_receive_t p_event_receive,
                      bluetooth_connect_t p_event_connect, bluetooth_disconnect_t p_event_disconnect) {
    // BLE Device
    BLEDevice::init(name);
    ESP_LOGI(TAG, "Device initialized, name:%s", name);

    // BLE Server
    p_server = BLEDevice::createServer();
    _server_callbacks = new bluetooth_server_callbacks();
    _server_callbacks->p_event_connect = p_event_connect;
    _server_callbacks->p_event_disconnect = _on_device_disconnect;
    _event_disconnect = p_event_disconnect;
    p_server->setCallbacks(_server_callbacks);
    ESP_LOGI(TAG, "Server created");

    // BLE Service
    p_service = p_server->createService(service_uuid);
    ESP_LOGI(TAG, "Service created, uuid:%s", service_uuid);

    // BLE Characteristic
    p_characteristic = p_service->createCharacteristic(
            characteristic_uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
            | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
    ESP_LOGI(TAG, "Characteristic created, uuid:%s", characteristic_uuid);

    // BLE Descriptor
    p_characteristic->addDescriptor(new BLE2902());
    _characteristic_callback = new bluetooth_characteristic_callbacks();
    _characteristic_callback->p_event_receive = p_event_receive;
    p_characteristic->setCallbacks(_characteristic_callback);
    ESP_LOGI(TAG, "Descriptor added");

    // Запуск сервиса
    p_service->start();
    ESP_LOGI(TAG, "Service started");
    delay(10);

    // Запуск рекламы
    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(service_uuid);
    advertising->setScanResponse(false);
    advertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();
    ESP_LOGI(TAG, "Advertising started");
}

uint8_t bluetooth::device_connected() {
    return _server_callbacks ? _server_callbacks->device_connected : 0;
}

bool bluetooth::send(uint8_t id, const uint8_t* data, size_t size) {
    if (device_connected() == 0) {
        ESP_LOGW(TAG, "Device not connected");
        return false;
    }
    if (!data || size == 0) {
        ESP_LOGW(TAG, "No data");
        return false;
    }

    net_frame_t frame;
    frame.value.id = id;
    memcpy(frame.value.data, data, size);
	size++;

    ESP_LOGI(TAG, "Send data: id:%d, size:%zu, data:%s", frame.value.id, size, frame.value.data);
    ESP_LOG_BUFFER_HEXDUMP(TAG, frame.bytes, BLUETOOTH_WRITE_SIZE, ESP_LOG_DEBUG);

    // отправляем данные по bluetooth
    p_characteristic->setValue(frame.bytes, size);
    p_characteristic->notify();
    // защита от перегруза стека bluetooth
    delay(5);
    return true;
}

void bluetooth::_on_device_disconnect() {
    // время на подготовку стека bluetooth
    delay(500);
    p_server->startAdvertising();
    if (_event_disconnect) _event_disconnect();
}
