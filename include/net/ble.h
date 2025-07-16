#ifndef NET_BLE_H
#define NET_BLE_H

#include "esp32_c3_objects/callback.h"
#include "packets/packet.h"
#include "ble_config.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_gatts_api.h"
#include "esp_gap_ble_api.h"

namespace net
{
    /**
     * @brief Класс для работы с Bluetooth Low Energy (BLE) 5.0
     * @details Обеспечивает:
     * - Инициализацию BLE 5.0 стека
     * - Создание сервисов и характеристик
     * - Управление подключениями с поддержкой PHY 2M и Coded
     * - Обмен данными через BLE 5.0 с обратной совместимостью
     */
    class BLE
    {
    public:
        /// @brief Тег для логирования
        static constexpr auto TAG = "BLE";

        /**
         * @brief Конструктор BLE-контроллера
         * @param preset Пресет конфигурации (по умолчанию BLE4_DEFAULT)
         * @note Выбор пресета влияет на:
         * - Мощность передачи
         * - Режим энергосбережения
         * - Поддерживаемую версию BLE (4.2 или 5.0)
         * - Параметры рекламы и соединений
         */
        explicit BLE(BleConfig::Preset preset = BleConfig::Preset::BLE4_DEFAULT);
        ~BLE();

        // Запрет копирования и присваивания
        BLE(const BLE&) = delete;
        BLE& operator=(const BLE&) = delete;

        /**
         * @brief Инициализация BLE 5.0 стека
         * @param deviceName Имя BLE устройства
         * @param dataCallback Callback для обработки входящих данных
         * @return esp_err_t Код ошибки ESP-IDF
         * @note В режиме HIGH_POWER используются настройки для максимальной производительности:
         *       - Максимальная мощность передачи (ESP_PWR_LVL_P9)
         *       - Отключение энергосбережения
         *       - Поддержка BLE 5.0
         *       - Увеличенное количество активных соединений
         */
        esp_err_t initialize(const std::string& deviceName,
                             std::unique_ptr<esp32_c3::objects::Callback> dataCallback);

        /**
         * @brief Быстрый старт BLE с настройками по умолчанию
         * @param deviceName Имя BLE устройства
         * @param dataCallback Callback для обработки входящих данных
         * @return esp_err_t Код ошибки ESP-IDF
         */
        esp_err_t quickStart(const std::string& deviceName,
                             std::unique_ptr<esp32_c3::objects::Callback> dataCallback);

        /**
         * @brief Конвертация строкового UUID в esp_bt_uuid_t
         * @param uuidStr Строка UUID в формате "00001234-0000-1000-8000-00805F9B34FB"
         * @param invertBytes Флаг инверсии байт (актуально для 128-бит UUID)
         * @return esp_bt_uuid_t Преобразованный UUID
         */
        static esp_bt_uuid_t uuidFromString(const std::string& uuidStr, bool invertBytes);

        /**
         * @brief Создание BLE сервиса по UUID
         * @param serviceUuid UUID создаваемого сервиса
         * @param isPrimary Флаг первичного сервиса (true по умолчанию)
         * @return esp_err_t
         */
        esp_err_t createService(const esp_bt_uuid_t& serviceUuid,
                                bool isPrimary = true) const;

        /**
         * @brief Создание BLE характеристики
         * @param charUuid UUID характеристики
         * @param properties Свойства характеристики (чтение/запись и т.д.)
         * @return esp_err_t Код ошибки ESP-IDF
         */
        esp_err_t createCharacteristic(const esp_bt_uuid_t& charUuid,
                                       esp_gatt_char_prop_t properties) const;

        /**
         * @brief Запуск BLE 5.0 рекламы
         * @return esp_err_t Код ошибки ESP-IDF
         */
        esp_err_t startAdvertising();

