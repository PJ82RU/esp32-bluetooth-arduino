#ifndef ESP32_BLE_ARDUINO_BLE_CALLBACKS_H
#define ESP32_BLE_ARDUINO_BLE_CALLBACKS_H

#include <Arduino.h>
#include <BLEUtils.h>
#include "callback.h"
#include "include/packets/packet.h"

namespace hardware
{
    class BluetoothServerCallbacks final : public BLEServerCallbacks
    {
    public:
        /**
         * @brief Количество подключенных устройств
         * @note Атомарный доступ не требуется, так как BLE работает в одном потоке
         */
        uint8_t connected_devices = 0;

        void onConnect(BLEServer* server) override;
        void onDisconnect(BLEServer* server) override;
    };

    class BluetoothCharacteristicCallbacks final : public BLECharacteristicCallbacks
    {
    public:
        tools::Callback* callback = nullptr;

        void onWrite(BLECharacteristic* characteristic) override;
    };
}

#endif // ESP32_BLE_ARDUINO_BLE_CALLBACKS_H
