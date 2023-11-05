#include "bluetooth_low_energy.h"
#include "esp32-hal-log.h"

namespace hardware {

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#pragma ide diagnostic ignored "modernize-use-auto"

    void task_callback(void *pv_parameters) {
        BluetoothLowEnergy *ble = (BluetoothLowEnergy *) pv_parameters;
        net_frame_t frame;

        for (;;) {
            if (ble->cb_receive) {
                if (xQueueReceive(ble->queue_ble_buffer, &frame, portMAX_DELAY) == pdTRUE) {
                    size_t size = ble->cb_receive(frame.value.id, frame.value.data, frame.value.size);
                    if (size != 0) ble->characteristic_set_value(frame, size + 3);
                }
            } else
                vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

#pragma clang diagnostic pop

    BluetoothLowEnergy::BluetoothLowEnergy() {
        queue_ble_buffer = xQueueCreate(BLE_BUFFER_SIZE, sizeof(net_frame_t));
        log_i("Queue buffer created");

        xTaskCreate(&task_callback, "BLE_CALLBACK", 8192, this, 15, &task_ble_cb);
        log_i("Task callback created");
    }

    BluetoothLowEnergy::~BluetoothLowEnergy() {
        end();

        vTaskDelete(task_ble_cb);
        log_i("Task callback deleted");
        vQueueDelete(queue_ble_buffer);
        log_i("Queue buffer deleted");
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
        ble_characteristic_callback->queue_ble_buffer = queue_ble_buffer;
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

    void BluetoothLowEnergy::characteristic_set_value(net_frame_t &frame, size_t size) {
        log_d("Send data: id: 0x%02x, size: %zu", frame.value.id, size);

        ble_characteristic->setValue(frame.bytes, size);
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
        characteristic_set_value(frame, size + 3);

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
        if (xQueueReceive(queue_ble_buffer, &frame, 0) == pdTRUE) {
            id = frame.value.id;
            if (size > frame.value.size) size = frame.value.size;
            memcpy(data, frame.value.data, size);
            return size;
        }

        log_d("No data to receive");
        return 0;
    }
}
