#ifndef NET_BLE_CONFIG_H
#define NET_BLE_CONFIG_H

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include <string>

namespace net
{
    /**
     * @brief Полная конфигурация параметров Bluetooth Low Energy (BLE)
     * @details Содержит все настраиваемые параметры для работы с BLE стеком ESP-IDF,
     *          включая настройки контроллера, рекламы, соединений и GATT сервера.
     *          Поддерживает готовые пресеты конфигурации и ручную настройку.
     */
    struct BleConfig
    {
        /**
         * @brief Предустановленные режимы конфигурации
         */
        enum class Preset
        {
            DEFAULT,    ///< Стандартные настройки ESP-IDF (баланс между производительностью и энергопотреблением)
            HIGH_POWER, ///< Максимальная производительность (увеличенная мощность, отключено энергосбережение)
            LOW_POWER   ///< Режим энергосбережения (пониженная мощность, увеличенные интервалы рекламы)
        };

        /**
         * @brief Конфигурация BLE контроллера
         * @details Настройки из esp_bt_controller_config_t:
         * - txpwr_dft: Мощность передачи по умолчанию
         * - ble_max_conn: Максимальное количество соединений
         * - sleep_mode: Режим энергосбережения
         */
        esp_bt_controller_config_t controller = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

        /**
         * @brief Параметры расширенной рекламы
         * @details Настройки из esp_ble_gap_ext_adv_params_t:
         * - interval_min/max: Интервалы рекламы (в единицах 0.625 мс)
         * - tx_power: Мощность передачи рекламных пакетов
         * - primary_phy/secondary_phy: Используемые PHY
         */
        esp_ble_gap_ext_adv_params_t advertising = {
            .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE,   ///< Устройство разрешает подключения
            .interval_min = 0x100,                              ///< Минимальный интервал = 160 мс
            .interval_max = 0x200,                              ///< Максимальный интервал = 320 мс
            .channel_map = ADV_CHNL_ALL,                        ///< Использовать все BLE-каналы (37, 38, 39)
            .own_addr_type = BLE_ADDR_TYPE_PUBLIC,              ///< Публичный статический адрес
            .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,             ///< Тип адреса peer-устройства
            .peer_addr = {0},                                   ///< MAC-адрес peer-устройства (не используется)
            .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY, ///< Фильтрация подключений и сканирований
            .tx_power = ESP_PWR_LVL_P9,                         ///< Максимальная мощность передачи
            .primary_phy = ESP_BLE_GAP_PHY_1M,                  ///< Основная PHY: 1 Mbps (большая дальность)
            .max_skip = 0,                                      ///< Не пропускать рекламные события
            .secondary_phy = ESP_BLE_GAP_PHY_2M,                ///< Вторичная PHY: 2 Mbps (высокая скорость)
            .sid = 0,                                           ///< ID рекламного набора
            .scan_req_notif = false                             ///< Не уведомлять о scan-запросах
        };

        /**
             * @brief Параметры безопасности BLE соединения
             */
        struct
        {
            /**
             * @brief Требования к аутентификации
             * @details Использует значения из esp_ble_auth_req_t:
             * - ESP_LE_AUTH_NO_BOND: Без аутентификации
             * - ESP_LE_AUTH_BOND: Требуется bonding
             * - ESP_LE_AUTH_REQ_MITM: Требуется защита от MITM
             */
            esp_ble_auth_req_t authReq = ESP_LE_AUTH_BOND;

            /**
             * @brief Возможности ввода/вывода
             * @details Использует значения из esp_ble_io_cap_t:
             * - ESP_IO_CAP_NONE: Нет ввода/вывода
             * - ESP_IO_CAP_IN: Только ввод
             * - ESP_IO_CAP_OUT: Только вывод
             * - ESP_IO_CAP_IO: Ввод и вывод
             * - ESP_IO_CAP_KBDISP: Клавиатура + дисплей
             */
            esp_ble_io_cap_t ioCap = ESP_IO_CAP_NONE;

            /**
             * @brief Размер ключа шифрования (7-16 байт)
             */
            uint8_t keySize = 16;

            /**
             * @brief Маска инициирующих ключей
             * @details Комбинация флагов:
             * - ESP_BLE_ENC_KEY_MASK
             * - ESP_BLE_ID_KEY_MASK
             * - ESP_BLE_CSR_KEY_MASK
             * - ESP_BLE_LINK_KEY_MASK
             */
            uint8_t initKey = ESP_BLE_ENC_KEY_MASK;

