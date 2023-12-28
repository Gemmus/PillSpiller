#include "eeprom.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <string.h>

#ifndef DEBUG_PRINT
#define DBG_PRINT(f_, ...)  printf((f_), ##__VA_ARGS__)
#else
#define DBG_PRINT(f_, ...)
#endif

//////////////////////////////////////////////////
//              GLOBAL VARIABLES                //
//////////////////////////////////////////////////
extern int *log_counter;

//////////////////////////////////////////////////
//              EEPROM FUNCTIONS                //
//////////////////////////////////////////////////

/**********************************************************************************************************************
 * \brief: Initializes EEPROM.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void i2cInit() {
    i2c_init(i2c0, BAUDRATE);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
}

/**********************************************************************************************************************
 * \brief: Writes a struct to the designated address of the EEPROM. Applies CRC to data. Uses i2cWriteBytes().
 *
 * \param: 1 param: pointer to struct.
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void writeStruct(const machineState *state) {
    machineState stateToWrite = *state;

    uint16_t crc = crc16((uint8_t *) &stateToWrite, sizeof(stateToWrite)-sizeof(stateToWrite.crc16));
    stateToWrite.crc16 = crc;

    uint16_t write_address = I2C_MEM_SIZE - sizeof(stateToWrite);
    uint8_t *buffer = (uint8_t *) &stateToWrite;
    i2cWriteBytes(write_address, buffer, sizeof(stateToWrite));
}

/**********************************************************************************************************************
 * \brief: Page writes the given length of data to the designated address of the EEPROM with the delay of 10 ms.
 *
 * \param: 3 params: uint16_t address, pointer to uint8_t data and uint8_t length indicating the length of the data.
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
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
}

/**********************************************************************************************************************
 * \brief: Writes to EEPROM one byte at a time without delay.
 *
 * \param: 2 params, uint16_t address to write to and data as uint8_t.
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void i2cWriteByte_NoDelay(uint16_t address, uint8_t data) {
    assert(address < I2C_MEM_SIZE);

    uint8_t buffer[3];
    buffer[0] = address >> 8; buffer[1] = address; buffer[2] = data;
    i2c_write_blocking(i2c0, DEVADDR, buffer, sizeof(buffer), false);
}

/**********************************************************************************************************************
 * \brief: Writes to EEPROM one byte at a time with 10 ms delay.
 *
 * \param: 2 params, uint16_t address to write to and data as uint8_t.
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void i2cWriteByte(uint16_t address, uint8_t data) {
    assert(address < I2C_MEM_SIZE);

    uint8_t buffer[3];
    buffer[0] = address >> 8; buffer[1] = address; buffer[2] = data;
    i2c_write_blocking(i2c0, DEVADDR, buffer, sizeof(buffer), false);
    sleep_ms(10);
}

/**********************************************************************************************************************
 * \brief: Reads from EEPROM one byte at a time.
 *
 * \param: 1 param: uint16_t address to indicate where to read from.
 *
 * \return: uint8_t data
 *
 * \remarks:
 **********************************************************************************************************************/
uint8_t i2cReadByte(uint16_t address) {
    assert(address < I2C_MEM_SIZE);

    uint8_t buffer[2];
    buffer[0] = address >> 8; buffer[1] = address;
    i2c_write_blocking(i2c0, DEVADDR, buffer, 2, true);
    i2c_read_blocking(i2c0, DEVADDR, buffer, 1, false);
    return buffer[0];
}

/**********************************************************************************************************************
 * \brief: Page reads the given length of data from the designated address of the EEPROM.
 *
 * \param: 3 params: uint16_t address to read from, pointer to uint8_t data to read to and the length of the data to be
 *                   read as uint8_t type.
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void i2cReadBytes(uint16_t address, uint8_t *data, uint8_t length) {
    assert(data != NULL);
    assert(address < I2C_MEM_SIZE);
    assert(0 < length);

    uint8_t buffer[2];
    buffer[0] = address >> 8; buffer[1] = address;
    i2c_write_blocking(i2c0, DEVADDR, buffer, 2, true);
    i2c_read_blocking(i2c0, DEVADDR, data, length, false);
}

/**********************************************************************************************************************
 * \brief: Reads a struct to the designated address of the EEPROM. Applies CRC check. Uses i2cReadBytes().
 *
 * \param: 1 param: pointer to struct.
 *
 * \return: boolean, true: if CRC check successful; false: if CRC check fails.
 *
 * \remarks:
 **********************************************************************************************************************/
