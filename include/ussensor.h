#pragma once

class UsSensor
{
public:
    UsSensor(int triggerGpio, int echoGpio);
    float GetDistanceCm();
    void ReleaseGpioLines();

private:
    struct gpiod_line *pLineTrigger;
    struct gpiod_line *pLineEcho;
};