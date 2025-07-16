#include "net/ble.h"

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

#include "esp_log.h"
#include "esp_err.h"

#include <cstring>
#include <cinttypes>

#define ESP_BLE_GAP_ALL_PHYS_PREF 0x03
#define ESP_BLE_GAP_PHY_OPTION_NO_PREF 0

// ReSharper disable once CppDFATimeOver
static net::BLE* sBLEInstance = nullptr;

namespace net
{
    BLE::BLE(const BleConfig::Preset preset) :
        mConfig(preset)
    {
        sBLEInstance = this;
        ESP_LOGD(TAG, "Instance created");
    }

    BLE::~BLE()
    {
        if (sBLEInstance == this)
        {
            sBLEInstance = nullptr;
        }
        stop();
    }

    esp_err_t BLE::initialize(const std::string& deviceName,
                              std::unique_ptr<esp32_c3::objects::Callback> dataCallback)
    {
        std::lock_guard lock(mMutex);

        if (mIsInitialized)
        {
            ESP_LOGW(TAG, "Already initialized");
            return ESP_OK;
        }

        if (deviceName.empty() || !dataCallback)
        {
            ESP_LOGE(TAG, "Invalid parameters: empty name or null callback");
            return ESP_ERR_INVALID_ARG;
        }

        mDeviceName = deviceName;
        mDataCallback = std::move(dataCallback);

        // Release classic BT memory
        ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

        // Initialize BLE 5.0 controller
        esp_err_t ret = esp_bt_controller_init(&mConfig.controller);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Controller init failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ret = esp_bt_controller_enable(static_cast<esp_bt_mode_t>(mConfig.controller.bluetooth_mode));
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Controller enable failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ret = esp_bluedroid_init();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ret = esp_bluedroid_enable();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
            return ret;
        }

