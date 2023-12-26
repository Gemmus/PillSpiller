#include "eeprom.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <string.h>

#ifndef DEBUG_PRINT
#define DBG_PRINT(f_, ...)  printf((f_), ##__VA_ARGS__)
#else
#define DBG_PRINT(f_, ...)
#endif

extern int *log_counter;

void i2cInit() {
    i2c_init(i2c0, BAUDRATE);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
}

void i2cWriteBytes(uint16_t address, const uint8_t *data, uint8_t length) {
    assert(data != NULL);
    assert(address < I2C_MEM_SIZE);
    assert(0 < length);
    assert(length <= I2C_MEM_PAGE_SIZE);
    assert((address / I2C_MEM_PAGE_SIZE) == ((address + length - 1) / I2C_MEM_PAGE_SIZE));

    uint8_t buffer[length+2];
    buffer[0] = address >> 8; buffer[1] = address;
    memcpy( &buffer[2], data, length);
    i2c_write_blocking(i2c0, DEVADDR, buffer, sizeof(buffer), false);
    sleep_ms(10);
    //DBG_PRINT("Writing to address %d data %d\n", address, data);
}

void i2cWriteByte_NoDelay(uint16_t address, uint8_t data) {
    assert(address < I2C_MEM_SIZE);

    uint8_t buffer[3];
    buffer[0] = address >> 8; buffer[1] = address; buffer[2] = data;
    i2c_write_blocking(i2c0, DEVADDR, buffer, sizeof(buffer), false);
    //DBG_PRINT("Writing to address %d data %d\n", address, data);
}

void i2cWriteByte(uint16_t address, uint8_t data) {
    assert(address < I2C_MEM_SIZE);

    uint8_t buffer[3];
    buffer[0] = address >> 8; buffer[1] = address; buffer[2] = data;
    i2c_write_blocking(i2c0, DEVADDR, buffer, sizeof(buffer), false);
    sleep_ms(10);
    //DBG_PRINT("Writing to address %d data %d\n", address, data);
}

uint8_t i2cReadByte(uint16_t address) {
    assert(address < I2C_MEM_SIZE);

    uint8_t buffer[2];
    buffer[0] = address >> 8; buffer[1] = address;
    i2c_write_blocking(i2c0, DEVADDR, buffer, 2, true);
    i2c_read_blocking(i2c0, DEVADDR, buffer, 1, false);
    //DBG_PRINT("Reading from address %d data %d\n", address, buffer[0]);
    return buffer[0];
}

void i2cReadBytes(uint16_t address, uint8_t *data, uint8_t length) {
    assert(data != NULL);
    assert(address < I2C_MEM_SIZE);
    assert(0 < length);

    uint8_t buffer[2];
    buffer[0] = address >> 8; buffer[1] = address;
    i2c_write_blocking(i2c0, DEVADDR, buffer, 2, true);
    i2c_read_blocking(i2c0, DEVADDR, data, length, false);
    //DBG_PRINT("Reading from address %d data %d\n", address, buffer[0]);
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

void writeStruct(const machineState *state) {
    machineState stateToWrite = *state;

    uint16_t crc = crc16((uint8_t *) &stateToWrite, sizeof(stateToWrite)-sizeof(stateToWrite.crc16));
    stateToWrite.crc16 = crc;

    uint16_t write_address = I2C_MEM_SIZE - sizeof(stateToWrite);
    uint8_t *buffer = (uint8_t *) &stateToWrite;
    i2cWriteBytes(write_address, buffer, sizeof(stateToWrite));
//    for (int i = 0; i < sizeof(stateToWrite); i++) {
//        i2cWriteByte(write_address++, buffer[i]);
//    }
}

bool readStruct(machineState *state) {
    machineState stateToRead;
    uint16_t read_address = I2C_MEM_SIZE - sizeof(stateToRead);
    i2cReadBytes(read_address, (uint8_t *) &stateToRead, sizeof(stateToRead));
//    uint8_t *buffer = (uint8_t *) &stateToRead;
//    for (int i = 0; i < sizeof(stateToRead); i++) {
//        buffer[i] = i2cReadByte(read_address++);
//    }

    uint16_t calc_crc16 = crc16((uint8_t *) &stateToRead, sizeof(stateToRead)-sizeof(stateToRead.crc16));

    if (stateToRead.crc16 == calc_crc16) {
        memcpy(state, &stateToRead, sizeof(stateToRead));
        return true;
    } else {
        return false;
    }
}

void writeLogEntry(const char *message) {
    if (*log_counter >= MAX_LOG_ENTRY) {
        printf("Maximum allowed log number reached. ");
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

        uint16_t log_address = MAX_LOG_SIZE * *log_counter;
        *log_counter = *log_counter + 1;
        i2cWriteBytes(log_address, buffer, message_length+3);
//        for(int i = 0; i < (message_length + 3); i++) {
//            i2cWriteByte(log_address++, buffer[i]);
//        }
        //printLog();
    } else {
        printf("Invalid input. Log message must contain at least one character.\n");
    }
}

void printLog() {
    if (0 != *log_counter) {
        uint16_t log_address = MEM_ADDR_START;
        uint8_t buffer[MAX_LOG_SIZE];

        printf("Printing log messages from memory:\n");
        for (int i = 0; i < *log_counter; i++) {
            log_address = i * MAX_LOG_SIZE;
            i2cReadBytes(log_address, buffer, MAX_LOG_SIZE);
//            for (int j = 0; j < MAX_LOG_SIZE; j++) {
//                buffer[j] = i2cReadByte(log_address++);
//            }

            int term_zero_index = 0;
            while (buffer[term_zero_index] != '\0') {
                term_zero_index++;
            }

            if(0 == crc16(buffer, (term_zero_index + 3)) && buffer[0] != 0 && (term_zero_index < (MAX_LOG_SIZE - 2))) {
                printf("Log #%d\n", i + 1);
                int index = 0;
                while (buffer[index]) {
                    printf("%c", buffer[index++]);
                }
                printf("\n");
            } else {
                printf("Log message #%d invalid. Exit printing.\n", i + 1);
                break;
            }
        }
    } else {
        printf("No log message in memory yet.\n\n");
    }
}

void eraseLog() {
    printf("Erasing log messages from memory... ");
    uint16_t log_address = 0;
    for(int i = 0; i < MAX_LOG_ENTRY; i++) {
        i2cWriteByte(log_address, 0);
        log_address += MAX_LOG_SIZE;
    }
    *log_counter = 0;
    printf(" done.\n\n");
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
