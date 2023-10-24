#ifndef ESP32_BLUETOOTH_ARDUINO_TYPES_H
#define ESP32_BLUETOOTH_ARDUINO_TYPES_H

#include <Arduino.h>

#define BLUETOOTH_WRITE_SIZE        512
#define BLUETOOTH_FRAME_DATA_SIZE   BLUETOOTH_WRITE_SIZE - 1

#pragma pack(push, 1)
/** Сетевой кадр */
typedef union net_frame_t {
    struct {
        uint8_t id;
        uint8_t data[BLUETOOTH_FRAME_DATA_SIZE];
    } value;
    uint8_t bytes[BLUETOOTH_WRITE_SIZE];
} net_frame_t;

/** Буфер */
typedef struct net_frame_buffer_t {
    bool is_data;
    size_t size;
    net_frame_t frame;
} net_frame_buffer_t;
#pragma pack(pop)

typedef size_t (*ble_receive_t)(uint8_t, uint8_t *, size_t);

#endif //ESP32_BLUETOOTH_ARDUINO_TYPES_H
