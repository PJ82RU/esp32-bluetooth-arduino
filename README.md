# ESP32 Bluetooth for Arduino



### Создание объекта

```c++
hardware::BluetoothLowEnergy ble;
```



### Пример инициализации

```c++
ble.callback.init(1);
ble.callback.set(on_bluetooth_receive);
ble.begin(BTS_NAME, BLUETOOTH_SERVICE_UUID, BLUETOOTH_CHARACTERISTIC_UUID);
```



### Пример функции обратного вызова

```c++
bool on_bluetooth_receive(void *p_value, void *p_parameters) {
    auto frame = (hardware::net_frame_t *) p_value;
    Serial.printf("In id = 0x%02x, size = %zu\n", frame->value.id, frame->value.size);
    return false;
}
```
