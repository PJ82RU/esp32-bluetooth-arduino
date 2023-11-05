#ifndef BACKEND_BLUETOOTH_H
#define BACKEND_BLUETOOTH_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "bluetooth_callbacks.h"

#define BLE_BUFFER_SIZE     16      // 16 * sizeof(net_frame_t)

namespace hardware {
    typedef size_t (*ble_receive_t)(uint8_t, uint8_t *, size_t);

    class BluetoothLowEnergy {
    public:
        /** Функция обратного вызова входящих данных */
        ble_receive_t cb_receive = nullptr;

        /**
         * Обратный вызов входящего кадра
         * @param pv_parameters
         */
        friend void task_callback(void *pv_parameters);

        /** Bluetooth Low Energy */
        BluetoothLowEnergy();

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
         * @param id   ID функции
         * @param data Массив данных
         * @param size Размер массива
         * @return Размер данных
         */
        size_t receive(uint8_t &id, uint8_t *data, size_t size);

    protected:
        QueueHandle_t queue_ble_buffer{};

        /**
         * Записать значение характеристики
         * @param frame Кадр данных
         * @param size Размер кадра данных
         */
        void characteristic_set_value(net_frame_t &frame, size_t size);

    private:
        TaskHandle_t task_ble_cb{};

        BLEServer *ble_server = nullptr;
        BLEService *ble_service = nullptr;
        BLECharacteristic *ble_characteristic = nullptr;
        BluetoothServerCallbacks *ble_server_callbacks = nullptr;
        BluetoothCharacteristicCallbacks *ble_characteristic_callback = nullptr;
    };
}

#endif //BACKEND_BLUETOOTH_H
