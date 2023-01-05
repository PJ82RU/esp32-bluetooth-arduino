#include "bluetooth.h"
#include "esp32-hal-log.h"

using namespace hardware;

void bluetooth_c::_event_receive(const char *data, size_t size) {
    log_d("Receive data: size: %zu, data: %s", size, data);
    bluetooth._size = size;
    memcpy(bluetooth._buffer.bytes, data, size);
}

void bluetooth_c::begin(const char *name, const char *service_uuid, const char *characteristic_uuid) {
    // BLE Device
    BLEDevice::init(name);
    log_i("Device initialized, name: %s", name);

    // BLE Server
    p_server = BLEDevice::createServer();
    _server_callbacks = new bluetooth_server_callbacks();
    p_server->setCallbacks(_server_callbacks);
    log_i("Server created");

    // BLE Service
    p_service = p_server->createService(service_uuid);
    log_i("Service created, uuid: %s", service_uuid);

    // BLE Characteristic
    p_characteristic = p_service->createCharacteristic(
            characteristic_uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
                                 | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
    log_i("Characteristic created, uuid: %s", characteristic_uuid);

    // BLE Descriptor
    p_characteristic->addDescriptor(new BLE2902());
    _characteristic_callback = new bluetooth_characteristic_callbacks();
    _characteristic_callback->p_event_receive = _event_receive;
    p_characteristic->setCallbacks(_characteristic_callback);
    log_i("Descriptor added");

    // Запуск сервиса
    p_service->start();
    log_i("Service started");
    delay(10);

    // Запуск рекламы
    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(service_uuid);
    advertising->setScanResponse(false);
    advertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();
    log_i("Advertising started");
}

uint8_t bluetooth_c::device_connected() {
    return _server_callbacks ? _server_callbacks->device_connected : 0;
}

bool bluetooth_c::send(uint8_t id, const uint8_t *data, size_t size) {
    if (device_connected() == 0) {
        log_w("Device not connected");
        return false;
    }
    if (!data || size == 0) {
        log_w("No data");
        return false;
    }

    net_frame_t frame{};
    frame.value.id = id;
    memcpy(frame.value.data, data, size);
    size++;
    log_i("Send data: id: %d, size: %zu, data: %s", frame.value.id, size, frame.value.data);

    // отправляем данные по bluetooth
    p_characteristic->setValue(frame.bytes, size);
    p_characteristic->notify();
    // защита от перегруза стека bluetooth
    delay(5);
    return true;
}

/** Наличие входящих данных */
bool bluetooth_c::is_receive() const {
    return _size > 0;
}

bool bluetooth_c::receive(uint8_t &id, uint8_t *data, size_t &size) {
    if (_size == 0) {
        log_d("No data to receive");
        return false;
    }

    id = _buffer.value.id;
    size = _size - 1;
    memcpy(data, _buffer.value.data, size);

    log_i("Receive data: id: %d, size: %zu, data: %s", id, size, data);
    _size = 0;
    return true;
}

bluetooth_c bluetooth;