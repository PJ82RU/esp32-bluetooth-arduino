#include "bluetooth_low_energy.h"
#include "esp32-hal-log.h"

namespace hardware {
    void BluetoothLowEnergy::on_response(void *p_value, void *p_parameters) {
        auto *frame = (net_frame_t *) p_value;
        auto *ble = (BluetoothLowEnergy *) p_parameters;
        ble->characteristic_set_value(*frame);
    }

    BluetoothLowEnergy::BluetoothLowEnergy() : callback(BLE_BUFFER_SIZE, sizeof(net_frame_t), "BLE_CALLBACK", 2048) {
        callback.cb_receive = on_response;
        callback.p_receive_parameters = this;
    }

    BluetoothLowEnergy::~BluetoothLowEnergy() {
        end();
    }

    bool BluetoothLowEnergy::begin(const char *name, const char *service_uuid, const char *characteristic_uuid) {
        if (ble_server) {
            log_w("The server BLE is already running");
            return false;
        }

        // BLE Device
        BLEDevice::init(name);
        log_i("Device initialized, name: %s", name);

        // BLE Server
        ble_server = BLEDevice::createServer();
        if (!ble_server) return false;

        ble_server_callbacks = new BluetoothServerCallbacks();
        ble_server->setCallbacks(ble_server_callbacks);
        log_i("Server created");

        // BLE Service
        ble_service = ble_server->createService(service_uuid);
        if (!ble_service) {
            BLEDevice::deinit(true);
            delete ble_server_callbacks;
            return false;
        }
        log_i("Service created, uuid: %s", service_uuid);

        // BLE Characteristic
        ble_characteristic = ble_service->createCharacteristic(characteristic_uuid, BLECharacteristic::PROPERTY_READ |
                                                                                    BLECharacteristic::PROPERTY_WRITE |
                                                                                    BLECharacteristic::PROPERTY_NOTIFY |
                                                                                    BLECharacteristic::PROPERTY_INDICATE);
        log_i("Characteristic created, uuid: %s", characteristic_uuid);

        // BLE Descriptor
        ble_characteristic->addDescriptor(new BLE2902());
        ble_characteristic_callback = new BluetoothCharacteristicCallbacks();
        ble_characteristic_callback->callback = &callback;
        ble_characteristic->setCallbacks(ble_characteristic_callback);
        log_i("Descriptor added");

        // Запуск сервиса
        ble_service->start();
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

    void BluetoothLowEnergy::end() {
        if (ble_server) {
            BLEDevice::stopAdvertising();
            ble_service->stop();
            BLEDevice::deinit(true);
            ble_server = nullptr;

            delete ble_server_callbacks;
            delete ble_characteristic_callback;

            log_i("Server BLE stopped");
        }
    }

    BLEServer *BluetoothLowEnergy::server() {
        return ble_server;
    }

    BLEService *BluetoothLowEnergy::service() {
        return ble_service;
    }

    BLECharacteristic *BluetoothLowEnergy::characteristic() {
        return ble_characteristic;
    }

    uint8_t BluetoothLowEnergy::device_connected() {
        return ble_server_callbacks ? ble_server_callbacks->device_connected : 0;
    }

    void BluetoothLowEnergy::characteristic_set_value(net_frame_t &frame) {
        log_d("Send data: id: 0x%02x, size: %zu", frame.value.id, frame.value.size);

        ble_characteristic->setValue(frame.bytes, frame.value.size + BLE_HEADER_SIZE);
        ble_characteristic->notify();
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
        frame.value.size = size;
        memcpy(frame.value.data, data, size);
        characteristic_set_value(frame);
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

        net_frame_t frame{};
        if (!callback.read(&frame)) return 0;

        id = frame.value.id;
        if (size > frame.value.size) size = frame.value.size;
        memcpy(data, frame.value.data, size);

        log_d("Reading data from the buffer: id: 0x%02x, size: %zu", frame.value.id, frame.value.size);
        return size;
    }
}
