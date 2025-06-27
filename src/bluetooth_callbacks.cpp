#include "bluetooth_callbacks.h"
#include <esp_log.h>

namespace hardware
{
    void BluetoothServerCallbacks::onConnect(BLEServer* server)
    {
        if (server)
        {
            connected_devices++;
            ESP_LOGI("BLE", "Device connected. Count: %d", connected_devices);
        }
    }

    void BluetoothServerCallbacks::onDisconnect(BLEServer* server)
    {
        if (!server) return;

        connected_devices = (connected_devices > 0) ? connected_devices - 1 : 0;
        ESP_LOGI("BLE", "Device disconnected. Count: %d", connected_devices);

        if (connected_devices == 0)
        {
            // Краткая пауза перед перезапуском рекламы
            delay(500);
            server->startAdvertising();
            ESP_LOGI("BLE", "Restarted advertising");
        }
    }

    void BluetoothCharacteristicCallbacks::onWrite(BLECharacteristic* characteristic)
    {
        if (!characteristic || !callback)
        {
            ESP_LOGW("BLE", "Invalid characteristic or callback");
            return;
        }

        const std::string& value = characteristic->getValue();
        const size_t size = value.size();

        if (size == 0 || size > BLE_PACKET_DATA_SIZE)
        {
            ESP_LOGW("BLE", "Invalid data size: %zu (max %zu)", size, BLE_PACKET_DATA_SIZE);
            return;
        }

        Packet packet{};
        packet.size = static_cast<uint16_t>(size);
        std::memcpy(packet.data, value.data(), size);

        callback->trigger(&packet);
        ESP_LOGD("BLE", "Received data: %zu bytes", size);
    }
}
