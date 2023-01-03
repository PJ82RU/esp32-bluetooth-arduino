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
         * @param p_event_receive     Метод события входящих данных
         * @param p_event_connect     Метод события подключения
         * @param p_event_disconnect  Метод события отключения
         */
        void begin(const char *name, const char *service_uuid, const char *characteristic_uuid,
                   bluetooth_receive_t p_event_receive,
                   bluetooth_connect_t p_event_connect = nullptr, bluetooth_disconnect_t p_event_disconnect = nullptr);

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

    private:
        bluetooth_server_callbacks *_server_callbacks = nullptr;
        bluetooth_characteristic_callbacks *_characteristic_callback = nullptr;
    };
}

extern hardware::bluetooth_c bluetooth;

#endif //BACKEND_BLUETOOTH_H
