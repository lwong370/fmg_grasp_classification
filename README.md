# Wearable Device for Biopotential Signal Acquisition

## Overview
### Objective
Designing an embedded wearable device as a modular application for registering biopotential signal acquisition and wireless data transmission to a computer application for prosthetic rehabilitation applications.

**I will update this as I go :)

### Built with: 
#### *Software*
- ESP-IDF Driver
  
#### *Hardware*
| Part | Quantity 
|:-----|:-----|
| ESP32 TinyS3 MCU | 1
| 3.7V LiPo Battery | 1
| FSR400 sensors | 8
| MCP3221 12-Bit ADC w/ I2C Interface | 8
| SO23-5 Breakout Board | 8

## Code Information
- **main/main_old_code.c** -- <br/>
    1. Contains the app_main() code, which includes the code that makes the LED turn on and off (from first experiment) <br/>
    2. Code that runs the predictions. Runs predictions with heartscale.model file and prints the prediction to the console. 
- **liblinear folder** -- Essentially depreceated at this point. Contains code from the liblinear GIT library that does the linear classification. Now classification will be done in MATLAB on the computer.
- **src/predictor.cpp** -- Ignore for now, main/GPIO_Example.c contains the code from this already.

## **Current Progress** 
At this point, I have played around with using the ESP TinyS3 MCU. As part of experimenting with using it, I programmed it to turn an LED on and off with a push-button. 
Now, I am experimenting the TinyS3 with classification. I ported code from liblinear to do classification with a model called heartscale.model (added to the TinyS3 file system called SPIFF). 

## Design Phases and Decisions

**Phase 1**: Breadboard prototype with FMG sensors  
- **Design Decisions:**
  1. **Connect force sensors along I²C bus lines**  
     This approach ultimately eliminates extra wiring and enables modularity, allowing different sensor types to be swapped in and out of the band.  
     Drawback is that I²C is slower than other alternatives such as SPI sensor interfaces or direct ADC pin connections. However, using I²C ensures that the number of available ADC pins on the MCU does not limit the number of sensor channels, allowing for future expansion if additional sensors are added. While adding more sensors does increase I²C bus latency (longer amount of time to get data from all sensors since multiple slave devices can't *technically* be read simultaneously with this set-up), the physical space constraints of the band inherently cap the number of sensors that can be placed. As a result, the latency will remain well within acceptable limits for our data acquisition requirements.

**Phase 2**: Protoboard prototype- 

## Challenges
### Part Selection
1. I²C-interface ADC IC components within acceptable voltage range only offered as surface mount units. For breadboard and protoboard steps, required finding suitable breakout board of size S023-5 to connect. Bought MCP3221 from Microchip but that component can only be configured to have 8 unique slave addresses. Realized that a multiplexer might need to be used to deploy multiple I²C bus-lines if more than 8 sensors are being used at the same time. 




