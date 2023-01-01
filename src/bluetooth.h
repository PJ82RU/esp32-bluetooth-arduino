#ifndef BACKEND_BLUETOOTH_H
#define BACKEND_BLUETOOTH_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "bluetooth_callbacks.h"

#define BLUETOOTH_TAG                           "BLE"
#define BLUETOOTH_TIMEOUT_SEND                  10
#define BLUETOOTH_TIMEOUT_START_ADVERTISING     500

namespace hardware {
    class bluetooth {
    public:
        /** Событие подключения */
        typedef void (*event_connected_t)();
        static volatile event_connected_t event_connected;

        /** Событие входящих данных (id, data, size_data) */
        typedef void (*event_receive_t)(uint8_t, const uint8_t[], size_t);
        static volatile event_receive_t event_receive;

        /**
         * Инициализация объектов
         * @param name                Имя устройства
         * @param service_uuid        UUID службы
         * @param characteristic_uuid UUID характеристики
         */
        static void begin(const char* name, const char* service_uuid, const char* characteristic_uuid);

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
        static bool send(uint8_t id, const uint8_t data[], size_t size);

        /** Метод обработки */
        static void handle();

    private:
        static const char* TAG;

        static BLEServer* _server;
        static BLEService* _service;
        static BLECharacteristic* _characteristic;
        static bluetooth_server_callbacks* _server_callbacks;
        static bluetooth_characteristic_callbacks* _characteristic_callback;

        static net_frame_t _buffer;
        static volatile uint8_t _old_device_connected;
        static volatile unsigned long ms_disconnected;
        static volatile unsigned long ms_send;
    };
}

#endif //BACKEND_BLUETOOTH_H
