#pragma once

class TTMotor
{
public:
    TTMotor(PiPCA9685::PCA9685 *pPCA9685, int pSpeed, int pForward, int pBackward);
    void Stop();
    void MoveForward(int speed = 19);
    void MoveBackward(int speed = 19);

private:
    PiPCA9685::PCA9685 *pPCA;
    int pinSpeed;
    int pinForWard;
    int pinBackward;
};