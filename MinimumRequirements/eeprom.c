#include "eeprom.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <string.h>

#ifndef DEBUG_PRINT
#define DBG_PRINT(f_, ...)  printf((f_), ##__VA_ARGS__)
#else
#define DBG_PRINT(f_, ...)
#endif

static volatile uint log_counter = 0;

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

void writeLogEntry(const char *message) {
    if (log_counter >= MAX_LOG_ENTRY) {
        DBG_PRINT("Maximum allowed log number reached. ");
        eraseLog();
    }

    size_t message_length = strlen(message);
    if (message_length >= 1) {
        if (message_length > 61) {
            message_length = 61;
        }

        uint8_t buffer[message_length + 3];
        memcpy(buffer, message, message_length);
        buffer[message_length] = '\0';

        uint16_t crc = crc16(buffer, message_length + 1);
        buffer[message_length + 1] = (uint8_t) (crc >> 8);
        buffer[message_length + 2] = (uint8_t) crc;

        uint16_t log_address = MAX_LOG_SIZE * log_counter++;
        for(int i = 0; i < (message_length + 3); i++) {
            i2cWriteByte(log_address++, buffer[i]);
        }
        //printLog();
    } else {
        DBG_PRINT("Invalid input. Log message must contain at least one character.\n");
    }
}

void printLog() {
    if (0 != log_counter) {
        uint16_t log_address = 0;
        uint8_t buffer[MAX_LOG_SIZE];

        DBG_PRINT("Printing log messages from memory:\n");
        for (int i = 0; i < log_counter; i++) {
            log_address = i * MAX_LOG_SIZE;
            for (int j = 0; j < MAX_LOG_SIZE; j++) {
                buffer[j] = i2cReadByte(log_address++);
            }
            /*
            for(int k = 0; k < MAX_LOG_SIZE/2; k += 4)
            {
                DBG_PRINT("%x %x %x %x\n", buffer[k+0], buffer[k+1], buffer[k+2], buffer[k+3]);
            }
            */
            int term_zero_index = 0;
            while (buffer[term_zero_index] != '\0') {
                term_zero_index++;
            }

            if(0 == crc16(buffer, (term_zero_index + 3)) && buffer[0] != 0 && (term_zero_index < (MAX_LOG_SIZE - 2))) {
                DBG_PRINT("Log #%d\n", i + 1);
                int index = 0;
                while (buffer[index]) {
                    DBG_PRINT("%c", buffer[index++]);
                }
                DBG_PRINT("\n");
            } else {
                DBG_PRINT("Log message #%d invalid. Exit printing.\n", i + 1);
                break;
            }
        }
    } else {
        DBG_PRINT("No log message in memory yet.\n\n");
    }
}

void eraseLog() {
    DBG_PRINT("Erasing log messages from memory... ");
    uint16_t log_address = 0;
    for(int i = 0; i < MAX_LOG_ENTRY; i++) {
        i2cWriteByte(log_address, 0);
        log_address += MAX_LOG_SIZE;
    }
    log_counter = 0;
    DBG_PRINT(" done.\n\n");
}

void eraseAll(){
    DBG_PRINT("Erasing all from memory... ");
    uint16_t log_address = 0;
    for(int i = 0; i < MAX_LOG_SIZE * MAX_LOG_ENTRY; i++) {
        i2cWriteByte(log_address, 0xFF);
        log_address++;
    }
    log_counter = 0;
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
