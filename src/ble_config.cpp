#include "net/ble_config.h"

namespace net
{
    BleConfig::BleConfig(const Preset preset)
    {
        applyPreset(preset);
    }

    BleConfig::Preset BleConfig::currentPreset() const noexcept
    {
        return mCurrentPreset;
    }

    bool BleConfig::supportsExtendedAdvertising() const noexcept
    {
        return mCurrentPreset == Preset::BLE5_DEFAULT ||
            mCurrentPreset == Preset::BLE5_LOW_POWER ||
            mCurrentPreset == Preset::BLE5_ULTRA_PERF;
    }

    void BleConfig::applyPreset(const Preset preset) noexcept
    {
        mCurrentPreset = preset;

        switch (preset)
        {
        case Preset::BLE5_DEFAULT:
        case Preset::BLE5_LOW_POWER:
        case Preset::BLE5_ULTRA_PERF:
            applyBle5Preset(preset);
            break;

        case Preset::BLE4_DEFAULT:
        case Preset::BLE4_LOW_POWER:
        case Preset::BLE4_HIGH_PERF:
            applyBle4Preset(preset);
            break;

        default:
            applyBle5Preset(Preset::BLE5_DEFAULT);
        }
    }

    void BleConfig::copyFrom(const BleConfig& source) noexcept
    {
        // Копируем пресет
        mCurrentPreset = source.mCurrentPreset;

        // Копируем простые структуры
        controller = source.controller;
        extAdvParams = source.extAdvParams;
        legacyAdvParams = source.legacyAdvParams;
        security = source.security;
        connection = source.connection;
        advertising = source.advertising;
        gatt = source.gatt;
    }

    void BleConfig::applyBle5Preset(const Preset preset) noexcept
    {
        // Базовые настройки для BLE 5.x
        controller = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        controller.bluetooth_mode = ESP_BT_MODE_BLE;
        controller.ble_max_act = 6;
        extAdvParams.primary_phy = ESP_BLE_GAP_PHY_2M;
        extAdvParams.secondary_phy = ESP_BLE_GAP_PHY_2M;
        controller.ble_50_feat_supp = true; // Включаем BLE 5.0 фичи
        gatt.invertBytes = false;

        // Настройки для разных пресетов BLE 5
        switch (preset)
        {
        case Preset::BLE5_ULTRA_PERF:
            controller.txpwr_dft = ESP_PWR_LVL_P9;
            controller.sleep_mode = ESP_BT_SLEEP_MODE_NONE;
            extAdvParams.tx_power = ESP_PWR_LVL_P9;
            extAdvParams.interval_min = 0x40; // 40ms
            extAdvParams.interval_max = 0x60; // 60ms
            connection.txPhy = ESP_BLE_GAP_PHY_2M;
            connection.rxPhy = ESP_BLE_GAP_PHY_2M;
            break;

        case Preset::BLE5_DEFAULT:
            controller.txpwr_dft = ESP_PWR_LVL_P6;
            controller.sleep_mode = ESP_BT_SLEEP_MODE_1;
            extAdvParams.tx_power = ESP_PWR_LVL_P6;
            extAdvParams.interval_min = 0x80;  // 80ms
            extAdvParams.interval_max = 0x100; // 160ms
            connection.txPhy = ESP_BLE_GAP_PHY_2M | ESP_BLE_GAP_PHY_1M;
            connection.rxPhy = ESP_BLE_GAP_PHY_2M | ESP_BLE_GAP_PHY_1M;
            break;

        case Preset::BLE5_LOW_POWER:
            controller.txpwr_dft = ESP_PWR_LVL_N12;
            controller.sleep_mode = ESP_BT_SLEEP_MODE_1;
            extAdvParams.tx_power = ESP_PWR_LVL_N6;
            extAdvParams.interval_min = 0x200; // 320ms
            extAdvParams.interval_max = 0x400; // 640ms
            connection.txPhy = ESP_BLE_GAP_PHY_1M;
            connection.rxPhy = ESP_BLE_GAP_PHY_1M;
            break;

        default: ;
        }
    }

    void BleConfig::applyBle4Preset(const Preset preset) noexcept
    {
        // Базовые настройки для BLE 4.2
        controller = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        controller.bluetooth_mode = ESP_BT_MODE_BLE;
        controller.ble_max_act = 3; // Меньше соединений для BLE 4.2
        controller.ble_50_feat_supp = false; // Отключаем BLE 5.0 фичи

        // Настройки legacy рекламы
        legacyAdvParams.adv_type = ADV_TYPE_IND;
        legacyAdvParams.channel_map = ADV_CHNL_ALL;

        gatt.invertBytes = true;

        // Настройки для разных пресетов BLE 4
        switch (preset)
        {
        case Preset::BLE4_HIGH_PERF:
            controller.txpwr_dft = ESP_PWR_LVL_P9;
            legacyAdvParams.adv_int_min = 0x20; // 20ms
            legacyAdvParams.adv_int_max = 0x30; // 30ms
            break;

        case Preset::BLE4_DEFAULT:
            controller.txpwr_dft = ESP_PWR_LVL_P6;
            legacyAdvParams.adv_int_min = 0x40; // 40ms
            legacyAdvParams.adv_int_max = 0x60; // 60ms
            break;

        case Preset::BLE4_LOW_POWER:
            controller.txpwr_dft = ESP_PWR_LVL_N12;
            legacyAdvParams.adv_int_min = 0x80; // 80ms
            legacyAdvParams.adv_int_max = 0xC0; // 120ms
            break;

        default: ;
        }
    }
}
