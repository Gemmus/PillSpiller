#include "eeprom.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <string.h>

#ifndef DEBUG_PRINT
#define DBG_PRINT(f_, ...)  printf((f_), ##__VA_ARGS__)
#else
#define DBG_PRINT(f_, ...)
#endif

void i2cInit() {
    i2c_init(i2c0, BAUDRATE);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
}

void i2cWriteByte(uint16_t address, uint8_t data) {
    uint8_t buffer[3];
    buffer[0] = address >> 8; buffer[1] = address; buffer[2] = data;
    i2c_write_blocking(i2c0, DEVADDR, buffer, sizeof(buffer), false);
    sleep_ms(10);
}

uint8_t i2cReadByte(uint16_t address) {
    uint8_t buffer[2];
    buffer[0] = address >> 8; buffer[1] = address;
    i2c_write_blocking(i2c0, DEVADDR, buffer, 2, true);
    i2c_read_blocking(i2c0, DEVADDR, buffer, 1, false);
    return buffer[0];
}

uint16_t crc16(const uint8_t *data_p, size_t length) {
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (length--) {
        x = crc >> 8 ^ *data_p++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t) (x << 12)) ^ ((uint16_t) (x << 5)) ^ ((uint16_t) (x));
    }
    return crc;
}

void writeInt(uint16_t address, int32_t data) {
    uint8_t buffer[6];

    memcpy(buffer, &data, sizeof(data));

    uint16_t crc = crc16(buffer, sizeof(data));
    buffer[sizeof(data) + 1] = (uint8_t)(crc >> 8);
    buffer[sizeof(data)] = (uint8_t)crc;

    uint16_t log_address = address;
    for (int i = 0; i < (sizeof(data) + 2); i++) {
        i2cWriteByte(log_address++, buffer[i]);
    }
}

int32_t readInt(uint16_t address) {
    uint8_t buffer[MAX_LOG_SIZE / 4];
    for (int i = 0; i < sizeof(int32_t) + 2; i++) {
        buffer[i] = i2cReadByte(address++);
    }

    uint16_t *read_crc16 = (uint16_t *) &buffer[sizeof(int32_t)];
    uint16_t calc_crc16 = crc16(buffer, sizeof(int32_t));

    if (*read_crc16 == calc_crc16) {
        int32_t data;
        memcpy(&data, buffer, sizeof(data));
        return data;
    } else {
        return 0;
    }
}

void eraseLog() {
    DBG_PRINT("Erasing log messages from memory... ");
    uint16_t log_address = 0;
    for(int i = 0; i < MAX_LOG_ENTRY; i++) {
        i2cWriteByte(log_address, 0);
        log_address += MAX_LOG_SIZE;
    }
    DBG_PRINT(" done.\n\n");
}

void eraseAll(){
    DBG_PRINT("Erasing all from memory... ");
    uint16_t log_address = 0;
    for(int i = 0; i < MAX_LOG_SIZE * MAX_LOG_ENTRY; i++) {
        i2cWriteByte(log_address, 0xFF);
        log_address++;
    }
    DBG_PRINT(" done.\n");
}

void printAllMemory() {
    DBG_PRINT("\n");
    for (int i = 0; i < MAX_LOG_ENTRY; i++) {
        for (int j = 0; j < MAX_LOG_SIZE; j++) {
            uint8_t printed = i2cReadByte(i * MAX_LOG_SIZE + j);
            DBG_PRINT("%x ", printed);
        }
        DBG_PRINT("\n");
    }
    DBG_PRINT("\n");
}
