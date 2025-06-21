#pragma once

class Servo
{
public:
    Servo(int pin);
    void Move(int position);

private:
    int pca9685Pin;
    int lastPosition;
};