#ifndef BACKEND_BLUETOOTH_H
#define BACKEND_BLUETOOTH_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "bluetooth_callbacks.h"

namespace hardware {
    class bluetooth_c {
    public:
        typedef size_t (*bluetooth_receive_t)(uint8_t, uint8_t *, size_t);
        /** Событие входящих данных (id, data, size_data) */
        bluetooth_receive_t event_receive = nullptr;

        /**
         * Инициализация объектов
         * @param name                Имя устройства
         * @param service_uuid        UUID службы
         * @param characteristic_uuid UUID характеристики
         */
        void begin(const char *name, const char *service_uuid, const char *characteristic_uuid);

        /** Сервер */
        BLEServer *server();
        /** Сервис */
        BLEService *service();
        /** Характеристика */
        BLECharacteristic *characteristic();

        ~bluetooth_c();

        /**
         * Статус подключения устройства
         * @return Количество подключений
         */
        uint8_t device_connected();

        /**
         * Отправка данных по Bluetooth
         * @param id   ID функции
         * @param data Массив данных
         * @param size Размер массива данных
         * @return Результат выполнения
         */
        bool send(uint8_t id, const uint8_t *data, size_t size);

        /**
         * Метод обработки
         * @return Результат выполнения
         */
        bool handle();

    private:
        BLEServer *_server = nullptr;
        BLEService *_service = nullptr;
        BLECharacteristic *_characteristic = nullptr;
        bluetooth_server_callbacks *_server_callbacks = nullptr;
        bluetooth_characteristic_callbacks *_characteristic_callback = nullptr;

        net_frame_buffer_t _buffer{};

        /**
         * Записать значение характеристики
         * @param frame Кадр данных
         * @param size Размер кадра данных
         */
        void _characteristic_set_value(net_frame_t &frame, size_t size);
    };
}

#endif //BACKEND_BLUETOOTH_H
