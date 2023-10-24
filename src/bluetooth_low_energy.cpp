#include "bluetooth_low_energy.h"
#include "esp32-hal-log.h"

namespace hardware {
    bool BluetoothLowEnergy::start(const char *name, const char *service_uuid, const char *characteristic_uuid) {
        if (_server) {
            log_w("The server BLE is already running");
            return false;
        }

        // BLE Device
        BLEDevice::init(name);
        log_i("Device initialized, name: %s", name);

        // BLE Server
        _server = BLEDevice::createServer();
        if (!_server) return false;

        _server_callbacks = new BluetoothServerCallbacks();
        _server->setCallbacks(_server_callbacks);
        log_i("Server created");

        // BLE Service
        _service = _server->createService(service_uuid);
        if (!_service) {
            BLEDevice::deinit(true);
            delete _server_callbacks;
            return false;
        }
        log_i("Service created, uuid: %s", service_uuid);

        // BLE Characteristic
        _characteristic = _service->createCharacteristic(characteristic_uuid, BLECharacteristic::PROPERTY_READ |
                                                                              BLECharacteristic::PROPERTY_WRITE |
                                                                              BLECharacteristic::PROPERTY_NOTIFY |
                                                                              BLECharacteristic::PROPERTY_INDICATE);
        log_i("Characteristic created, uuid: %s", characteristic_uuid);

        // BLE Descriptor
        _characteristic->addDescriptor(new BLE2902());
        _characteristic_callback = new BluetoothCharacteristicCallbacks();
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

        return true;
    }

    void BluetoothLowEnergy::stop() {
        if (_server) {
            BLEDevice::stopAdvertising();
            _service->stop();
            BLEDevice::deinit(true);
            _server = nullptr;

            delete _server_callbacks;
            delete _characteristic_callback;

            log_i("Server BLE stopped");
        }
    }

    BluetoothLowEnergy::~BluetoothLowEnergy() {
        stop();
    }

    BLEServer *BluetoothLowEnergy::server() {
        return _server;
    }

    BLEService *BluetoothLowEnergy::service() {
        return _service;
    }

    BLECharacteristic *BluetoothLowEnergy::characteristic() {
        return _characteristic;
    }

    uint8_t BluetoothLowEnergy::device_connected() {
        return _server_callbacks ? _server_callbacks->device_connected : 0;
    }

    void BluetoothLowEnergy::_characteristic_set_value(net_frame_t &frame, size_t size) {
        log_d("Send data: id: %d, size: %zu", frame.value.id, size);

        _characteristic->setValue(frame.bytes, size);
        _characteristic->notify();
        // защита от перегруза стека bluetooth
        delay(5);
    }

    bool BluetoothLowEnergy::send(uint8_t id, const uint8_t *data, size_t size) {
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

    size_t BluetoothLowEnergy::receive(uint8_t &id, uint8_t *data, size_t size) {
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

    bool BluetoothLowEnergy::handle(ble_receive_t cb) {
        if (!cb) {
            log_e("Event receive not found");
            return false;
        }
        if (device_connected() == 0) {
            log_v("Device not connected");
            return false;
        }
        if (!_buffer.is_data || _buffer.size == 0) {
            log_v("No data to receive");
            return false;
        }

        log_d("Incoming data");

        net_frame_t frame = _buffer.frame;
        _buffer.is_data = false;
        size_t size = cb(frame.value.id, frame.value.data, _buffer.size - 1);
        if (size != 0) _characteristic_set_value(frame, size + 1);

        return true;
    }
}