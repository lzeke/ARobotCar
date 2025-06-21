#pragma once

class Led
{
public:
    Led(int gpio);
    void SetValue(int value);

private:
    struct gpiod_line *pGpioLine;
};