            /**
             * @brief Маска ответных ключей
             * @details Аналогично initKey
             */
            uint8_t rspKey = ESP_BLE_ENC_KEY_MASK;
        } security;

        /**
         * @brief Параметры BLE соединения и физического уровня (PHY)
         */
        struct
        {
            /**
             * @brief Предпочитаемые PHY для передачи
             * @details Битовая маска из esp_ble_gap_phy_mask_t:
             * - ESP_BLE_GAP_PHY_1M
             * - ESP_BLE_GAP_PHY_2M
             * - ESP_BLE_GAP_PHY_CODED
             */
            esp_ble_gap_phy_mask_t txPhy = ESP_BLE_GAP_PHY_2M | ESP_BLE_GAP_PHY_1M;

            /**
             * @brief Предпочитаемые PHY для приема
             */
            esp_ble_gap_phy_mask_t rxPhy = ESP_BLE_GAP_PHY_2M | ESP_BLE_GAP_PHY_1M;
        } connection;

        /**
             * @brief Параметры рекламных данных
             */
        struct
        {
            /**
             * @brief Флаги рекламы
             */
            uint8_t flags = ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT;
        } advertisingParams;

        /**
         * @brief Параметры GATT сервера и характеристик
         */
        struct
        {
            /**
             * @brief Идентификатор приложения GATT
             * @details Должен быть уникальным для каждого GATT-сервера
             */
            uint16_t appId = 0;

            /**
             * @brief UUID сервиса по умолчанию
             * @note В формате "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
             */
            std::string serviceUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";

            /**
             * @brief UUID характеристики по умолчанию
             */
            std::string charUuid = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";

            /**
             * @brief Свойства характеристики
             * @details Комбинация флагов:
             * - ESP_GATT_CHAR_PROP_BIT_READ
             * - ESP_GATT_CHAR_PROP_BIT_WRITE
             * - ESP_GATT_CHAR_PROP_BIT_NOTIFY и др.
             */
            esp_gatt_char_prop_t charProperties =
                ESP_GATT_CHAR_PROP_BIT_READ |
                ESP_GATT_CHAR_PROP_BIT_WRITE |
                ESP_GATT_CHAR_PROP_BIT_NOTIFY;

            /**
             * @brief Права доступа к характеристике
             * @details Комбинация флагов:
             * - ESP_GATT_PERM_READ
             * - ESP_GATT_PERM_WRITE
             * - ESP_GATT_PERM_READ_ENCRYPTED и др.
             */
            esp_gatt_perm_t charPermissions =
                ESP_GATT_PERM_READ |
                ESP_GATT_PERM_WRITE;
        } gatt;

        /**
         * @brief Применяет предустановленную конфигурацию
         * @param[in] preset Выбранный пресет (DEFAULT, HIGH_POWER, LOW_POWER)
         * @details В зависимости от выбранного пресета настраивает:
         * - Мощность передачи
         * - Режим энергосбережения
         * - Интервалы рекламы
         * - Количество соединений
         */
        void applyPreset(const Preset preset)
        {
            // Общие настройки для всех пресетов
            controller = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
            controller.bluetooth_mode = ESP_BT_MODE_BLE;
            controller.ble_max_act = 6;

            // Настройки по умолчанию для рекламы
            advertising.tx_power = ESP_PWR_LVL_P9;
            advertising.interval_min = 0x100; // 160 ms
            advertising.interval_max = 0x200; // 320 ms

            // Специфичные настройки для каждого пресета
            switch (preset)
            {
            case Preset::HIGH_POWER:
                controller.txpwr_dft = ESP_PWR_LVL_P9;
                controller.sleep_mode = ESP_BT_SLEEP_MODE_NONE;
                advertising.interval_min = 0x80; // 80 ms
                break;

            case Preset::LOW_POWER:
                controller.txpwr_dft = ESP_PWR_LVL_N12;
                controller.sleep_mode = ESP_BT_SLEEP_MODE_1;
                advertising.tx_power = ESP_PWR_LVL_N6;
                advertising.interval_max = 0x400; // 640 ms
                break;

            default: // DEFAULT сохраняет базовые настройки
                break;
            }
        }
    };
} // namespace net::ble

#endif //NET_BLE_CONFIG_H
