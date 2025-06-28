#ifndef HARDWARE_BLE_H
#define HARDWARE_BLE_H

#include "callback.h"
#include "semaphore.h"
#include "bluetooth_callbacks.h"

namespace hardware
{
    /**
     * @brief Класс для работы с Bluetooth Low Energy (BLE)
     *
     * @details Предоставляет высокоуровневый интерфейс для создания BLE-сервера,
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
        static void onResponse(void* value, void* params) noexcept;

        /**
         * @brief Конструктор BLE сервера
         */
        explicit BluetoothLowEnergy() noexcept;

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
         * @param serviceUuid UUID сервиса в формате строки
         * @param characteristicUuid UUID характеристики в формате строки
         * @param callback Механизм callback для обработки входящих данных
         * @return true в случае успешной инициализации, false при ошибке
         */
        [[nodiscard]] bool begin(const char* name,
                                 const char* serviceUuid,
                                 const char* characteristicUuid,
                                 pj_tools::Callback* callback) noexcept;

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
        [[nodiscard]] uint8_t deviceConnected() const noexcept;

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
        void characteristicSetValue(Packet& packet) const noexcept;

        /**
         * @brief Освобождение ресурсов и очистка указателей
         */
        void cleanupResources() noexcept;

        /// Семафор для синхронизации доступа
        pj_tools::Semaphore mSemaphore;

        /// Указатель на BLE сервер
        BLEServer* mServer = nullptr;

        /// Указатель на BLE сервис
        BLEService* mService = nullptr;

        /// Указатель на BLE характеристику
        BLECharacteristic* mCharacteristic = nullptr;

        /// Callback для событий сервера
        BluetoothServerCallbacks* mServerCallbacks = nullptr;

        /// Callback для событий характеристики
        BluetoothCharacteristicCallbacks* mCharCallbacks = nullptr;
    };
} // namespace hardware

#endif // HARDWARE_BLE_H
