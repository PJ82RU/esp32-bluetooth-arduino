#ifndef ESP32_BLUETOOTH_ARDUINO_BLUETOOTH_CALLBACKS_H
#define ESP32_BLUETOOTH_ARDUINO_BLUETOOTH_CALLBACKS_H

#include <Arduino.h>
#include <BLEUtils.h>

#define BLUETOOTH_WRITE_SIZE        512
#define BLUETOOTH_FRAME_DATA_SIZE   BLUETOOTH_WRITE_SIZE - 1

namespace hardware {
#pragma pack(push,1)
    /** Структура сетевого кадра */
    typedef union u_net_frame {
        struct {
            uint8_t id;
            uint8_t data[BLUETOOTH_FRAME_DATA_SIZE];
        } value;
        uint8_t bytes[BLUETOOTH_WRITE_SIZE];
    } net_frame_t;
#pragma pack(pop)

    class bluetooth_server_callbacks :
            public BLEServerCallbacks {
    public:
        /** Статус подключения */
        uint8_t device_connected = 0;

        /**
         * Подключение по Bluetooth
         * @param pServer Сервер
         */
        void onConnect(BLEServer *pServer) override;

        /**
         * Отключение от Bluetooth
         * @param pServer Сервер
         */
        void onDisconnect(BLEServer *pServer) override;

    private:
        const char* TAG = "bluetooth server";
    };

    class bluetooth_characteristic_callbacks :
            public BLECharacteristicCallbacks {
    public:
        /** Событие входящих данных (id, data, size_data) */
        typedef void (*event_receive_t)(uint8_t, const uint8_t[], size_t);
        event_receive_t event_receive = nullptr;

        /**
         * Входящие данные
         * @param pCharacteristic
         */
        void onWrite(BLECharacteristic *pCharacteristic) override;

    private:
        const char* TAG = "bluetooth characteristic";
    };
}

#endif //ESP32_BLUETOOTH_ARDUINO_BLUETOOTH_CALLBACKS_H
