#ifndef BACKEND_BLUETOOTH_H
#define BACKEND_BLUETOOTH_H

#include "callback.h"
#include "semaphore.h"
#include "bluetooth_callbacks.h"

namespace hardware
{
    /**
     * @brief Класс для работы с Bluetooth Low Energy (BLE)
     * 
     * Предоставляет высокоуровневый интерфейс для создания BLE-сервера,
     * управления подключениями и обмена данными через BLE характеристики.
     */
    class BluetoothLowEnergy
    {
    public:
        /**
         * @brief Статический метод обработки ответа
         * @param value Указатель на данные для отправки
         * @param params Указатель на объект BluetoothLowEnergy
         */
        static void on_response(void* value, void* params) noexcept;

        /**
         * @brief Конструктор BLE сервера
         * @param num_packets Количество пакетов в буфере callback (по умолчанию 16)
         * @param stack_depth Глубина стека для задачи callback (по умолчанию 4096)
         */
        explicit BluetoothLowEnergy(uint8_t num_packets = 16, uint32_t stack_depth = 4096) noexcept;

        /**
         * @brief Деструктор (останавливает сервер и освобождает ресурсы)
         */
        ~BluetoothLowEnergy();

        // Запрещаем копирование
        BluetoothLowEnergy(const BluetoothLowEnergy&) = delete;
        BluetoothLowEnergy& operator=(const BluetoothLowEnergy&) = delete;

        /**
         * @brief Инициализация и запуск BLE сервера
         * @param name Имя BLE устройства
         * @param service_uuid UUID сервиса в формате строки
         * @param characteristic_uuid UUID характеристики в формате строки
         * @return true в случае успешной инициализации, false при ошибке
         */
        [[nodiscard]] bool begin(const char* name, const char* service_uuid, const char* characteristic_uuid) noexcept;

        /**
         * @brief Остановка BLE сервера и освобождение ресурсов
         */
        void end() noexcept;

        // Геттеры
        /**
         * @brief Получить указатель на BLE сервер
         */
        [[nodiscard]] BLEServer* server() const noexcept;

        /**
         * @brief Получить указатель на BLE сервис
         */
        [[nodiscard]] BLEService* service() const noexcept;

        /**
         * @brief Получить указатель на BLE характеристику
         */
        [[nodiscard]] BLECharacteristic* characteristic() const noexcept;

        /**
         * @brief Получить количество подключенных устройств
         */
        [[nodiscard]] uint8_t device_connected() const noexcept;

        // Операции с данными
        /**
         * @brief Отправить данные через BLE характеристику
         * @param packet Структура с данными для отправки
         * @return true если данные успешно отправлены
         */
        [[nodiscard]] bool send(Packet& packet) const noexcept;

        /**
         * @brief Получить данные из BLE характеристики
         * @param packet Структура для записи полученных данных
         * @return true если данные успешно получены
         */
        [[nodiscard]] bool receive(Packet& packet) const noexcept;

    private:
        /**
         * @brief Установка значения характеристики и отправка уведомления
         * @param packet Структура с данными для отправки
         */
        void characteristic_set_value(Packet& packet) const noexcept;

        /**
         * @brief Освобождение ресурсов и очистка указателей
         */
        void cleanup_resources() noexcept;

        tools::Callback callback_; ///< Механизм callback для обработки входящих данных
        tools::Semaphore semaphore_; ///< Семафор для синхронизации доступа

        BLEServer* server_ = nullptr; ///< Указатель на BLE сервер
        BLEService* service_ = nullptr; ///< Указатель на BLE сервис
        BLECharacteristic* characteristic_ = nullptr; ///< Указатель на BLE характеристику
        BluetoothServerCallbacks* server_callbacks_ = nullptr; ///< Callback для событий сервера
        BluetoothCharacteristicCallbacks* char_callbacks_ = nullptr; ///< Callback для событий характеристики
    };
}

#endif // BACKEND_BLUETOOTH_H
