#include "bluetooth.h"
using namespace hardware;
const char* bluetooth::TAG = BLUETOOTH_TAG;

volatile bluetooth::event_connected_t bluetooth::event_connected = nullptr;
volatile bluetooth::event_receive_t bluetooth::event_receive = nullptr;

BLEServer* bluetooth::_server = nullptr;
BLEService* bluetooth::_service = nullptr;
BLECharacteristic* bluetooth::_characteristic = nullptr;
bluetooth_server_callbacks* bluetooth::_server_callbacks = nullptr;
bluetooth_characteristic_callbacks* bluetooth::_characteristic_callback = nullptr;

net_frame_t bluetooth::_buffer;
volatile uint8_t bluetooth::_old_device_connected = 0;
volatile unsigned long bluetooth::ms_disconnected = 0;
volatile unsigned long bluetooth::ms_send = 0;

void bluetooth::begin(const char* name, const char* service_uuid, const char* characteristic_uuid) {
    // BLE Device
    BLEDevice::init(name);
    ESP_LOGI(TAG, "Device initialized, name: %s", name);

    // BLE Server
    _server = BLEDevice::createServer();
    _server_callbacks = new bluetooth_server_callbacks();
    _server->setCallbacks(_server_callbacks);
    ESP_LOGI(TAG, "Server created");

    // BLE Service
    _service = _server->createService(service_uuid);
    ESP_LOGI(TAG, "Service created, uuid: %s", service_uuid);

    // BLE Characteristic
    _characteristic = _service->createCharacteristic(
            characteristic_uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
            | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
    ESP_LOGI(TAG, "Characteristic created, uuid: %s", characteristic_uuid);

    // BLE Descriptor
    _characteristic->addDescriptor(new BLE2902());
    _characteristic_callback = new bluetooth_characteristic_callbacks();
    _characteristic->setCallbacks(_characteristic_callback);
    ESP_LOGI(TAG, "Descriptor added");

    // Запуск service
    _service->start();
    ESP_LOGI(TAG, "Service started");

    // Запуск advertising
    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(service_uuid);
    advertising->setScanResponse(false);
    advertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();
    ESP_LOGI(TAG, "Advertising started");

    ms_disconnected = 0;
    delay(10);
}

uint8_t bluetooth::device_connected() {
    return _server_callbacks != nullptr ? _server_callbacks->device_connected : 0;
}

bool bluetooth::send(uint8_t id, const uint8_t data[], size_t size) {
    if (device_connected() == 0) {
        ESP_LOGW(TAG, "Device not connected");
        return false;
    }
    if (!data || size == 0) {
        ESP_LOGW(TAG, "No data");
        return false;
    }

    unsigned long ms = millis();
    if (ms_send > ms) {
        ESP_LOGW(TAG, "Timeout send");
        return false;
    }
    ms_send = ms + BLUETOOTH_TIMEOUT_SEND;

    _buffer.value.id = id;
    memcpy(_buffer.value.data, data, size);
	size++;

    ESP_LOGI(TAG, "Send data: id:%d, size:%zu, data:%s", _buffer.value.id, size, _buffer.value.data);
    ESP_LOG_BUFFER_HEXDUMP(TAG, _buffer.bytes, BLUETOOTH_WRITE_SIZE, ESP_LOG_DEBUG);

    // отправляем данные по bluetooth
    _characteristic->setValue(_buffer.bytes, size);
    _characteristic->notify();
    return true;
}

void bluetooth::handle() {
    uint8_t connected = device_connected();

    // подключение
    if (connected > _old_device_connected) {

        _old_device_connected = connected;
        ms_disconnected = 0;
        ESP_LOGI(TAG, "Device connected, #:%d", connected);
        if (event_connected) event_connected();

    } else {

        // отключение
        if (connected < _old_device_connected) {
            if (ms_disconnected == 0) ms_disconnected = millis() + BLUETOOTH_TIMEOUT_START_ADVERTISING;
            else if (ms_disconnected <= millis()) {
                if (connected == 0) _server->startAdvertising();
                _old_device_connected = connected;
                ms_disconnected = 0;
                ESP_LOGI(TAG, "Device disconnected");
            }
        }
    }
}
