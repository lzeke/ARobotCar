C++ code for a proof-of-concept, Raspberry Pi 5 based robot car (using the following controllers, and sensors: PCA9685, SG90, VL53L0X, LM2596, L298N, LSM6DSOX, LIS3MDL)

The capabilities of the car:
- TT-motor driving (using LM2596 voltage regualators and L298N motor drivers)
- motor driver and servo control using the PCA9685 PWM Servo Motor Driver
- object avoidance (using multiple VL53L0X sensors)
- floor sensing (using a single VL53L0X sensor)
- text-to-speech (talking)
- speech-to-text (speech recognition through Google's speech-to-text API)
- not too succesful turning angle detection attempt using the LSM6DSOX accelerometer and gyroscope and the LIS3MDL compass.

Details can be found at: https://www.youtube.com/watch?v=5kd9n4RXpWY

Use VS Code to edit, use CMAKE to compile.

Rapsberry PI 5 GPIO pin assignments are in the main.cpp file

PCA9685 pin assignments in the src/car.cpp file

This code depends on the following libraries:

- gpiod library: for GPIO communication: https://github.com/brgl/libgpiod
- PiPCA9685 library: for PCA9685 communication: https://github.com/barulicm/PiPCA9685
- i2c library: for I2C communication: part of Linux (more precisely the i2c-tools package) 
- tof library: based on code from the Pololu Corp. to communicate with the VL53LOX chips (I modified it slightly to reuse some of its methods): https://github.com/bitbank2/VL53L0X
- espeak-ng: for speech synthesis: https://github.com/espeak-ng/espeak-ng
- miniaudio.h: for speech capture: https://github.com/mackron/miniaudio
- fvad: for Voice Activity Detection: https://github.com/dpirch/libfvad
- sndfile: used by fvad for audio file read/write. It is part of the libsndfile1 and libsndfile-dev packages
- googlespeechtotext: my own library encapsulating google's speech-to-text API. When we compile google's speech-to-text library, the loader takes a long time, therefore, I separated it out into my own library to save compile time.
