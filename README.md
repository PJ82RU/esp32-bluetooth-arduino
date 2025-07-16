# ESP32-C3 BLE
**Гибкий BLE-модуль для ESP32 (ESP-IDF), поддерживающий BLE 5.0 (Extended Advertising) и обратную совместимость с BLE 4.2 (Legacy).**

![Лицензия](https://img.shields.io/badge/license-Unlicense-blue.svg)
![PlatformIO](https://img.shields.io/badge/platform-ESP32--C3-green.svg)
![BLE 5.0](https://img.shields.io/badge/BLE-5.0%2F4.2-brightgreen.svg)  
![Version](https://img.shields.io/badge/version-1.1.0-orange)

---

## **📌 Возможности**
✅ **Поддержка BLE 5.0 + BLE 4.2**
- Расширенная реклама (Extended Advertising) для Android/iOS.
- Legacy-реклама для Web Bluetooth (Chrome).

✅ **Гибкая настройка параметров BLE**
- Пресеты (`BleConfig::Preset`): `BLE5_ULTRA_PERF`, `BLE4_LOW_POWER` и др.
- Ручная настройка PHY, интервалов рекламы, мощности TX.

✅ **Работа с UUID в разных форматах**
- Автоматическая инверсия байт для Web Bluetooth.

✅ **Потокобезопасный API**
- Все методы защищены `std::recursive_mutex`.

---

## **⚙️ Настройка**
### **1. Клонирование и сборка**
```bash
git clone https://github.com/your-repo/esp32-ble-gateway.git
cd esp32-ble-gateway
idf.py set-target esp32c3  # или esp32, esp32s3
idf.py menuconfig          # настройка параметров (UART, BLE режим)
idf.py build flash monitor # сборка и прошивка
```

### **2. Быстрый старт**
```cpp
#include "net/ble.h"

net::BLE ble(net::BleConfig::Preset::BLE5_DEFAULT);

void app_main() {
    ble.quickStart(
        "MyBLEDevice",
        std::make_unique<MyDataCallback>() // Ваш callback для обработки данных
    );
}
```

---

## **📊 Пресеты BLE**
| Пресет                  | Описание                                  |
|-------------------------|------------------------------------------|
| `BLE5_ULTRA_PERF`       | Макс. производительность (PHY 2M, 9dBm)  |
| `BLE5_LOW_POWER`        | Энергосбережение (интервалы 320-640ms)   |
| `BLE4_DEFAULT`          | Стандартные настройки ESP-IDF            |

---

## **🔧 Примеры**
### **1. Настройка BLE 5.0**
```cpp
net::BleConfig config(net::BleConfig::Preset::BLE5_ULTRA_PERF);
config.extAdvParams.tx_power = ESP_PWR_LVL_P9;
ble.updateConfig(config);
```

### **2. Работа с UUID**
```cpp
// Для Chrome (инвертированный UUID)
auto uuid = ble.uuidFromString("6E400001-B5A3...", true);

// Для Android (прямой порядок)
auto uuid = ble.uuidFromString("6E400001-B5A3...", false);
```

---

## **📡 Поддерживаемые клиенты**
| Устройство       | Режим          | UUID          |  
|------------------|---------------|---------------|  
| **Chrome**       | BLE 4.2       | Инвертированный |  
| **Android/iOS**  | BLE 5.0       | Прямой        |  

---

## **📜 Лицензия**
Данная библиотека распространяется как свободное и бесплатное программное обеспечение, переданное в общественное достояние.
Вы можете свободно копировать, изменять, публиковать, использовать, компилировать, продавать или распространять это программное обеспечение в исходном коде или в виде скомпилированного бинарного файла для любых целей, коммерческих или некоммерческих, и любыми средствами.

---
