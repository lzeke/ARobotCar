#include <stdio.h>
#include <gpiod.h>
#include <unistd.h>
#include <chrono>
#include "servo.h"
#include <PCA9685.h>

extern PiPCA9685::PCA9685 *pPCA;

Servo::Servo(int pin)
{
  this->pca9685Pin = pin;
  this->lastPosition = -1;
}

// This is for the SG90 mini-servo.
// Its duty cycle is 50Hz (20ms) while the pulse-with is a few ms
void Servo::Move(int position) // position: 0-180
{
  if(position != this->lastPosition)
  {
    // 0.5: 0degree, 1.5ms:90degree, 2.5ms:180 degree
    double value = 2.0 * (((double)position) / 180.0) + 0.5;
    // printf("pin:%d, value=%f\n", this->pca9685Pin, value);
    pPCA->set_pwm_ms(this->pca9685Pin, value);
    this->lastPosition = position;
    usleep(250000); // wait 0.25 seconds
  }
}