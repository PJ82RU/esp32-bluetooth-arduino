#include "bluetooth_low_energy.h"
#include <BLEDevice.h>
#include <BLE2902.h>

namespace hardware
{
    void BluetoothLowEnergy::onResponse(void* value, void* params) noexcept
    {
        if (value && params)
        {
            const auto* ble = static_cast<BluetoothLowEnergy*>(params);
            auto* packet = static_cast<Packet*>(value);
            ble->characteristicSetValue(*packet);
        }
    }

    BluetoothLowEnergy::BluetoothLowEnergy() noexcept
        : mSemaphore(false)
    {
    }

    BluetoothLowEnergy::~BluetoothLowEnergy()
    {
        end();
    }

    bool BluetoothLowEnergy::begin(const char* name,
                                   const char* serviceUuid,
                                   const char* characteristicUuid,
                                   pj_tools::Callback* callback) noexcept
    {
        if (!name || !serviceUuid || !characteristicUuid)
        {
            log_e("Invalid parameters");
            return false;
        }

        if (!mSemaphore.take()) return false;

        bool result = false;
        do
        {
            if (mServer)
            {
                log_w("Server already running");
                break;
            }

            // Инициализация устройства
            BLEDevice::init(name);
            log_i("Device initialized: %s", name);

            // Создание сервера
            mServer = BLEDevice::createServer();
            if (!mServer)
            {
                log_e("Server creation failed");
                BLEDevice::deinit(true);
                break;
            }

            mServerCallbacks = new(std::nothrow) BluetoothServerCallbacks();
            if (!mServerCallbacks)
            {
                log_e("Failed to allocate server callbacks");
                cleanupResources();
                break;
            }

            mServer->setCallbacks(mServerCallbacks);
            log_i("Server created");

            // Создание сервиса
            mService = mServer->createService(serviceUuid);
            if (!mService)
            {
                log_e("Service creation failed");
                cleanupResources();
                break;
            }
            log_i("Service created: %s", serviceUuid);

            // Создание характеристики
            mCharacteristic = mService->createCharacteristic(
                characteristicUuid,
                BLECharacteristic::PROPERTY_READ |
                BLECharacteristic::PROPERTY_WRITE |
                BLECharacteristic::PROPERTY_NOTIFY |
                BLECharacteristic::PROPERTY_INDICATE);

            if (!mCharacteristic)
            {
                log_e("Characteristic creation failed");
                cleanupResources();
                break;
            }
            log_i("Characteristic created: %s", characteristicUuid);

            // Настройка дескриптора
            mCharacteristic->addDescriptor(new(std::nothrow) BLE2902());
            mCharCallbacks = new(std::nothrow) BluetoothCharacteristicCallbacks();
            if (!mCharCallbacks)
            {
                log_e("Failed to allocate char callbacks");
                cleanupResources();
                break;
            }

            mCharCallbacks->callback = callback;
            mCharacteristic->setCallbacks(mCharCallbacks);
            log_i("Descriptor added");

            // Запуск сервиса
            mService->start();
            log_i("Service started");

            // Настройка и запуск рекламы
            BLEAdvertising* advertising = BLEDevice::getAdvertising();
            if (!advertising)
            {
                log_e("Failed to get advertising");
                cleanupResources();
                break;
            }

            advertising->addServiceUUID(serviceUuid);
            advertising->setScanResponse(false);
            advertising->setMinPreferred(0x0);
            BLEDevice::startAdvertising();

            result = true;
            log_i("Advertising started");
        }
        while (false);

        (void)mSemaphore.give();
        return result;
    }

    void BluetoothLowEnergy::end() noexcept
    {
        if (!mSemaphore.take()) return;

        if (mServer)
        {
            BLEDevice::stopAdvertising();
            if (mService)
            {
                mService->stop();
            }
            BLEDevice::deinit(true);
            cleanupResources();
            log_i("Server stopped");
        }

        (void)mSemaphore.give();
    }

    void BluetoothLowEnergy::cleanupResources() noexcept
    {
        delete mServerCallbacks;
        mServerCallbacks = nullptr;

        delete mCharCallbacks;
        mCharCallbacks = nullptr;

        mServer = nullptr;
        mService = nullptr;
        mCharacteristic = nullptr;
    }

    BLEServer* BluetoothLowEnergy::server() const noexcept
    {
        return mServer;
    }

    BLEService* BluetoothLowEnergy::service() const noexcept
    {
        return mService;
    }

    BLECharacteristic* BluetoothLowEnergy::characteristic() const noexcept
    {
        return mCharacteristic;
    }

    uint8_t BluetoothLowEnergy::deviceConnected() const noexcept
    {
        return mServerCallbacks ? mServerCallbacks->connectedDevices : 0;
    }

    void BluetoothLowEnergy::characteristicSetValue(Packet& packet) const noexcept
    {
        if (!mCharacteristic || packet.size == 0) return;

        log_d("Sending packet, size: %u", packet.size);
        mCharacteristic->setValue(packet.data, packet.size);
        mCharacteristic->notify();
    }

    bool BluetoothLowEnergy::send(Packet& packet) const noexcept
    {
        if (packet.size == 0)
        {
            log_w("Empty packet");
            return false;
        }

        if (!mSemaphore.take())
        {
            return false;
        }

        bool result = false;
        if (deviceConnected() != 0)
        {
            characteristicSetValue(packet);
            result = true;
        }
        else
        {
            log_w("No connected devices");
        }

        (void)mSemaphore.give();
        return result;
    }

    bool BluetoothLowEnergy::receive(Packet& packet) const noexcept
    {
        packet.size = 0;
        if (deviceConnected() == 0)
        {
            log_w("No connected devices");
            return false;
        }

        if (!mCharCallbacks->callback->read(&packet) || packet.size == 0)
        {
            log_w("No data received");
            return false;
        }
        return true;
    }
} // namespace hardware
