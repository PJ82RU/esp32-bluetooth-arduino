#ifndef BACKEND_BLUETOOTH_H
#define BACKEND_BLUETOOTH_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "bluetooth_callbacks.h"
#include "callback.h"

namespace hardware {
    class BluetoothLowEnergy {
    public:
        /**
         * Ответ на запрос
         * @param p_value      Значение
         * @param p_parameters Параметры
         */
        static void on_response(void *p_value, void *p_parameters);

        /** Функция обратного вызова входящих данных */
        tools::Callback callback;

        /**
         * Bluetooth Low Energy
         * @param num         Количество net_frame_t в буфере
         * @param stack_depth Глубина стека
         */
        BluetoothLowEnergy(uint8_t num = 16, uint32_t stack_depth = 4096);

        ~BluetoothLowEnergy();

        /**
         * Запустить BLE сервер
         * @param name                Имя устройства
         * @param service_uuid        UUID службы
         * @param characteristic_uuid UUID характеристики
         * @return Результат выполнения
         */
        bool begin(const char *name, const char *service_uuid, const char *characteristic_uuid);

        /** Остановить BLE сервер */
        void end();


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
         * @param id    ID функции
         * @param data  Массив данных
         * @param size  Размер массива
         * @return Размер данных
         */
        size_t receive(uint8_t &id, uint8_t *data, size_t size);

    protected:
        /**
         * Записать значение характеристики
         * @param frame Кадр данных
         */
        void characteristic_set_value(net_frame_t &frame);

    private:
        BLEServer *ble_server = nullptr;
        BLEService *ble_service = nullptr;
        BLECharacteristic *ble_characteristic = nullptr;
        BluetoothServerCallbacks *ble_server_callbacks = nullptr;
        BluetoothCharacteristicCallbacks *ble_characteristic_callback = nullptr;
    };
}

#endif //BACKEND_BLUETOOTH_H
