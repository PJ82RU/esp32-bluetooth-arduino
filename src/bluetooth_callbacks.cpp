#include "bluetooth_callbacks.h"
#include "esp32-hal-log.h"

namespace hardware {
    void BluetoothServerCallbacks::onConnect(BLEServer *pServer) {
        device_connected++;
        log_i("Device connected");
    }

    void BluetoothServerCallbacks::onDisconnect(BLEServer *pServer) {
        device_connected--;
        log_i("Device disconnected");

        if (device_connected == 0 && pServer) {
            // время на подготовку стека bluetooth
            delay(500);
            pServer->startAdvertising();
        }
    }

    void BluetoothCharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
        if (!pCharacteristic) {
            log_w("Characteristic not found");
            return;
        }

        std::string value = pCharacteristic->getValue();
        size_t size = value.length();
        if (size < 2 || size > BLE_WRITE_SIZE) {
            log_w("Receive data size is outside");
            return;
        }

        net_frame_t frame{};
        memcpy(frame.bytes, value.c_str(), size);
        xQueueSend(queue_ble_buffer, &frame, 0);

        log_d("Receive data: id: 0x%02x, size: %zu", frame.value.id, frame.value.size);
    }
}