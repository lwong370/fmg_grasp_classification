# Wearable Device Pipeline for Biopotential Signal Acquisition

## Overview
### Objective
Designing an embedded wearable device pipeline as a modular application for registering biopotential signal acquisition and wireless data transmission to a computer application for prosthetic rehabilitation applications.

### Built with: 
#### *Software*
- C++
- C
- ESP-IDF Driver
- FreeRTOS
  
#### *Hardware*
| Part | Quantity 
|:-----|:-----|
| ESP32 TinyS3 MCU | 1
| 3.7V LiPo Battery | 1
| FSR400 sensors | 8
| ADC121C021 12-Bit ADC w/ I2C Interface | 8
| Custom Breakout Board for ADC121C021 | 8
| 4.7kΩ Resistor | 10
| 100uF Capacitor | 10
| JST_SH_BM04B-SRSS-TB_1x04 | 16
| JST_PH_S2B-PH-K_1x02 | 1

## **Current Progress** 
**I will update this as I go :D

At this point, I have a breadboard prototype that includes:
- Selected necessary electronic componenets to build the system. Developed break-out boards required to connect SMD components to through-hole breadboard system. 
- Wired ADC IC's to an I2C busline and developed firmware code that communicates with the ICs slave addresses to read in force sensor data
- Use FreeRTOS to schedule tasks that collect data from force sensors and send that data to MATLAB wirelessly over BLE.
- Designed PCB of daughterboard that MCU will rest on top of for next stage of prototyping. 

## Design Phases and Decisions

**Phase 1**: Breadboard prototype with FMG sensors  
<p align="center">
  <img src="./images/breadboard_prototype.png"
       alt="Prototype on breadboard"
       width="300">
</p>

- **Design Decisions:**
  1. **Connect force sensors along I²C bus lines**  
     This approach ultimately eliminates extra wiring and enables modularity, allowing different sensor types to be swapped in and out of the band.  
     Drawback is that I²C is slower than other alternatives such as SPI sensor interfaces or direct ADC pin connections. However, using I²C ensures that the number of available ADC pins on the MCU does not limit the number of sensor channels, allowing for future expansion if additional sensors are added. While adding more sensors does increase I²C bus latency, the physical space constraints of the band inherently cap the number of sensors that can be placed. As a result, the latency will remain well within acceptable limits for our data acquisition requirements.


**Phase 2**: Breadboard prototype with FMG sensors  
- **PCB Design: MCU Daughter Board**
<p align="center">
  <img src="./images/esp_motherboard.png"
       alt="MCU Daughter Board"
       width="600">
</p>

<p align="center">
  <img src="./images/esp_motherboard_schematic.png"
       alt="MCU Daughter Board Schematic"
       width="600">
</p>
  
## Challenges
### 1. Ghosting ADC Pins
Ghosting ADC pins (when originally only used the analog pins and not the I2C busline to read in sensor input). When one sensor had pressure applied to it, the readings for all the other sensors would indicate that they were experiencing pressure being applied as well (despite that being the case). Internally, the ADC samples each channel by briefly connecting it to a shared sample-and-hold capacitor. When the sampling switch closes, this capacitor must charge (or discharge) to the input voltage within the available sampling window.
Because the FSR sensors present a high source impedance, the effective RC time constant formed with the ADC’s internal sample-and-hold capacitor becomes large. As a result, the capacitor cannot fully settle to the correct voltage before conversion. When the ADC rapidly switches between channels, residual charge from the previously sampled channel can be injected into the next channel, producing ghosting and cross-channel couplingI realized that the issue had to due with a high impedence inputBy reading the datasheet for the ESP32-S2, it can be directly inferred that there is only one internal mux and a Share-Hold capacitor that all ADC pins are connected to. Therefore, when the ADC switches channels the S/H cap and internal mux briefly share the same internal node. The fact that the ESP32 ADC lacks strong internal input buffering, making it sensitive to source impedance.

*Solution*: Adding small capacitors at each ADC input to stabilize the voltage and provide a local charge reservoir for the internal sample-and-hold capacitor

### 2. Part Selection
I²C-interface ADC IC components within acceptable voltage range only offered as SMD components. For breadboard and protoboard steps, required finding suitable breakout board to connect. Bought MCP3221 from Microchip but that component ended up only being able to be configured to 1 unique slave address. Originally was going to use a multiplexer but that would be less efificent than finding a new I²C-interface ADC IC part. Ended up choosing to use ADC121c021 from Texas Instruments, which has 8 configurable slave addresses. 

<p align="center">
  <img src="./images/adc12_breakout.png"
       alt="ADC IC Breakout Board"
       width="300">
</p>

### 3. Learning New Tools and Technologies
1. Learning PCB design with KiCAD
2. Understanding how BLE is configured
3. How to use FreeRTOS to manage multiple tasks and the general hierarchy of how the OS interacts with ESP drivers.



