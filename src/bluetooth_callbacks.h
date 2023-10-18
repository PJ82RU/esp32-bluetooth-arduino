#ifndef ESP32_BLUETOOTH_ARDUINO_BLUETOOTH_CALLBACKS_H
#define ESP32_BLUETOOTH_ARDUINO_BLUETOOTH_CALLBACKS_H

#include "types.h"
#include <BLEUtils.h>

namespace hardware {
    class BluetoothServerCallbacks :
            public BLEServerCallbacks {
    public:
        /** Статус подключения */
        uint8_t device_connected = 0;

        /**
         * Подключение по Bluetooth
         * @param pServer Сервер
         */
        void onConnect(BLEServer *pServer) override;

        /**
         * Отключение от Bluetooth
         * @param pServer Сервер
         */
        void onDisconnect(BLEServer *pServer) override;
    };

    class BluetoothCharacteristicCallbacks :
            public BLECharacteristicCallbacks {
    public:
        net_frame_buffer_t* buffer = nullptr;

        /**
         * Входящие данные
         * @param pCharacteristic
         */
        void onWrite(BLECharacteristic *pCharacteristic) override;
    };
}

#endif //ESP32_BLUETOOTH_ARDUINO_BLUETOOTH_CALLBACKS_H