        /**
         * @brief Установка предпочтительных параметров PHY
         * @param txPhy Предпочтительный PHY для передачи (битовая маска)
         * @param rxPhy Предпочтительный PHY для приема (битовая маска)
         * @return esp_err_t Код ошибки ESP-IDF
         */
        esp_err_t setPreferredPhy(esp_ble_gap_phy_mask_t txPhy,
                                  esp_ble_gap_phy_mask_t rxPhy) const;

        /**
         * @brief Отправка данных через BLE
         * @param connId Идентификатор соединения (0 - всем подключенным)
         * @param buffer Буфер данных
         * @param size Длина данных в буфере
         * @return esp_err_t Код ошибки ESP-IDF
         */
        esp_err_t sendData(uint16_t connId, std::array<uint8_t, MAX_MTU>& buffer, size_t size) const;

        /**
         * @brief Отправка пакета данных через BLE
         * @param packet Ссылка на пакет для отправки
         * @return esp_err_t Код ошибки ESP-IDF
         */
        esp_err_t sendPacket(Packet& packet) const;

        /**
         * @brief Остановка BLE стека и освобождение ресурсов
         * @return esp_err_t Код ошибки ESP-IDF
         */
        esp_err_t stop();

        /**
         * @brief Получение количества подключенных устройств
         * @return uint8_t Количество активных подключений
         */
        uint8_t getConnectedDevicesCount() const noexcept;

        /**
         * @brief Получение текущего MTU
         * @return uint16_t Текущий размер MTU
         */
        uint16_t getMtu() const noexcept;

        /**
         * @brief Получить текущую конфигурацию
         * @return std::shared_ptr<const BleConfig> Конфигурация
         */
        std::shared_ptr<const BleConfig> getConfig() const;

        /**
         * @brief Обновить конфигурацию (только до инициализации)
         * @param newConfig Новая конфигурация
         * @return esp_err_t ESP_OK если успешно, ESP_ERR_INVALID_STATE если уже инициализирован
         */
        esp_err_t updateConfig(const BleConfig& newConfig);

    private:
        /**
         * @brief Обработчик событий GATT сервера
         */
        static void gattsEventHandler(esp_gatts_cb_event_t event,
                                      esp_gatt_if_t gattsIf,
                                      esp_ble_gatts_cb_param_t* param);

        /**
         * @brief Обработчик событий GAP
         */
        static void gapEventHandler(esp_gap_ble_cb_event_t event,
                                    esp_ble_gap_cb_param_t* param);

        /**
         * @brief Обработка события записи в характеристику
         */
        void handleWriteEvent(uint16_t connId, const esp_ble_gatts_cb_param_t* param) const;

        /**
         * @brief Запуск legacy рекламы (BLE 4.x)
         */
        esp_err_t startLegacyAdvertising();

        /**
         * @brief Настройка параметров расширенной рекламы BLE 5.0
         */
        esp_err_t configureExtendedAdvertising();

        /**
         * @brief Внутренний метод отправки данных конкретному устройству
         */
        esp_err_t sendToDevice(uint16_t connId, std::array<uint8_t, MAX_MTU>& buffer, size_t size) const noexcept;

        struct DeviceConnection
        {
            uint16_t connId;
            esp_bd_addr_t address;
        };

        mutable std::recursive_mutex mMutex;              ///< Мьютекс для потокобезопасности
        BleConfig mConfig;                                ///< Текущая конфигурация BLE
        std::vector<DeviceConnection> mActiveConnections; ///< Список активных подключений

        std::string mDeviceName;                                    ///< Имя BLE-устройства для рекламы и подключения
        std::unique_ptr<esp32_c3::objects::Callback> mDataCallback; ///< Callback для данных
        esp_gatt_if_t mGattsIf = ESP_GATT_IF_NONE;                  ///< Интерфейс GATT
        uint16_t mServiceHandle = 0;                                ///< Хэндл сервиса
        uint16_t mCharHandle = 0;                                   ///< Хэндл характеристики
        uint16_t mMtu = 23;                                         ///< Текущий размер MTU
        bool mIsInitialized = false;                                ///< Флаг инициализации
    };
} // namespace net

#endif // NET_BLE_H
