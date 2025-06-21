#include <stdio.h>
#include <gpiod.h>
#include "led.h"

// this is not used, just here if we want to add some LEDs for show

extern struct gpiod_chip *pChip;

Led::Led(int gpio)
{
    // Open GPIO lines
    pGpioLine = gpiod_chip_get_line(::pChip, gpio);

    // Open LED lines for output
    gpiod_line_request_output(pGpioLine, "example1", 0);
}

void Led::SetValue(int value)
{
    gpiod_line_set_value(pGpioLine, value);
}