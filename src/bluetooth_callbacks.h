#ifndef ESP32_BLE_ARDUINO_BLE_CALLBACKS_H
#define ESP32_BLE_ARDUINO_BLE_CALLBACKS_H

#include "Arduino.h"
#include <BLEUtils.h>
#include "callback.h"

#define BLE_FRAME_DATA_SIZE     509

namespace hardware
{
#pragma pack(push, 1)
    /** Сетевой кадр */
    typedef union net_frame_t
    {
        struct
        {
            uint8_t id; // ID пакета
            uint16_t size; // Размер данных
            uint8_t data[BLE_FRAME_DATA_SIZE]; // Данные
        } value;

        uint8_t bytes[3 + BLE_FRAME_DATA_SIZE];
    } net_frame_t;
#pragma pack(pop)

    class BluetoothServerCallbacks final :
        public BLEServerCallbacks
    {
    public:
        /** Статус подключения */
        uint8_t device_connected = 0;

        /**
         * Подключение по Bluetooth
         * @param pServer Сервер
         */
        void onConnect(BLEServer* pServer) override;

        /**
         * Отключение от Bluetooth
         * @param pServer Сервер
         */
        void onDisconnect(BLEServer* pServer) override;
    };

    class BluetoothCharacteristicCallbacks final :
        public BLECharacteristicCallbacks
    {
    public:
        /** Обратный вызов */
        tools::Callback* callback = nullptr;

        /**
         * Входящие данные
         * @param pCharacteristic
         */
        void onWrite(BLECharacteristic* pCharacteristic) override;
    };
}

#endif //ESP32_BLE_ARDUINO_BLE_CALLBACKS_H
