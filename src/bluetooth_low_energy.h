#ifndef BACKEND_BLUETOOTH_H
#define BACKEND_BLUETOOTH_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "bluetooth_callbacks.h"

namespace hardware {
    class BluetoothLowEnergy {
    public:
        /**
         * Запустить BLE сервер
         * @param name                Имя устройства
         * @param service_uuid        UUID службы
         * @param characteristic_uuid UUID характеристики
         * @return Результат выполнения
         */
        bool start(const char *name, const char *service_uuid, const char *characteristic_uuid);

        /** Остановить BLE сервер */
        void stop();

        ~BluetoothLowEnergy();

        /** Сервер */
        BLEServer *server();
        /** Сервис */
        BLEService *service();
        /** Характеристика */
        BLECharacteristic *characteristic();

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
         * Входящие данные по Bluetooth
         * @param id   ID функции
         * @param data Массив данных
         * @param size Размер массива
         * @return Размер данных
         */
        size_t receive(uint8_t &id, uint8_t *data, size_t size);

        /**
         * Метод обработки
         * @param cb Функция обратного вызова
         * @return Результат выполнения
         */
        bool handle(ble_receive_t cb);

    private:
        BLEServer *_server = nullptr;
        BLEService *_service = nullptr;
        BLECharacteristic *_characteristic = nullptr;
        BluetoothServerCallbacks *_server_callbacks = nullptr;
        BluetoothCharacteristicCallbacks *_characteristic_callback = nullptr;

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
