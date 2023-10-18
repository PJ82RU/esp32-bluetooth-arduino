#include "bluetooth.h"
#include "esp32-hal-log.h"

using namespace hardware;

void Bluetooth::begin(const char *name, const char *service_uuid, const char *characteristic_uuid) {
    // BLE Device
    BLEDevice::init(name);
    log_i("Device initialized, name: %s", name);

    // BLE Server
    _server = BLEDevice::createServer();
    _server_callbacks = new bluetooth_server_callbacks();
    _server->setCallbacks(_server_callbacks);
    log_i("Server created");

    // BLE Service
    _service = _server->createService(service_uuid);
    log_i("Service created, uuid: %s", service_uuid);

    // BLE Characteristic
    _characteristic = _service->createCharacteristic(
            characteristic_uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
                                 | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
    log_i("Characteristic created, uuid: %s", characteristic_uuid);

    // BLE Descriptor
    _characteristic->addDescriptor(new BLE2902());
    _characteristic_callback = new bluetooth_characteristic_callbacks();
    _characteristic_callback->buffer = &_buffer;
    _characteristic->setCallbacks(_characteristic_callback);
    log_i("Descriptor added");

    // Запуск сервиса
    _service->start();
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

BLEServer *Bluetooth::server() {
    return _server;
}

BLEService *Bluetooth::service() {
    return _service;
}

BLECharacteristic *Bluetooth::characteristic() {
    return _characteristic;
}

Bluetooth::~Bluetooth() {
    if (_server) {
        BLEDevice::stopAdvertising();
        _service->stop();
        BLEDevice::deinit(true);

        delete _server_callbacks;
        delete _characteristic_callback;
    }
}

uint8_t Bluetooth::device_connected() {
    return _server_callbacks ? _server_callbacks->device_connected : 0;
}

void Bluetooth::_characteristic_set_value(net_frame_t &frame, size_t size) {
    log_d("Send data: id: %d, size: %zu", frame.value.id, size);

    _characteristic->setValue(frame.bytes, size);
    _characteristic->notify();
    // защита от перегруза стека bluetooth
    delay(5);
}

bool Bluetooth::send(uint8_t id, const uint8_t *data, size_t size) {
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
    _characteristic_set_value(frame, size + 1);

    return true;
}

size_t Bluetooth::receive(uint8_t &id, uint8_t *data, size_t size) {
    if (device_connected() == 0) {
        log_w("Device not connected");
        return 0;
    }
    if (!data || size == 0) {
        log_w("No data");
        return 0;
    }
    if (!_buffer.is_data || _buffer.size == 0) {
        log_d("No data to receive");
        return 0;
    }
    _buffer.is_data = false;
    id = _buffer.frame.value.id;
    size_t result_size = _buffer.size - 1;
    if (result_size > size) result_size = size;
    memcpy(data, _buffer.frame.value.data, result_size);
    return result_size;
}

bool Bluetooth::handle() {
    if (device_connected() == 0) {
//        log_w("Device not connected");
        return false;
    }
    if (!event_receive) {
        log_w("Event receive not found");
        return false;
    }
    if (!_buffer.is_data || _buffer.size == 0) {
//        log_d("No data to receive");
        return false;
    }

//    log_d("Incoming data");

    net_frame_t frame = _buffer.frame;
    _buffer.is_data = false;
    size_t size = event_receive(frame.value.id, frame.value.data, _buffer.size - 1);
    if (size != 0) _characteristic_set_value(frame, size + 1);

    return true;
}
