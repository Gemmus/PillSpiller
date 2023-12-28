# PillSpiller
Second year embedded project programmed to dispense pills using Raspberry Pi Pico WH.
The program is implemented in the C language and built in the Pico SDK environment.

## Device:
![image](https://github.com/Gemmus/PillSpiller/assets/112064697/641524c7-6315-4df3-8b06-687cce7da123)
<br>
<small>_Figure 1: Main parts of the pill dispenser device._</small>

<b>Components:</b>
<ul>
      <li>Raspberry Pi Pico W</li>
      <li> PicoProbe debugger</li>
      <li>LoRaWAN module</li>
      <li>Crowtail - I2C EEPROM</li>
      <li>Stepper motor driver</li>
      <li>Dispenser: base and wheel</li>
      <li>Opto Sensor</li>
      <li>Piezo Sensor</li>
</ul>



## Project goal
The goal of the project is to implement an automated pill dispenser that dispenses daily medicine to
a patient. The dispenser has eight compartments. One of them is used for calibrating the dispenser
wheel position leaving the other seven for pills. Dispenser wheel will turn daily at a fixed time to
dispense the daily medicine and the piezoelectric sensor will be used to confirm that pill(s) were
dispensed. The dispenser state is stored in non-volatile memory so that the state of the device
(number of pills left, daily dispensing log, etc) will persist over restarts. The state of the device is
communicated to a server using LoRaWAN network.
For testing purposes, the time between dispensing pills is reduced to 30 seconds. 

### Minimum requirements
When the dispenser is started it waits for a button to be pressed and blinks an LED while waiting.
When the button is pressed the dispenser is calibrated by turning the wheel at least one full turn and
then stopping the wheel so that the compartment with the sensor opening is aligned with the drop
tube.
After calibration the program keeps the LED on until user presses another button. Then it dispenses
pills every 30 seconds starting from the button press. When the wheel is turned, the piezo sensor is
used to detect if a pill was dispensed or not. If no pill was detected the LED blinks 5 times. When all
pills have been dispensed (wheel turned 7 times) the program starts over (wait for calibration, etc.).

### Advanced requirements
Device remembers the state of the device across boot/power down.
Device connect to LoRaWAN and reports status of the device when there is a status change (boot, pill
dispensed/not dispensed, dispenser empty, power off during turning, etc.)
Device can detect if it was reset / powered off while motor was turning.
Device can recalibrate automatically after power off during middle of a turn without dispensing the
pills that still remain.

## Operation Principle
