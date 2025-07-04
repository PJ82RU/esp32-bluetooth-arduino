# ESP32-C3 BLE Utilities

![Лицензия](https://img.shields.io/badge/license-Unlicense-blue.svg)
![PlatformIO](https://img.shields.io/badge/platform-ESP32--C3-green.svg)
![Version](https://img.shields.io/badge/version-1.1.0-orange)

Библиотека для работы с Bluetooth Low Energy (BLE) на ESP32-C3.

## Особенности

- Управление подключениями устройств
- Обработка входящих данных через callback-функции
- Отправка структурированных пакетов данных
- Потокобезопасные операции
- Поддержка стандартных UUID сервисов и характеристик

## Установка

### PlatformIO
Добавьте в platformio.ini:
```ini
lib_deps =
    https://github.com/PJ82RU/esp32-bluetooth-arduino.git
```

### Arduino IDE
1. Скачайте ZIP из GitHub
2. Скетч → Подключить библиотеку → Добавить .ZIP библиотеку

## Быстрый старт

```cpp
#include "ble/bluetooth_low_energy.h"

ble::BluetoothLowEnergy ble;

void setup() {
    ble.begin("ESP32-C3",
        "0000180a-0000-1000-8000-00805f9b34fb",
        "00002a57-0000-1000-8000-00805f9b34fb",
        nullptr);
}

void loop() {
    // Пример отправки данных
    Packet packet;
    if(ble.send(packet)) {
        log_d("Данные отправлены");
    }
}
```

## Примеры

1. `basic` - Создание BLE сервера
2. `advanced` - Обмен структурированными пакетами

## Документация

### Класс `BluetoothLowEnergy`
- `begin()` - Инициализация BLE сервера
- `send()` - Отправка пакета данных
- `receive()` - Получение пакета данных
- `deviceConnected()` - Проверка подключенных устройств

### Класс `BluetoothServerCallbacks`
- `onConnect()` - Обработчик подключения
- `onDisconnect()` - Обработчик отключения

### Класс `BluetoothCharacteristicCallbacks`
- `onWrite()` - Обработчик записи данных

## Лицензия

Библиотека распространяется как общественное достояние (Unlicense).