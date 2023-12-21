#ifndef EEPROM
#define EEPROM

#include <stdio.h>

/*   I2C   */
#define I2C0_SDA_PIN 16
#define I2C0_SCL_PIN 17
#define DEVADDR 0x50
#define BAUDRATE 100000
#define I2C_MEMORY_SIZE 32768
#define MAX_LOG_SIZE 64
#define MAX_LOG_ENTRY 32

/////////////////////////////////////////////////////
//             FUNCTION DECLARATIONS               //
/////////////////////////////////////////////////////

void i2cInit();
void i2cWriteByte(uint16_t address, uint8_t data);
uint8_t i2cReadByte(uint16_t address);
uint16_t crc16(const uint8_t *data_p, size_t length);
void writeLogEntry(const char *message);
void printLog();
void eraseLog();
void printAllMemory();
void eraseAll();

#endif
