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

## Methods and Material
### Processing Environment
#### Raspberry Pi Pico WH and Raspberry Pi Debug Probe
The Raspberry Pi Pico WH, paired with the Raspberry Pi Debug Probe, forms the core processing unit of the embedded device. Raspberry Pi Pico features a RP2040 microcontroller chip and a dual-core Arm Cortex M0+ processor, as illustrated on the left side of Figure 2. [1] The board supports a wide range of peripherals and also offers a 2.4 GHz wireless LAN connection.

![image](https://github.com/Gemmus/PillSpiller/assets/112064697/bcbfdf9d-8f5e-48c8-8dad-a8fcab81363e)
<br><small>_Figure 2: showcases the Raspberry Pi Pico WH on the left and Raspberry Pi Debug Probe on the right. [1][2]_</small>
<br> 

During the developmental phase of the project, the Raspberry Pi Debug Probe, depicted on the right side of Figure 1, plays a pivotal role. This USB device offers solderless, plug-and-play debug functionality through a standard ARM Serial Wire Debug (SWD) interface. [2]

#### I2C EEPROM
In order to store the state of the device throughout the boot process and the log messages, the implementation of a non-volatile memory storage is required. For this objective, an electrically erasable programmable read-only memory, the Elecrow’s Crowtail - I2C EEPROM, has been selected. 

![image](https://github.com/Gemmus/PillSpiller/assets/112064697/9d2b6cad-43c5-4b84-8d30-fdb9ff77b0bd)
<br><small>_Figure 3: showcases the Crowtail i2c EEPROM, the non-volatile memory of the system. [3]_</small>
<br> 

EEPROM is a modifiable memory, displayed in Figure 3, where bytes or pages can be erased and written to repeatedly by applying electrical voltage. It is widely used to store small amounts of data in electronic devices. The capacity of Crowtail - I2C EEPROM is 256k bits, providing ample storage for the device state. [4]

### LoRaWAN Communication
LoRa, a radio communication technique derived from chirp spread spectrum (CSS) technology, encodes information on radio waves using chirp pulses. The LoRaWAN protocol further defines the communication protocol and system architecture for devices utilizing LoRa technology. [5]

The current state of the dispenser and essential information are transmitted through
the LoRaWAN network. To facilitate this communication, the project has adopted the Grove LoRa-E5, showcased in 
Figure 4. This module establishes a connection to the processing environment through a UART connection.

![image](https://github.com/Gemmus/PillSpiller/assets/112064697/d45d4655-362b-49b6-bcc9-7988c2a24ea1)
<br><small>_Figure 4: Hardware specifications of the Grove - LoRa-E5, the LoRaWAN communication module for the project. [6]_</small>
<br> 

Designed by Seeed studio, the Grove LoRa-E5 is a wireless radio module that supports both LoRa and LoRaWan protocols. [6] Each communication instance is controlled through AT commands. Once a connection with the server is established, each message is sent individually, with each transmission prompting a 10-second waiting period.

### Dispenser Assembly
This subchapter introduces the aspect of the project with which the user directly interacts. Divided into two parts, the first section delves into the intricacies of the physical pill dispenser, while the second part places emphasis on the hardware's sensor components.

#### Stepper Motor
The base of the pill dispenser comprises two plastic components, crafted with a 3D printer: the base and the wheel, as illustrated in Figure 5. In addition to the plastic compartment, the base houses the stepper motor, as well as the the optical and piezoelectric sensors. 

![image](https://github.com/Gemmus/PillSpiller/assets/112064697/77508716-1be4-4ce2-8db7-cc6d5a27d77e)
<br><small>_Figure 5: Showcase of pill dispenser: the wheel on the left, base on the right, and the two sensors - piezoelectric and optical._</small>
<br> 

The plastic part of the base features an opening designed for pill dispensing. At the bottom of this aperture, resides the piezo sensor that detects the descent of the pill(s). The wheel connects to the base through the stepper motor shaft and incorporates a calibration opening that facilitates the functionality of the optical sensor, known as the opto fork. As mentioned in Chapter 1, the wheel is divided into 8 equal compartments: one for calibration and seven for storing drugs corresponding to each day of the week.

#### Sensors of the System
The system incorporates two sensors: an optical sensor to precisely detect the location of the dispenser wheel and a piezoelectric sensor to verify the dispensing of the pill(s) from the respective compartment. Each is discussed in their own subchapter.

##### Optical Sensor

##### Piezoelectric Sensor 


## Operation Principle
![image](https://github.com/Gemmus/PillSpiller/assets/112064697/c04dadc6-7121-439b-ab9f-56200268ddd5)
<br><small>_Figure 6: Logic of implemented program._</small>
<br> 

## References
1 Raspberry Pi. Raspberry Pi Pico WH. Accessed: 05.01.2024
https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html <br>
2 Raspberry Debug Probe. Accessed: 05.01.2024
https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html <br>
3 Partco. Crowtail I2C EEPROM 2.0 (24C256). Accessed: 05.01.2024 
https://www.partco.fi/en/diy-kits/crowtail/23675-ect-ct010021e.html <br>
4 Elecrow. Crowtail - I2C EEPROM. Accessed: 05.01.2024 
https://www.elecrow.com/wiki/index.php?title=Crowtail-_I2C_EEPROM <br>
5 Wikipedia. LoRa. Accessed: 05.01.2024 
https://en.wikipedia.org/wiki/LoRa <br>
6 Seeed studio. Grove - LoRa-E5. Accessed: 05.01.2024 
https://media.digikey.com/pdf/Data%20Sheets/Seeed%20Technology/Grove_LoRa-E5_Web.pdf <br>
