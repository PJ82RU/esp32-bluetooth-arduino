#include "bluetooth_callbacks.h"
#include <cstring>
#include <esp_log.h>

namespace hardware
{
    void BluetoothServerCallbacks::onConnect(BLEServer* server)
    {
        if (server)
        {
            connectedDevices++;
            log_i("Device connected. Count: %d", connectedDevices);
        }
    }

    void BluetoothServerCallbacks::onDisconnect(BLEServer* server)
    {
        if (!server) return;

        connectedDevices = (connectedDevices > 0) ? connectedDevices - 1 : 0;
        log_i("Device disconnected. Count: %d", connectedDevices);

        if (connectedDevices == 0)
        {
            // Краткая пауза перед перезапуском рекламы
            delay(500);
            server->startAdvertising();
            log_i("Restarted advertising");
        }
    }

    void BluetoothCharacteristicCallbacks::onWrite(BLECharacteristic* characteristic)
    {
        if (!characteristic || !callback)
        {
            log_w("Invalid characteristic or callback");
            return;
        }

        const std::string& value = characteristic->getValue();
        const size_t size = value.size();

        if (size == 0 || size > PACKET_DATA_SIZE)
        {
            log_w("Invalid data size: %zu (max %zu)", size, PACKET_DATA_SIZE);
            return;
        }

        Packet packet{};
        packet.size = static_cast<uint16_t>(size);
        std::memcpy(packet.data, value.data(), size);

        callback->invoke(&packet);
        log_d("Received data: %zu bytes", size);
    }
} // namespace hardware
