#pragma once

class PWM
{
public:
    PWM();
    bool Init(const char *period_ns = "10000000", const char *duty_cycle_ns = "2500000");

private:
    bool WriteSYS(const char filename[], const char value[]);
};