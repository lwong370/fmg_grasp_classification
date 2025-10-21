# Wearable Device for Biopotential Signal Acquisition

## Overview
### Objective
Designing an embedded wearable device for biopotential signal acquisition and wireless data transmission to a computer application for prosthetic rehabilitation applications.

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

## Challenges
### Part Selection




