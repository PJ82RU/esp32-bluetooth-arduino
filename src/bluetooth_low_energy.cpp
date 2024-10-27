#include "bluetooth_low_energy.h"
#include "esp32-hal-log.h"

namespace hardware {
    void BluetoothLowEnergy::on_response(void *p_value, void *p_parameters) {
        if (!p_value || !p_parameters) return;
        auto *frame = (net_frame_t *) p_value;
        auto *ble = (BluetoothLowEnergy *) p_parameters;
        ble->characteristic_set_value(*frame);
    }

    BluetoothLowEnergy::BluetoothLowEnergy(uint8_t num, uint32_t stack_depth) :
            callback(num, sizeof(net_frame_t), "CALLBACK_BLE", stack_depth) {
        callback.parent_callback.set(on_response, this);
    }

    BluetoothLowEnergy::~BluetoothLowEnergy() {
        end();
    }

    bool BluetoothLowEnergy::begin(const char *name, const char *service_uuid, const char *characteristic_uuid) {
        bool result = false;
        if (semaphore.take()) {
            if (!ble_server) {
                // BLE Device
                BLEDevice::init(name);
                log_i("Device initialized, name: %s", name);

                // BLE Server
                ble_server = BLEDevice::createServer();
                if (ble_server) {
                    ble_server_callbacks = new BluetoothServerCallbacks();
                    ble_server->setCallbacks(ble_server_callbacks);
                    log_i("Server created");

                    // BLE Service
                    ble_service = ble_server->createService(service_uuid);
                    if (ble_service) {
                        log_i("Service created, uuid: %s", service_uuid);

                        // BLE Characteristic
                        ble_characteristic = ble_service->createCharacteristic(characteristic_uuid,
                                                                               BLECharacteristic::PROPERTY_READ |
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

                        result = true;
                        log_i("Advertising started");
                    } else {
                        BLEDevice::deinit(true);
                        delete ble_server_callbacks;
                        log_w("The service has not been created");
                    }
                } else {
                    log_w("The server has not been created");
                }
            } else {
                log_w("The server is already running");
            }
            semaphore.give();
        }
        return result;
    }

    void BluetoothLowEnergy::end() {
        if (semaphore.take()) {
            if (ble_server) {
                BLEDevice::stopAdvertising();
                ble_service->stop();
                BLEDevice::deinit(true);
                ble_server = nullptr;

                delete ble_server_callbacks;
                delete ble_characteristic_callback;

                log_i("Server BLE stopped");
            }
            semaphore.give();
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
        // защита от перегруза стека bluetooth
        semaphore.wait_time();

        log_d("Send data: id: 0x%02x, size: %zu", frame.value.id, frame.value.size);
        ble_characteristic->setValue(frame.bytes, frame.value.size + 3);
        ble_characteristic->notify();

        semaphore.set_wait_time(5);
    }

    bool BluetoothLowEnergy::send(uint8_t id, const uint8_t *data, size_t size) {
        bool result = false;
        if (semaphore.take()) {
            if (device_connected() != 0) {
                if (data && size != 0) {
                    net_frame_t frame{};
                    frame.value.id = id;
                    frame.value.size = size;
                    memcpy(frame.value.data, data, size);
                    characteristic_set_value(frame);
                    result = true;
                } else {
                    log_w("No data");
                }
            } else {
                log_w("Device not connected");
            }
            semaphore.give();
        }
        return result;
    }

    size_t BluetoothLowEnergy::receive(uint8_t &id, uint8_t *data, size_t size) {
        size_t result = 0;
        if (device_connected() != 0) {
            if (data && size != 0) {
                net_frame_t frame{};
                if (callback.read(&frame)) {
                    id = frame.value.id;
                    result = size > frame.value.size ? frame.value.size : size;
                    memcpy(data, frame.value.data, result);
                    log_d("Reading data from the buffer: id: 0x%02x, size: %zu", frame.value.id, frame.value.size);
                }
            } else {
                log_w("No data");
            }
        } else {
            log_w("Device not connected");
        }
        return size;
    }
}
