# PillSpiller
Embedded project programmed to dispense pills using Raspberry Pi Pico WH.


## Project goal
The goal of the project is to implement an automated pill dispenser that dispenses daily medicine to
a patient. The dispenser has eight compartments. One of them is used for calibrating the dispenser
wheel position leaving the other seven for pills. Dispenser wheel will turn daily at a fixed time to
dispense the daily medicine and the piezoelectric sensor will be used to confirm that pill(s) were
dispensed. The dispenser state is stored in non-volatile memory so that the state of the device
(number of pills left, daily dispensing log, etc) will persist over restarts. The state of the device is
communicated to a server using LoRaWAN network.
For testing purposes, the time between dispensing pills is reduced to 30 seconds. 
