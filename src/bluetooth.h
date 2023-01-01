#ifndef BACKEND_BLUETOOTH_H
#define BACKEND_BLUETOOTH_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "bluetooth_callbacks.h"

#define BLUETOOTH_TAG   "BLE"

namespace hardware {
    class bluetooth {
    public:
        static const char* TAG;

        static BLEServer* p_server;
        static BLEService* p_service;
        static BLECharacteristic* p_characteristic;

        /**
         * Инициализация объектов
         * @param name                Имя устройства
         * @param service_uuid        UUID службы
         * @param characteristic_uuid UUID характеристики
         * @param p_event_receive     Метод события входящих данных
         * @param p_event_connect     Метод события подключения
         * @param p_event_disconnect  Метод события отключения
         */
        static void begin(const char* name, const char* service_uuid, const char* characteristic_uuid, bluetooth_receive_t p_event_receive,
                          bluetooth_connect_t p_event_connect = nullptr, bluetooth_disconnect_t p_event_disconnect = nullptr);

        /**
         * Статус подключения устройства
         * @return Количество подключений
         */
        static uint8_t device_connected();

        /**
         * Отправка данных по Bluetooth
         * @param id   ID функции
         * @param data Массив данных
         * @param size Размер массива данных
         * @return Результат выполнения
         */
        static bool send(uint8_t id, const uint8_t* data, size_t size);

    private:
        static bluetooth_server_callbacks* _server_callbacks;
        static bluetooth_characteristic_callbacks* _characteristic_callback;
        static bluetooth_disconnect_t _event_disconnect;

        /** Событие отключения */
        static void _on_device_disconnect();
    };
}

#endif //BACKEND_BLUETOOTH_H
