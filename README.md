**Current Project Progress**: <br /> 
At this point, I have played around with using the ESP TinyS3 MCU. As part of experimenting with using it, I programmed it to turn an LED on and off with a push-button. 
Now, I am experimenting the TinyS3 with classification. I ported code from liblinear to do classification with a model called heartscale.model (added to the TinyS3 file system called SPIFF). 

**main/GPIO_Example.c** -- <br/>
    1. Contains the app_main() code, which includes the code that makes the LED turn on and off (from first experiment) <br/>
    2. Code that runs the predictions. Runs predictions with heartscale.model file and prints the prediction to the console. 

**liblinear folder** -- Contains code from the liblinear GIT library that does the linear classification

**src/predictor.cpp** -- Ignore for now, main/GPIO_Example.c contains the code from this already.

