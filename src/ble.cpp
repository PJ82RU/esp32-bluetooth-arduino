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
    BLE::BLE(const std::shared_ptr<BleConfig>& config) :
        mConfig(config ? config : std::make_shared<BleConfig>())
    {
        sBLEInstance = this;
        if (!config) mConfig->applyPreset(BleConfig::Preset::HIGH_POWER);
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
        esp_err_t ret = esp_bt_controller_init(&mConfig->controller);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Controller init failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ret = esp_bt_controller_enable(static_cast<esp_bt_mode_t>(mConfig->controller.bluetooth_mode));
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

        esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &mConfig->security.authReq, sizeof(uint8_t));
        esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &mConfig->security.ioCap, sizeof(uint8_t));
        esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &mConfig->security.keySize, sizeof(uint8_t));
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &mConfig->security.initKey, sizeof(uint8_t));
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &mConfig->security.rspKey, sizeof(uint8_t));

        ret = esp_ble_gatts_app_register(mConfig->gatt.appId);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "GATTS app register failed: %s", esp_err_to_name(ret));
            return ret;
        }

        // Устанавливаем предпочтительные параметры PHY по умолчанию
        ret = setPreferredPhy(mConfig->connection.txPhy, mConfig->connection.rxPhy);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set preferred PHY: %s (0x%04X)",
                     esp_err_to_name(ret), ret);
            return ret;
        }

        mIsInitialized = true;
        ESP_LOGI(TAG, "BLE 5.0 initialized successfully. Device name: %s", deviceName.c_str());
        return ESP_OK;
    }

    esp_err_t BLE::quickStart(const std::string& deviceName,
                              std::unique_ptr<esp32_c3::objects::Callback> dataCallback)
    {
        // Инициализация BLE стека
        esp_err_t ret = initialize(deviceName, std::move(dataCallback));
        if (ret != ESP_OK)
        {
            return ret;
        }

        // Создание сервиса
        const esp_bt_uuid_t mServiceUuid = uuidFromString(mConfig->gatt.serviceUuid);
        ret = createService(mServiceUuid);
        if (ret != ESP_OK)
        {
            stop();
            return ret;
        }

        // Создание характеристики с возможностью чтения и записи
        const esp_bt_uuid_t mCharUuid = uuidFromString(mConfig->gatt.charUuid);
        ret = createCharacteristic(mCharUuid, mConfig->gatt.charProperties);
        if (ret != ESP_OK)
        {
            stop();
            return ret;
        }

        // Запуск рекламы
        ret = startAdvertising();
        if (ret != ESP_OK)
        {
            stop();
            return ret;
        }

        ESP_LOGI(TAG, "Quick start completed. Service: %s, Char: %s",
                 mConfig->gatt.serviceUuid.c_str(), mConfig->gatt.charUuid.c_str());
        return ESP_OK;
    }

    esp_bt_uuid_t BLE::uuidFromString(const std::string& uuidStr)
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
                std::string byteStr = normalized.substr(i * 2, 2);
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
            mServiceHandle,                // Хэндл сервиса
            &charUuidCopy,                 // Копия UUID характеристики
            mConfig->gatt.charPermissions, // Права доступа
            properties,                    // Свойства характеристики
            &charValue,                    // Значение характеристики
            &control                       // Управление атрибутом
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

        return configureExtendedAdvertising();
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

    esp_err_t BLE::sendData(uint16_t connId,
                            const uint8_t* data,
                            const size_t length,
                            const bool useFastPhy) const
    {
        std::lock_guard lock(mMutex);

        if (!mIsInitialized || mCharHandle == 0)
        {
            ESP_LOGE(TAG, "BLE not initialized or characteristic not created");
            return ESP_ERR_INVALID_STATE;
        }

        if (!data || length == 0 || length > mMtu)
        {
            ESP_LOGE(TAG, "Invalid data (len=%zu, max=%d)", length, mMtu);
            return ESP_ERR_INVALID_ARG;
        }

        // Устанавливаем PHY для соединения, если требуется
        if (useFastPhy && connId != 0)
        {
            // Получаем MAC-адрес из сохраненных подключений
            const auto it = std::ranges::find_if(mActiveConnections,
                                                 [connId](const auto& conn) { return conn.connId == connId; });

            if (it != mActiveConnections.end())
            {
                const esp_err_t phyRet = esp_ble_gap_set_preferred_phy(
                    const_cast<uint8_t*>(it->address),
                    0x03, // ESP_BLE_GAP_ALL_PHYS_PREF
                    ESP_BLE_GAP_PHY_2M,
                    ESP_BLE_GAP_PHY_2M,
                    0 // ESP_BLE_GAP_PHY_OPTION_NO_PREF
                );

                if (phyRet != ESP_OK)
                {
                    ESP_LOGW(TAG, "Failed to set 2M PHY for conn %d: %s",
                             connId, esp_err_to_name(phyRet));
                }
            }
        }

        if (connId == 0)
        {
            if (mActiveConnections.empty())
            {
                ESP_LOGW(TAG, "No active connections for broadcast");
                return ESP_ERR_NOT_FOUND;
            }

            esp_err_t finalRet = ESP_OK;
            for (const auto& [connId, address] : mActiveConnections)
            {
                const esp_err_t ret = esp_ble_gatts_send_indicate(
                    mGattsIf, connId, mCharHandle, length, const_cast<uint8_t*>(data), false);

                if (ret != ESP_OK)
                {
                    ESP_LOGE(TAG, "Failed to send to conn %d: %s", connId, esp_err_to_name(ret));
                    finalRet = ret;
                }
            }
            return finalRet;
        }

        // Отправка конкретному устройству
        const esp_err_t ret = esp_ble_gatts_send_indicate(
            mGattsIf, connId, mCharHandle, length, const_cast<uint8_t*>(data), false);

        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send to conn %d: %s", connId, esp_err_to_name(ret));
        }
        return ret;
    }

    esp_err_t BLE::sendPacket(const Packet& packet, const bool useFastPhy) const
    {
        return sendData(packet.id, packet.data.data(), packet.size, useFastPhy);
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
        return mConfig;
    }

    esp_err_t BLE::updateConfig(const std::shared_ptr<BleConfig>& newConfig)
    {
        if (!newConfig)
        {
            ESP_LOGE(TAG, "Invalid nullptr config");
            return ESP_ERR_INVALID_ARG;
        }

        std::lock_guard lock(mMutex);

        if (mIsInitialized)
        {
            ESP_LOGW(TAG, "Cannot update config after initialization");
            return ESP_ERR_INVALID_STATE;
        }

        mConfig = newConfig;
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

    esp_err_t BLE::configureExtendedAdvertising()
    {
        // 1. Устанавливаем параметры расширенной рекламы
        esp_err_t ret = esp_ble_gap_ext_adv_set_params(0, &mConfig->advertising);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Set extended adv params failed: %s", esp_err_to_name(ret));
            return ret;
        }

        // 2. Подготавливаем данные рекламы с реальным именем устройства
        std::vector<uint8_t> advData;

        // Добавляем флаги
        advData.push_back(0x02); // длина
        advData.push_back(ESP_BLE_AD_TYPE_FLAG);
        advData.push_back(mConfig->advertisingParams.flags);

        // Добавляем имя устройства
        const uint8_t nameLen = static_cast<uint8_t>(std::min(mDeviceName.length(),
                                                              static_cast<size_t>(ESP_BLE_ADV_DATA_LEN_MAX - 2)));
        advData.push_back(nameLen + 1); // длина
        advData.push_back(ESP_BLE_AD_TYPE_NAME_CMPL);
        advData.insert(advData.end(), mDeviceName.begin(), mDeviceName.begin() + nameLen);

        // Устанавливаем данные рекламы
        ret = esp_ble_gap_config_ext_adv_data_raw(0, advData.size(), advData.data());
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Config adv data raw failed: %s", esp_err_to_name(ret));
            return ret;
        }

        // 3. Начинаем расширенную рекламу
        constexpr esp_ble_gap_ext_adv_t extAdv = {
            .instance = 0,
            .duration = 0,
            .max_events = 0
        };

        ret = esp_ble_gap_ext_adv_start(1, &extAdv);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Start extended adv failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ESP_LOGI(TAG, "Extended advertising started successfully");
        return ESP_OK;
    }

    void BLE::handleWriteEvent(const uint16_t connId, const esp_ble_gatts_cb_param_t* param) const
    {
        std::lock_guard lock(mMutex);

        if (!param || !mDataCallback)
        {
            ESP_LOGE(TAG, "Invalid write event or callback. Conn_id: %d", connId);
            return;
        }

        if (param->write.handle != mCharHandle)
        {
            ESP_LOGW(TAG, "Write to unknown handle: %d (expected: %d). Conn_id: %d",
                     param->write.handle, mCharHandle, connId);
            return;
        }

        if (param->write.len == 0 || param->write.len > PACKET_DATA_SIZE)
        {
            ESP_LOGW(TAG, "Invalid data size: %d (max %zu). Conn_id: %d",
                     param->write.len, PACKET_DATA_SIZE, connId);
            return;
        }

        // Создаем пакет с учетом connection ID
        Packet packet{};
        packet.id = connId; // Сохраняем идентификатор соединения
        if (!packet.setPayload(param->write.value, param->write.len))
        {
            ESP_LOGE(TAG, "Failed to set payload. Conn_id: %d, Size: %d",
                     connId, param->write.len);
            return;
        }

        // Передаем в callback с информацией о соединении
        mDataCallback->invoke(&packet);

        ESP_LOGD(TAG, "Received %d bytes from connection: %d", packet.size, connId);

        // Отправляем подтверждение (если требуется)
        const esp_err_t ret = esp_ble_gatts_send_response(
            mGattsIf,
            connId,
            param->write.trans_id,
            ESP_GATT_OK,
            nullptr
        );

        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send response. Conn_id: %d, Error: %s",
                     connId, esp_err_to_name(ret));
        }
    }
} // namespace net
