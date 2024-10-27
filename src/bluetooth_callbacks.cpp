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
        if (pCharacteristic) {
            std::string value = pCharacteristic->getValue();
            size_t size = value.length();
            if (size > 1 & size <= sizeof(net_frame_t)) {
                net_frame_t frame;
                memcpy(frame.bytes, value.c_str(), size);
                callback->call(&frame);
                log_d("Receive data. Size: %zu", size);
            } else {
                log_w("Receive data size is outside");
            }
        } else {
            log_w("Characteristic not found");
        }
    }
}