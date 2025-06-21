#include <stdio.h>
#include <PCA9685.h>
#include "ttMotor.h"

TTMotor::TTMotor(PiPCA9685::PCA9685 *pPCA9685, int pSpeed, int pForward, int pBackward)
{
    this->pPCA = pPCA9685;
    this->pinSpeed = pSpeed;
    this->pinForWard = pForward;
    this->pinBackward = pBackward;
}

void TTMotor::Stop()
{
    pPCA->set_pwm_ms(this->pinSpeed, 0); // speed (speed range: 11-19)

    // direction control: 0-1, or 1-0
    pPCA->set_pwm_ms(this->pinForWard, 15); // speed (15 means 1)
    pPCA->set_pwm_ms(this->pinBackward, 1); // speed (5 means 0)
}

void TTMotor::MoveForward(int speed) // default speed=19
{
    pPCA->set_pwm_ms(this->pinSpeed, speed); // speed (speed range: 11-19)

    // direction control: 0-1, or 1-0
    pPCA->set_pwm_ms(this->pinForWard, 15); // speed (15 means 1)
    pPCA->set_pwm_ms(this->pinBackward, 1); // speed (5 means 0)
}

void TTMotor::MoveBackward(int speed) // default speed=19
{
    pPCA->set_pwm_ms(this->pinSpeed, speed); // speed (speed range: 11-19)

    // direction control: 0-1, or 1-0
    pPCA->set_pwm_ms(this->pinForWard, 1);   // speed (15 means 1)
    pPCA->set_pwm_ms(this->pinBackward, 15); // speed (5 means 0)
}
