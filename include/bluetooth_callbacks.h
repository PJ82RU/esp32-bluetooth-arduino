#ifndef ESP32_BLE_ARDUINO_BLE_CALLBACKS_H
#define ESP32_BLE_ARDUINO_BLE_CALLBACKS_H

#include <Arduino.h>
#include <BLEUtils.h>
#include "callback.h"
#include "include/packets/packet.h"

namespace hardware
{
    /// @brief Класс обработчиков событий сервера BLE
    ///
    /// Реализует логику отслеживания подключений/отключений устройств
    /// Наследует стандартные колбэки BLEServerCallbacks от ESP32 BLE
    class BluetoothServerCallbacks final : public BLEServerCallbacks
    {
    public:
        /**
         * @brief Счетчик подключенных устройств
         * @note Атомарный доступ не требуется, так как все вызовы BLE
         *       обрабатываются в контексте одного потока (BLE task)
         */
        uint8_t connected_devices = 0;

        /**
         * @brief Обработчик подключения нового устройства
         * @param server Указатель на сервер BLE
         */
        void onConnect(BLEServer* server) override;

        /**
         * @brief Обработчик отключения устройства
         * @param server Указатель на сервер BLE
         */
        void onDisconnect(BLEServer* server) override;
    };

    /// @brief Класс обработчиков событий характеристик BLE
    ///
    /// Обрабатывает запись данных в характеристику и перенаправляет
    /// в зарегистрированный callback
    class BluetoothCharacteristicCallbacks final : public BLECharacteristicCallbacks
    {
    public:
        /// @brief Указатель на callback-функцию для обработки записей
        tools::Callback* callback = nullptr;

        /**
         * @brief Обработчик записи данных в характеристику
         * @param characteristic Указатель на измененную характеристику
         */
        void onWrite(BLECharacteristic* characteristic) override;
    };
}

#endif // ESP32_BLE_ARDUINO_BLE_CALLBACKS_H