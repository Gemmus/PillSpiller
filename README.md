# PillSpiller
Second year embedded project programmed to dispense pills using Raspberry Pi Pico WH.
The program is implemented in the C language and built in the Pico SDK environment.

## Main parts of the device
![image](https://github.com/Gemmus/PillSpiller/assets/112064697/641524c7-6315-4df3-8b06-687cce7da123)
<br>
Figure 1: Main parts of the pill dispenser device.

## Project goal
The goal of the project is to implement an automated pill dispenser that dispenses daily medicine to
a patient. The dispenser has eight compartments. One of them is used for calibrating the dispenser
wheel position leaving the other seven for pills. Dispenser wheel will turn daily at a fixed time to
dispense the daily medicine and the piezoelectric sensor will be used to confirm that pill(s) were
dispensed. The dispenser state is stored in non-volatile memory so that the state of the device
(number of pills left, daily dispensing log, etc) will persist over restarts. The state of the device is
communicated to a server using LoRaWAN network.
For testing purposes, the time between dispensing pills is reduced to 30 seconds. 
