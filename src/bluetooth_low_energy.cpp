#include "bluetooth_low_energy.h"
#include <BLEDevice.h>
#include <BLE2902.h>

namespace hardware
{
    void BluetoothLowEnergy::on_response(void* value, void* params) noexcept
    {
        if (value && params)
        {
            const auto* ble = static_cast<BluetoothLowEnergy*>(params);
            auto* packet = static_cast<Packet*>(value);
            ble->characteristic_set_value(*packet);
        }
    }

    BluetoothLowEnergy::BluetoothLowEnergy() noexcept
        : semaphore_(false)
    {
    }

    BluetoothLowEnergy::~BluetoothLowEnergy()
    {
        end();
    }

    bool BluetoothLowEnergy::begin(const char* name, const char* service_uuid, const char* characteristic_uuid,
                                   tools::Callback* callback) noexcept
    {
        if (!name || !service_uuid || !characteristic_uuid)
        {
            ESP_LOGE("BLE", "Invalid parameters");
            return false;
        }

        if (!semaphore_.take()) return false;

        bool result = false;
        do
        {
            if (server_)
            {
                ESP_LOGW("BLE", "Server already running");
                break;
            }

            // Инициализация устройства
            BLEDevice::init(name);
            ESP_LOGI("BLE", "Device initialized: %s", name);

            // Создание сервера
            server_ = BLEDevice::createServer();
            if (!server_)
            {
                ESP_LOGE("BLE", "Server creation failed");
                BLEDevice::deinit(true);
                break;
            }

            server_callbacks_ = new(std::nothrow) BluetoothServerCallbacks();
            if (!server_callbacks_)
            {
                ESP_LOGE("BLE", "Failed to allocate server callbacks");
                cleanup_resources();
                break;
            }

            server_->setCallbacks(server_callbacks_);
            ESP_LOGI("BLE", "Server created");

            // Создание сервиса
            service_ = server_->createService(service_uuid);
            if (!service_)
            {
                ESP_LOGE("BLE", "Service creation failed");
                cleanup_resources();
                break;
            }
            ESP_LOGI("BLE", "Service created: %s", service_uuid);

            // Создание характеристики
            characteristic_ = service_->createCharacteristic(
                characteristic_uuid,
                BLECharacteristic::PROPERTY_READ |
                BLECharacteristic::PROPERTY_WRITE |
                BLECharacteristic::PROPERTY_NOTIFY |
                BLECharacteristic::PROPERTY_INDICATE);

            if (!characteristic_)
            {
                ESP_LOGE("BLE", "Characteristic creation failed");
                cleanup_resources();
                break;
            }
            ESP_LOGI("BLE", "Characteristic created: %s", characteristic_uuid);

            // Настройка дескриптора
            characteristic_->addDescriptor(new(std::nothrow) BLE2902());
            char_callbacks_ = new(std::nothrow) BluetoothCharacteristicCallbacks();
            if (!char_callbacks_)
            {
                ESP_LOGE("BLE", "Failed to allocate char callbacks");
                cleanup_resources();
                break;
            }

            char_callbacks_->callback = callback;
            characteristic_->setCallbacks(char_callbacks_);
            ESP_LOGI("BLE", "Descriptor added");

            // Запуск сервиса
            service_->start();
            ESP_LOGI("BLE", "Service started");

            // Настройка и запуск рекламы
            BLEAdvertising* advertising = BLEDevice::getAdvertising();
            if (!advertising)
            {
                ESP_LOGE("BLE", "Failed to get advertising");
                cleanup_resources();
                break;
            }

            advertising->addServiceUUID(service_uuid);
            advertising->setScanResponse(false);
            advertising->setMinPreferred(0x0);
            BLEDevice::startAdvertising();

            result = true;
            ESP_LOGI("BLE", "Advertising started");
        }
        while (false);

        (void)semaphore_.give();
        return result;
    }

    void BluetoothLowEnergy::end() noexcept
    {
        if (!semaphore_.take()) return;

        if (server_)
        {
            BLEDevice::stopAdvertising();
            if (service_)
            {
                service_->stop();
            }
            BLEDevice::deinit(true);
            cleanup_resources();
            ESP_LOGI("BLE", "Server stopped");
        }

        (void)semaphore_.give();
    }

    void BluetoothLowEnergy::cleanup_resources() noexcept
    {
        delete server_callbacks_;
        server_callbacks_ = nullptr;

        delete char_callbacks_;
        char_callbacks_ = nullptr;

        server_ = nullptr;
        service_ = nullptr;
        characteristic_ = nullptr;
    }

    BLEServer* BluetoothLowEnergy::server() const noexcept
    {
        return server_;
    }

    BLEService* BluetoothLowEnergy::service() const noexcept
    {
        return service_;
    }

    BLECharacteristic* BluetoothLowEnergy::characteristic() const noexcept
    {
        return characteristic_;
    }

    uint8_t BluetoothLowEnergy::device_connected() const noexcept
    {
        return server_callbacks_ ? server_callbacks_->connected_devices : 0;
    }

    void BluetoothLowEnergy::characteristic_set_value(Packet& packet) const noexcept
    {
        if (!characteristic_ || packet.size == 0) return;

        ESP_LOGD("BLE", "Sending packet, size: %u", packet.size);
        characteristic_->setValue(packet.data, packet.size);
        characteristic_->notify();
    }

    bool BluetoothLowEnergy::send(Packet& packet) const noexcept
    {
        if (packet.size == 0)
        {
            ESP_LOGW("BLE", "Empty packet");
            return false;
        }

        if (!semaphore_.take())
        {
            return false;
        }

        bool result = false;
        if (device_connected() != 0)
        {
            characteristic_set_value(packet);
            result = true;
        }
        else
        {
            ESP_LOGW("BLE", "No connected devices");
        }

        (void)semaphore_.give();
        return result;
    }

    bool BluetoothLowEnergy::receive(Packet& packet) const noexcept
    {
        packet.size = 0;
        if (device_connected() == 0)
        {
            ESP_LOGW("BLE", "No connected devices");
            return false;
        }

        if (!char_callbacks_->callback->read(&packet) || packet.size == 0)
        {
            ESP_LOGW("BLE", "No data received");
            return false;
        }
        return true;
    }
}