        // Регистрируем оба обработчика событий
        ret = esp_ble_gatts_register_callback(gattsEventHandler);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "GATTS register callback failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ret = esp_ble_gap_register_callback(gapEventHandler);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "GAP register callback failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ret = esp_ble_gap_set_device_name(deviceName.c_str());
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Set device name failed: %s", esp_err_to_name(ret));
            return ret;
        }

        esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &mConfig.security.authReq, sizeof(uint8_t));
        esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &mConfig.security.ioCap, sizeof(uint8_t));
        esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &mConfig.security.keySize, sizeof(uint8_t));
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &mConfig.security.initKey, sizeof(uint8_t));
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &mConfig.security.rspKey, sizeof(uint8_t));

        ret = esp_ble_gatts_app_register(mConfig.gatt.appId);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "GATTS app register failed: %s", esp_err_to_name(ret));
            return ret;
        }

        mIsInitialized = true;

        // Устанавливаем предпочтительные параметры PHY по умолчанию
        ret = setPreferredPhy(mConfig.connection.txPhy, mConfig.connection.rxPhy);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set preferred PHY: %s (0x%04X)",
                     esp_err_to_name(ret), ret);
            return ret;
        }

        ESP_LOGI(TAG, "BLE 5.0 initialized successfully. Device name: %s", deviceName.c_str());
        return ESP_OK;
    }

    esp_err_t BLE::quickStart(const std::string& deviceName,
                              std::unique_ptr<esp32_c3::objects::Callback> dataCallback)
    {
        // 1. Инициализация BLE стека
        esp_err_t ret = initialize(deviceName, std::move(dataCallback));
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "BLE initialization failed: %s", esp_err_to_name(ret));
            return ret;
        }

        // 2. Создание сервиса
        const esp_bt_uuid_t serviceUuid = uuidFromString(mConfig.gatt.serviceUuid, mConfig.gatt.invertBytes);
        ret = createService(serviceUuid);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Service creation failed: %s", esp_err_to_name(ret));
            stop();
            return ret;
        }

        // 3. Создание характеристики
        const esp_bt_uuid_t charUuid = uuidFromString(mConfig.gatt.charUuid, mConfig.gatt.invertBytes);
        ret = createCharacteristic(charUuid, mConfig.gatt.charProperties);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Characteristic creation failed: %s", esp_err_to_name(ret));
            stop();
            return ret;
        }

        // 4. Запуск рекламы (автовыбор типа)
        ret = startAdvertising();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Advertising start failed: %s", esp_err_to_name(ret));
            stop();
            return ret;
        }

        ESP_LOGI(TAG, "Quick start completed. Service: %s, Char: %s",
                 mConfig.gatt.serviceUuid.c_str(), mConfig.gatt.charUuid.c_str());
        return ESP_OK;
    }

    esp_bt_uuid_t BLE::uuidFromString(const std::string& uuidStr, const bool invertBytes)
    {
        if (uuidStr.empty())
        {
            ESP_LOGE(TAG, "Empty UUID string");
            return esp_bt_uuid_t{
                .len = 0,
                .uuid = {.uuid16 = 0}
            };
        }

        esp_bt_uuid_t uuid = {
            .len = 0,
            .uuid = {.uuid16 = 0}
        };

        // Удаляем все дефисы и приводим к нижнему регистру
        std::string normalized;
        for (const char c : uuidStr)
        {
            if (c != '-')
            {
                normalized += tolower(c);
            }
        }

        // Проверяем длину и формат UUID
        if (normalized.length() == 4)
        {
            // 16-bit UUID (0000)
            uuid.len = ESP_UUID_LEN_16;
            uuid.uuid.uuid16 = static_cast<uint16_t>(strtoul(normalized.c_str(), nullptr, 16));
            ESP_LOGD(TAG, "Converted 16-bit UUID: 0x%04" PRIx16, uuid.uuid.uuid16);
        }
        else if (normalized.length() == 8)
        {
            // 32-bit UUID (00000000)
            uuid.len = ESP_UUID_LEN_32;
            uuid.uuid.uuid32 = static_cast<uint32_t>(strtoul(normalized.c_str(), nullptr, 16));
            ESP_LOGD(TAG, "Converted 32-bit UUID: 0x%08" PRIx32, uuid.uuid.uuid32);
        }
        else if (normalized.length() == 32)
        {
            // 128-bit UUID (00000000000000000000000000000000)
            uuid.len = ESP_UUID_LEN_128;

            // Проверяем что все символы hex
            if (normalized.find_first_not_of("0123456789abcdef") != std::string::npos)
            {
                ESP_LOGE(TAG, "Invalid characters in 128-bit UUID");
                uuid.len = 0;
                return uuid;
            }

            // Конвертируем каждые 2 символа в байт
            for (size_t i = 0; i < ESP_UUID_LEN_128; i++)
            {
                // Если `invertBytes` == true, читаем байты в обратном порядке
                const size_t bytePos = invertBytes
                                           ? (ESP_UUID_LEN_128 - 1 - i) * 2 // Инвертируем порядок
                                           : i * 2;                         // Оставляем как есть

                std::string byteStr = normalized.substr(bytePos, 2);
                uuid.uuid.uuid128[i] = static_cast<uint8_t>(strtoul(byteStr.c_str(), nullptr, 16));
            }

            // Логируем первые 4 байта для проверки
            ESP_LOGD(TAG, "Converted 128-bit UUID (first bytes): %02X%02X%02X%02X...",
                     uuid.uuid.uuid128[0], uuid.uuid.uuid128[1],
                     uuid.uuid.uuid128[2], uuid.uuid.uuid128[3]);
        }
        else
        {
            ESP_LOGE(TAG, "Invalid UUID format/length: %s", uuidStr.c_str());
            uuid.len = 0;
        }

        return uuid;
    }

    esp_err_t BLE::createService(const esp_bt_uuid_t& serviceUuid,
                                 const bool isPrimary) const
    {
        std::lock_guard lock(mMutex);

        if (!mIsInitialized)
        {
            ESP_LOGE(TAG, "BLE not initialized");
            return ESP_ERR_INVALID_STATE;
        }

        if (mServiceHandle != 0)
        {
            ESP_LOGW(TAG, "Service already created");
            return ESP_OK;
        }

        // Внутренняя подготовка структуры serviceId
        esp_gatt_srvc_id_t serviceId = {
            .id = {
                .uuid = serviceUuid,
                .inst_id = 0 // Всегда первый экземпляр
            },
            .is_primary = isPrimary
        };

        if (const esp_err_t ret = esp_ble_gatts_create_service(mGattsIf, &serviceId, 4); ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Create service failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ESP_LOGI(TAG, "Service creation initiated with UUID (len: %d)",
                 serviceUuid.len);
        return ESP_OK;
    }

    esp_err_t BLE::createCharacteristic(const esp_bt_uuid_t& charUuid,
                                        const esp_gatt_char_prop_t properties) const
    {
        std::lock_guard lock(mMutex);

        if (mServiceHandle == 0)
        {
            ESP_LOGE(TAG, "Service not created");
            return ESP_ERR_INVALID_STATE;
        }

        if (mCharHandle != 0)
        {
            ESP_LOGW(TAG, "Characteristic already created");
            return ESP_OK;
        }

        // Подготовка структуры управления атрибутом
        esp_attr_control_t control = {
            .auto_rsp = ESP_GATT_AUTO_RSP
        };

        // Подготовка значения характеристики
        esp_attr_value_t charValue = {
            .attr_max_len = MAX_MTU,
            .attr_len = 0,
            .attr_value = nullptr
        };

        // Создание копии UUID для передачи (так как charUuid - const ссылка)
        esp_bt_uuid_t charUuidCopy = charUuid;

        // 4. Вызов API с корректными параметрами
        const esp_err_t ret = esp_ble_gatts_add_char(
            mServiceHandle,               // Хэндл сервиса
            &charUuidCopy,                // Копия UUID характеристики
            mConfig.gatt.charPermissions, // Права доступа
            properties,                   // Свойства характеристики
            &charValue,                   // Значение характеристики
            &control                      // Управление атрибутом
        );

        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Add characteristic failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ESP_LOGI(TAG, "Characteristic creation initiated. UUID len: %d", charUuid.len);
        return ESP_OK;
    }

    esp_err_t BLE::startAdvertising()
    {
        std::lock_guard lock(mMutex);

        if (!mIsInitialized || mServiceHandle == 0)
        {
            ESP_LOGE(TAG, "BLE not properly initialized");
            return ESP_ERR_INVALID_STATE;
        }

        if (mConfig.supportsExtendedAdvertising())
        {
            ESP_LOGI(TAG, "Starting extended advertising (BLE 5.0)");
            return configureExtendedAdvertising();
        }
        else
        {
            ESP_LOGI(TAG, "Starting legacy advertising (BLE 4.2)");
            return startLegacyAdvertising();
        }
    }

    esp_err_t BLE::setPreferredPhy(const esp_ble_gap_phy_mask_t txPhy,
                                   const esp_ble_gap_phy_mask_t rxPhy) const
    {
        std::lock_guard lock(mMutex);

        if (!mIsInitialized)
        {
            ESP_LOGE(TAG, "BLE not initialized");
            return ESP_ERR_INVALID_STATE;
        }

        // Устанавливаем предпочтительные PHY для всех соединений
        for (const auto& [connId, address] : mActiveConnections)
        {
            const esp_err_t ret = esp_ble_gap_set_preferred_phy(
                const_cast<uint8_t*>(address), // [in] MAC-адрес устройства
                ESP_BLE_GAP_ALL_PHYS_PREF,     // [in] Все PHY доступны
                txPhy,                         // [in] Предпочтения TX PHY
                rxPhy,                         // [in] Предпочтения RX PHY
                ESP_BLE_GAP_PHY_OPTION_NO_PREF // [in] Без специальных опций
            );

            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to set PHY for conn %d: %s",
                         connId, esp_err_to_name(ret));
                return ret;
            }
        }

        // Устанавливаем PHY по умолчанию для новых соединений
        return esp_ble_gap_set_preferred_default_phy(txPhy, rxPhy);
    }

    esp_err_t BLE::sendData(const uint16_t connId, std::array<uint8_t, MAX_MTU>& buffer, const size_t size) const
    {
        std::lock_guard lock(mMutex);

        // Валидация параметров
        if (!mIsInitialized || size == 0 || size > MAX_MTU || size > mMtu)
        {
            ESP_LOGE(TAG, "Invalid send params: init=%d, len=%zu, max_mtu=%u, current_mtu=%u",
                     mIsInitialized, size, MAX_MTU, mMtu);
            return ESP_ERR_INVALID_ARG;
        }

        // Обработка broadcast
        if (connId == 0)
        {
            if (mActiveConnections.empty())
            {
                ESP_LOGW(TAG, "No connections for broadcast");
                return ESP_ERR_NOT_FOUND;
            }

            esp_err_t finalRet = ESP_OK;
            for (const auto& [connId, address] : mActiveConnections)
            {
                if (const esp_err_t ret = sendToDevice(connId, buffer, size); ret != ESP_OK)
                {
                    finalRet = ret;
                }
            }
            return finalRet;
        }

        // Отправка конкретному устройству
        return sendToDevice(connId, buffer, size);
    }

    esp_err_t BLE::sendToDevice(uint16_t connId, std::array<uint8_t, MAX_MTU>& buffer, const size_t size) const noexcept
    {
        // Поиск соединения
        const auto it = std::ranges::find_if(mActiveConnections,
                                             [connId](const auto& conn) { return conn.connId == connId; });

        if (it == mActiveConnections.cend())
        {
            ESP_LOGE(TAG, "Connection %u not found", connId);
            return ESP_ERR_NOT_FOUND;
        }

        // Оптимизированная отправка через кэшированные параметры
        const esp_err_t ret = esp_ble_gatts_send_indicate(
            mGattsIf, connId, mCharHandle, size, buffer.data(), false);

        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Send failed to %u: %s", connId, esp_err_to_name(ret));
        }

        return ret;
    }

    esp_err_t BLE::sendPacket(Packet& packet) const
    {
        return sendData(packet.id, packet.buffer, packet.size);
    }

    esp_err_t BLE::stop()
    {
        std::lock_guard lock(mMutex);
        if (!mIsInitialized) return ESP_OK;

        esp_err_t finalRet = ESP_OK;
        auto check_error = [&](const esp_err_t ret, const char* msg)
        {
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "%s: %s", msg, esp_err_to_name(ret));
                finalRet = ret;
            }
        };

        // Останавливаем рекламу (если активна)
        esp_ble_gap_stop_advertising();
        constexpr uint8_t extAdvInst = 0; // Останавливаем нулевую инстанцию
        check_error(esp_ble_gap_ext_adv_stop(1, &extAdvInst), "Stop extended advertising failed");

        if (mServiceHandle != 0)
        {
            check_error(esp_ble_gatts_delete_service(mServiceHandle), "Delete service failed");
            mServiceHandle = 0;
        }

        check_error(esp_bluedroid_disable(), "Bluedroid disable failed");
        check_error(esp_bluedroid_deinit(), "Bluedroid deinit failed");
        check_error(esp_bt_controller_disable(), "Controller disable failed");
        check_error(esp_bt_controller_deinit(), "Controller deinit failed");

        mGattsIf = ESP_GATT_IF_NONE;
        mCharHandle = 0;
        mActiveConnections.clear();
        mIsInitialized = false;
        mDataCallback.reset();

        ESP_LOGI(TAG, "BLE stopped with status: %s", esp_err_to_name(finalRet));
        return finalRet;
    }

    uint8_t BLE::getConnectedDevicesCount() const noexcept
    {
        std::lock_guard lock(mMutex);
        return mActiveConnections.size();
    }

    uint16_t BLE::getMtu() const noexcept
    {
        std::lock_guard lock(mMutex);
        return mMtu;
    }

    std::shared_ptr<const BleConfig> BLE::getConfig() const
    {
        std::lock_guard lock(mMutex);
        return std::make_shared<BleConfig>(mConfig);
    }

    esp_err_t BLE::updateConfig(const BleConfig& newConfig)
    {
        std::lock_guard lock(mMutex);

        if (mIsInitialized)
        {
            ESP_LOGE(TAG, "Cannot update config after initialization");
            return ESP_ERR_INVALID_STATE;
        }

        mConfig.copyFrom(newConfig);
        ESP_LOGD(TAG, "Config updated (preset: %d)",
                 static_cast<int>(mConfig.currentPreset()));
        return ESP_OK;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    void BLE::gattsEventHandler(const esp_gatts_cb_event_t event,
                                const esp_gatt_if_t gattsIf,
                                esp_ble_gatts_cb_param_t* param)
    {
        if (!sBLEInstance) return;

        switch (event)
        {
        case ESP_GATTS_REG_EVT:
            if (param->reg.status == ESP_OK)
            {
                sBLEInstance->mGattsIf = gattsIf;
                ESP_LOGI(TAG, "GATTS registered, interface: %d", gattsIf);
            }
            break;

        case ESP_GATTS_CREATE_EVT:
            if (param->create.status == ESP_OK)
            {
                sBLEInstance->mServiceHandle = param->create.service_handle;
                ESP_LOGI(TAG, "Service created, handle: %d", sBLEInstance->mServiceHandle);
            }
            break;

        case ESP_GATTS_ADD_CHAR_EVT:
            if (param->add_char.status == ESP_OK)
            {
                sBLEInstance->mCharHandle = param->add_char.attr_handle;
                ESP_LOGI(TAG, "Characteristic added, handle: %d", sBLEInstance->mCharHandle);
            }
            break;

        case ESP_GATTS_CONNECT_EVT:
            {
                std::lock_guard lock(sBLEInstance->mMutex);
                DeviceConnection conn = {
                    .connId = param->connect.conn_id,
                    .address = {}
                };
                memcpy(conn.address, param->connect.remote_bda, ESP_BD_ADDR_LEN);
                sBLEInstance->mActiveConnections.push_back(conn);
                ESP_LOGI(TAG, "Device connected. Conn_id: %d", param->connect.conn_id);
                break;
            }

        case ESP_GATTS_DISCONNECT_EVT:
            {
                std::lock_guard lock(sBLEInstance->mMutex);
                const uint16_t conn_id = param->disconnect.conn_id;
                const size_t before = sBLEInstance->mActiveConnections.size();

                // Удаляем через erase-remove idiom
                std::erase_if(
                    sBLEInstance->mActiveConnections,
                    [conn_id](const DeviceConnection& conn)
                    {
                        return conn.connId == conn_id;
                    });

                if (sBLEInstance->mActiveConnections.size() < before)
                {
                    ESP_LOGI(TAG, "Device disconnected. Conn_id: %d", conn_id);
                }
                break;
            }

        case ESP_GATTS_WRITE_EVT:
            sBLEInstance->handleWriteEvent(param->write.conn_id, param);
            break;

        case ESP_GATTS_MTU_EVT:
            sBLEInstance->mMtu = param->mtu.mtu > MAX_MTU ? MAX_MTU : param->mtu.mtu;
            ESP_LOGI(TAG, "MTU updated: %d", sBLEInstance->mMtu);
            break;

        default:
            ESP_LOGD(TAG, "Unhandled GATTS event: %d", event);
            break;
        }
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    void BLE::gapEventHandler(const esp_gap_ble_cb_event_t event,
                              esp_ble_gap_cb_param_t* param)
    {
        if (!sBLEInstance) return;

        switch (event)
        {
        case ESP_GAP_BLE_PHY_UPDATE_COMPLETE_EVT:
            if (param->phy_update.status == ESP_OK)
            {
                ESP_LOGI(TAG, "PHY updated: TX=%d, RX=%d",
                         param->phy_update.tx_phy, param->phy_update.rx_phy);
            }
            else
            {
                ESP_LOGE(TAG, "PHY update failed: %s",
                         esp_err_to_name(param->phy_update.status));
            }
            break;

        case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
            if (param->ext_adv_data_set.status != ESP_OK)
            {
                ESP_LOGE(TAG, "Set extended adv data failed: %s",
                         esp_err_to_name(param->ext_adv_data_set.status));
            }
            break;

        case ESP_GAP_BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT:
            if (param->ext_adv_set_rand_addr.status != ESP_OK)
            {
                ESP_LOGE(TAG, "Set extended adv random address failed: %s",
                         esp_err_to_name(param->ext_adv_set_rand_addr.status));
            }
            break;

        case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT:
            if (param->ext_adv_set_params.status != ESP_OK)
            {
                ESP_LOGE(TAG, "Set extended adv params failed: %s",
                         esp_err_to_name(param->ext_adv_set_params.status));
            }
            break;

        case ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT:
            if (param->ext_adv_start.status != ESP_OK)
            {
                ESP_LOGE(TAG, "Start extended adv failed: %s",
                         esp_err_to_name(param->ext_adv_start.status));
            }
            break;

        default:
            ESP_LOGD(TAG, "Unhandled GAP event: %d", event);
            break;
        }
    }

    esp_err_t BLE::startLegacyAdvertising()
    {
        std::lock_guard lock(mMutex);

        if (!mIsInitialized || mServiceHandle == 0)
        {
            ESP_LOGE(TAG, "BLE not initialized");
            return ESP_ERR_INVALID_STATE;
        }

        // 1. Подготовка advertising data
        constexpr size_t MAX_ADV_DATA_LEN = ESP_BLE_ADV_DATA_LEN_MAX - 2;
        const uint8_t nameLen = static_cast<uint8_t>(std::min(mDeviceName.length(), MAX_ADV_DATA_LEN));

        std::vector<uint8_t> advData = {
            // Flags
            0x02, ESP_BLE_AD_TYPE_FLAG, mConfig.advertising.flags,
            // Device name
            static_cast<uint8_t>(nameLen + 1), ESP_BLE_AD_TYPE_NAME_CMPL
        };
        advData.insert(advData.end(), mDeviceName.begin(), mDeviceName.begin() + nameLen);

        // 2. Добавление UUID сервиса (если 128-bit)
        if (const auto [len, uuid] = uuidFromString(mConfig.gatt.serviceUuid, mConfig.gatt.invertBytes); len == ESP_UUID_LEN_128)
        {
            advData.insert(advData.end(), {
                               static_cast<uint8_t>(1 + ESP_UUID_LEN_128),
                               ESP_BLE_AD_TYPE_128SRV_CMPL
                           });
            advData.insert(advData.end(), uuid.uuid128, uuid.uuid128 + ESP_UUID_LEN_128);
        }

        // 3. Установка advertising data
        if (const esp_err_t ret = esp_ble_gap_config_adv_data_raw(advData.data(), advData.size());
            ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Config adv data failed: %s", esp_err_to_name(ret));
            return ret;
        }

        // 4. Запуск рекламы
        if (const esp_err_t ret = esp_ble_gap_start_advertising(&mConfig.legacyAdvParams);
            ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Start advertising failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ESP_LOGI(TAG, "Legacy advertising started | Intv: %d-%dms | Flags: 0x%02X",
                 mConfig.legacyAdvParams.adv_int_min * 5/8,
                 mConfig.legacyAdvParams.adv_int_max * 5/8,
                 mConfig.advertising.flags);

        return ESP_OK;
    }

    esp_err_t BLE::configureExtendedAdvertising()
    {
        std::lock_guard lock(mMutex);

        if (!mIsInitialized || mServiceHandle == 0)
        {
            ESP_LOGE(TAG, "BLE not properly initialized");
            return ESP_ERR_INVALID_STATE;
        }

        // 1. Установка параметров рекламы из конфига
        esp_err_t ret = esp_ble_gap_ext_adv_set_params(0, &mConfig.extAdvParams);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Set extended adv params failed (0x%X): %s", ret, esp_err_to_name(ret));
            return ret;
        }

        // 2. Формирование advertising data
        std::vector<uint8_t> advData;

        // 2.1. Флаги из конфига (обязательное поле)
        advData.insert(advData.end(), {
                           0x02, // Length
                           ESP_BLE_AD_TYPE_FLAG,
                           mConfig.advertising.flags // Флаги из конфига
                       });

        // 2.2. Имя устройства (макс. 28 байт для лучшей совместимости)
        const uint8_t nameLen = static_cast<uint8_t>(std::min(mDeviceName.length(), static_cast<size_t>(28)));
        advData.insert(advData.end(), {
                           static_cast<uint8_t>(nameLen + 1), // Length
                           ESP_BLE_AD_TYPE_NAME_CMPL
                       });
        advData.insert(advData.end(), mDeviceName.begin(), mDeviceName.begin() + nameLen);

        // 2.3. UUID сервиса из конфига (автоматическое определение типа)
        if (const auto [uuidLen, uuid] = uuidFromString(mConfig.gatt.serviceUuid, mConfig.gatt.invertBytes); uuidLen == ESP_UUID_LEN_16)
        {
            advData.insert(advData.end(), {
                               0x03, // Length
                               ESP_BLE_AD_TYPE_16SRV_CMPL,
                               static_cast<uint8_t>(uuid.uuid16 & 0xFF),
                               static_cast<uint8_t>((uuid.uuid16 >> 8) & 0xFF)
                           });
        }
        else if (uuidLen == ESP_UUID_LEN_128)
        {
            advData.insert(advData.end(), {
                               static_cast<uint8_t>(1 + ESP_UUID_LEN_128),
                               ESP_BLE_AD_TYPE_128SRV_CMPL
                           });
            advData.insert(advData.end(), uuid.uuid128, uuid.uuid128 + ESP_UUID_LEN_128);
        }

        // 3. Установка advertising data
        ret = esp_ble_gap_config_ext_adv_data_raw(0, advData.size(), advData.data());
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Config adv data failed (0x%X): %s", ret, esp_err_to_name(ret));
            return ret;
        }

        // 4. Scan Response Data (оставляем пустым, но можно добавить в конфиг при необходимости)
        static constexpr std::array<uint8_t, 0> scanRspData = {};
        ret = esp_ble_gap_config_ext_scan_rsp_data_raw(0, scanRspData.size(), scanRspData.data());
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Config scan rsp data failed (0x%X): %s", ret, esp_err_to_name(ret));
            return ret;
        }

        // 5. Запуск рекламы
        constexpr esp_ble_gap_ext_adv_t extAdv = {
            .instance = 0,
            .duration = 0,  // Бесконечно
            .max_events = 0 // Без ограничений
        };

        ret = esp_ble_gap_ext_adv_start(1, &extAdv);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Start extended adv failed (0x%X): %s", ret, esp_err_to_name(ret));
            return ret;
        }

        ESP_LOGI(TAG, "Extended advertising started | PHY: %s | Interval: %.1f-%.1fms | TxPower: %ddBm | UUID: %s",
                 mConfig.extAdvParams.primary_phy == ESP_BLE_GAP_PHY_2M ? "2M" : "1M",
                 mConfig.extAdvParams.interval_min * 0.625f,
                 mConfig.extAdvParams.interval_max * 0.625f,
                 mConfig.extAdvParams.tx_power,
                 mConfig.gatt.serviceUuid.c_str());

        return ESP_OK;
    }

    void BLE::handleWriteEvent(const uint16_t connId, const esp_ble_gatts_cb_param_t* param) const
    {
        if (param == nullptr)
        {
            ESP_LOGE(TAG, "Null event param for conn: %u", connId);
            return;
        }

        std::lock_guard lock(mMutex);

        // Проверка callback
        if (mDataCallback == nullptr)
        {
            ESP_LOGE(TAG, "Data callback is null. Conn: %u", connId);
            return;
        }

        // Валидация handle
        if (param->write.handle != mCharHandle)
        {
            ESP_LOGW(TAG, "Invalid handle %u (expected %u). Conn: %u",
                     param->write.handle, mCharHandle, connId);
            return;
        }

        // Валидация размера данных
        const size_t dataLen = param->write.len;
        if (dataLen == 0 || dataLen > MAX_MTU)
        {
            ESP_LOGW(TAG, "Invalid data size: %zu (max %u). Conn: %u",
                     dataLen, MAX_MTU, connId);
            return;
        }

        // Создание и заполнение пакета
        Packet packet;
        packet.id = connId;

        if (!packet.setPayload(param->write.value, dataLen))
        {
            ESP_LOGE(TAG, "Payload set failed. Conn: %u, Size: %zu", connId, dataLen);
            return;
        }

        // Вызов callback
        mDataCallback->invoke(&packet);

        // Отправка подтверждения
        const esp_err_t ret = esp_ble_gatts_send_response(
            mGattsIf,
            connId,
            param->write.trans_id,
            ESP_GATT_OK,
            nullptr
        );

        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Response failed. Conn: %u, Error: %s",
                     connId, esp_err_to_name(ret));
        }
    }
} // namespace net
