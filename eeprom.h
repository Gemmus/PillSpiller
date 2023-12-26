#ifndef EEPROM
#define EEPROM

#include <stdio.h>
#include <stdbool.h>

/*   I2C   */
#define I2C0_SDA_PIN 16
#define I2C0_SCL_PIN 17
#define DEVADDR 0x50
#define BAUDRATE 100000
#define I2C_MEM_SIZE 32768
#define MEM_ADDR_START 0
#define MAX_LOG_SIZE 64
#define MAX_LOG_ENTRY 32

enum SystemState {
    CALIB_WAITING,       // EEPROM, CALIBRATED: 0 == CALIB_WAITING
    DISPENSE_WAITING     //                     1 == DISPENSE_WAITING
};

enum CompartmentState {
    IN_THE_MIDDLE,      // EEPROM, 0 == IN_THE_MIDDLE
    FINISHED            //         1 == FINISHED
};

typedef struct __attribute__((__packed__)) machineState {
    enum SystemState currentState;
    enum CompartmentState compartmentFinished;
    int calibrationCount;
    int compartmentsMoved;
    uint16_t crc16;
} machineState;

/////////////////////////////////////////////////////
//             FUNCTION DECLARATIONS               //
/////////////////////////////////////////////////////

void i2cInit();
void i2cWriteByte(uint16_t address, uint8_t data);
uint8_t i2cReadByte(uint16_t address);
uint16_t crc16(const uint8_t *data_p, size_t length);
void writeInt(uint16_t address, int32_t data);
int32_t readInt(uint16_t address);
void writeStruct(const machineState *state);
bool readStruct(machineState *state);
void writeLogEntry(const char *message);
void printLog();
void eraseLog();
void printAllMemory();
void eraseAll();

#endif
