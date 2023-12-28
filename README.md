# PillSpiller
Second year embedded project programmed to dispense pills using Raspberry Pi Pico WH.
The program is implemented in the C language and built in the Pico SDK environment.

## Device
![image](https://github.com/Gemmus/PillSpiller/assets/112064697/6cee24b3-4210-494f-9be9-1fda59376eff)
<br>
<small>_Figure 1: Main parts of the pill dispenser device._</small>

<b>Components:</b>
<ul>
      <li><a href="https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html">Raspberry Pi Pico WH</a></li>
      <li><a href="https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html">Raspberry Pi Debug Probe</a></li>
      <li>LoRaWAN module - <a href="https://media.digikey.com/pdf/Data%20Sheets/Seeed%20Technology/Grove_LoRa-E5_Web.pdf">Grove LoRa-E5</a></li>
      <li>I2C EEPROM - <a href="https://www.elecrow.com/wiki/index.php?title=Crowtail-_I2C_EEPROM">Crowtail</a></li>
      <li>Stepper motor driver - <a href="https://elecrow.com/wiki/index.php?title=ULN2003_Stepper_Motor_Driver">Elecrow ULN2003</a></li>
      <li>Dispenser: base and wheel</li>
      <li>Stepper motor - <a href="https://www.mouser.com/datasheet/2/758/stepd-01-data-sheet-1143075.pdf">28BYJ-48</a></li>
      <li>Optical Sensor</li>
      <li>Piezo Sensor</li>
</ul>



## Project Goal
The goal of the project is to implement an automated pill dispenser that dispenses daily medicine to
a patient. The dispenser has eight compartments. One of them is used for calibrating the dispenser
wheel position leaving the other seven for pills. Dispenser wheel will turn daily at a fixed time to
dispense the daily medicine and the piezoelectric sensor will be used to confirm that pill(s) were
dispensed. The dispenser state is stored in non-volatile memory so that the state of the device
(number of pills left, daily dispensing log, etc) will persist over restarts. The state of the device is
communicated to a server using LoRaWAN network.
For testing purposes, the time between dispensing pills is reduced to 30 seconds. 

### Minimum Requirements
When the dispenser is started it waits for a button to be pressed and blinks an LED while waiting.
When the button is pressed the dispenser is calibrated by turning the wheel at least one full turn and
then stopping the wheel so that the compartment with the sensor opening is aligned with the drop
tube.
After calibration the program keeps the LED on until user presses another button. Then it dispenses
pills every 30 seconds starting from the button press. When the wheel is turned, the piezo sensor is
used to detect if a pill was dispensed or not. If no pill was detected the LED blinks 5 times. When all
pills have been dispensed (wheel turned 7 times) the program starts over (wait for calibration, etc.).

### Advanced Requirements
Device remembers the state of the device across boot/power down.
Device connect to LoRaWAN and reports status of the device when there is a status change (boot, pill
dispensed/not dispensed, dispenser empty, power off during turning, etc.)
Device can detect if it was reset / powered off while motor was turning.
Device can recalibrate automatically after power off during middle of a turn without dispensing the
pills that still remain.

## Operation Principle
![image](https://github.com/Gemmus/PillSpiller/assets/112064697/c04dadc6-7121-439b-ab9f-56200268ddd5)
<br><small>_Figure 2: Logic of implemented program._</small>
<br> 