bool readStruct(machineState *state) {
    machineState stateToRead;
    uint16_t read_address = I2C_MEM_SIZE - sizeof(stateToRead);
    i2cReadBytes(read_address, (uint8_t *) &stateToRead, sizeof(stateToRead));
    uint16_t calc_crc16 = crc16((uint8_t *) &stateToRead, sizeof(stateToRead)-sizeof(stateToRead.crc16));

    if (stateToRead.crc16 == calc_crc16) {
        memcpy(state, &stateToRead, sizeof(stateToRead));
        return true;
    } else {
        return false;
    }
}

/**********************************************************************************************************************
 * \brief: Calculates the crc for passed data.
 *
 * \param: 2 params: pointer to uint8_t data_p and the length of data as size_t length.
 *
 * \return: return crc as type uint16_t
 *
 * \remarks:
 **********************************************************************************************************************/
uint16_t crc16(const uint8_t *data, size_t length) {
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (length--) {
        x = crc >> 8 ^ *data++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t) (x << 12)) ^ ((uint16_t) (x << 5)) ^ ((uint16_t) (x));
    }
    return crc;
}

/**********************************************************************************************************************
 * \brief: Writes the passed log message to the next available log address of the EEPROM. If no log address available,
 *         deletes all the existing log messages by calling eraseLog(), and restarts the logging from first log address.
 *         Applies CRC. For transmission calls i2cWriteBytes().
 *
 * \param: 1 param: pointer to a char message.
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void writeLogEntry(const char *message) {
    if (*log_counter >= MAX_LOG_ENTRY) {
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

        uint16_t log_address = MAX_LOG_SIZE * *log_counter;
        *log_counter = *log_counter + 1;
        i2cWriteBytes(log_address, buffer, message_length+3);
    } else {
        DBG_PRINT("Invalid input. Log message must contain at least one character.\n");
    }
}

/**********************************************************************************************************************
 * \brief: Prints/reads all the existing log messages stored from EEPROM. Calls i2cReadBytes(). Applies CRC check.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void printLog() {
    if (0 != *log_counter) {
        uint16_t log_address = MEM_ADDR_START;
        uint8_t buffer[MAX_LOG_SIZE];

        DBG_PRINT("Printing log messages from memory:\n");
        for (int i = 0; i < *log_counter; i++) {
            log_address = i * MAX_LOG_SIZE;
            i2cReadBytes(log_address, buffer, MAX_LOG_SIZE);

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
        DBG_PRINT("No log message in memory yet.\n");
    }
}

/**********************************************************************************************************************
 * \brief: Erases all 32 log messages from EEPROM by writing a zero to the beginning of each log. Calls i2cWriteByte()
 *         to write 0.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void eraseLog() {
    DBG_PRINT("Erasing log messages from memory... ");
    uint16_t log_address = 0;
    for(int i = 0; i < MAX_LOG_ENTRY; i++) {
        i2cWriteByte(log_address, 0);
        log_address += MAX_LOG_SIZE;
    }
    *log_counter = 0;
    DBG_PRINT(" done.\n");
}

/**********************************************************************************************************************
 * \brief: Erases all the data allocated for log messages from the EEPROM. Calls i2cWriteByte() for every each byte.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void eraseAll(){
    DBG_PRINT("Erasing all from memory... ");
    uint16_t log_address = 0;
    for(int i = 0; i < MAX_LOG_SIZE * MAX_LOG_ENTRY; i++) {
        i2cWriteByte(log_address, 0xFF);
        log_address++;
    }
    DBG_PRINT(" done.\n");
}

/**********************************************************************************************************************
 * \brief: Prints all the data allocated for log messages from the EEPROM. Calls i2cReadByte() for every each byte.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
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
