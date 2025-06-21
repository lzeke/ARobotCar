#pragma once

class LaserSensor
{
public:
    LaserSensor();
    bool ResetSensor(int gpioPin);
    bool FinishResetting();
    bool SetAddress(int slaveAddress);
    bool Init();
    int GetDistanceCm();
    void Finish();
    bool TestAddressChange(int slaveAddress);

private:
    int i2cSlaveAddress;
    int gpioRestPin;
    struct gpiod_line *pGpioLine;
};