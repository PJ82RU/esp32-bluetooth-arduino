#ifndef BACKEND_BLUETOOTH_H
#define BACKEND_BLUETOOTH_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "bluetooth_callbacks.h"

namespace hardware {
    class bluetooth_c {
    public:
        BLEServer *p_server = nullptr;
        BLEService *p_service = nullptr;
        BLECharacteristic *p_characteristic = nullptr;

        /**
         * Инициализация объектов
         * @param name                Имя устройства
         * @param service_uuid        UUID службы
         * @param characteristic_uuid UUID характеристики
         */
        void begin(const char *name, const char *service_uuid, const char *characteristic_uuid);

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
         * @return Наличие входящих данных
         */
        bool is_receive() const;

        /**
         * Входящие данные по Bluetooth
         * @param id   ID функции
         * @param data Массив данных
         * @param size Размер массива данных
         * @return Результат выполнения
         */
        bool receive(uint8_t &id, uint8_t *data, size_t &size);

    private:
        bluetooth_server_callbacks *_server_callbacks = nullptr;
        bluetooth_characteristic_callbacks *_characteristic_callback = nullptr;
        u_net_frame _buffer{};
        size_t _size = 0;

        /**
         * Событие входящих данных
         * @param data Данные
         * @param size Размер данных
         */
        static void _event_receive(const char *data, size_t size);
    };
}

extern hardware::bluetooth_c bluetooth;

#endif //BACKEND_BLUETOOTH_H
