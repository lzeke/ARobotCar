#include <stdio.h>
#include <gpiod.h>
#include "ussensor.h"
#include <unistd.h>
#include <chrono>

extern struct gpiod_chip *pChip;

// This is for the HC-SR04 ultrasonic sensors. I used them at the beginning - not any more.
// They work great for hard material like wood, steel, stone, wall,
// but soft material (carpet, etc.) is invisible for them, therefore they are 
// pretty useless for object detection since some objects they just don't see.
// The VL53L0X time of flight sensor doesn't have this problem as far as I can see. 

UsSensor::UsSensor(int triggerGpio, int echoGpio)
{
    pLineTrigger = gpiod_chip_get_line(::pChip, triggerGpio);
    pLineEcho = gpiod_chip_get_line(::pChip, echoGpio);

    // Open trigger line for output
    gpiod_line_request_output(pLineTrigger, "example1", 0);
    gpiod_line_request_input(pLineEcho, "example1");
}

void UsSensor::ReleaseGpioLines()
{
    gpiod_line_release(pLineTrigger);
    gpiod_line_release(pLineEcho);
}

float UsSensor::GetDistanceCm()
{
    float distance = -1;

    if (gpiod_line_set_value(pLineTrigger, 0) != 0)
        printf("ERROR:%s(): faild to set trigger line to 0\n", __func__);
    usleep(2);
    if (gpiod_line_set_value(pLineTrigger, 1) != 0)
        printf("ERROR:%s(): faild to set trigger line to 1\n", __func__);
    usleep(10);
    if (gpiod_line_set_value(pLineTrigger, 0) != 0)
        printf("ERROR:%s(): faild to set trigger line to 0 again\n", __func__);

    int count1 = 1;
    while (gpiod_line_get_value(pLineEcho) == 0)
        count1++;
    // printf("Waited for 0 for %d\n", count1);

    auto start = std::chrono::high_resolution_clock::now();

    int count2 = 1;
    while (gpiod_line_get_value(pLineEcho) == 1)
        count2++;
    // printf("Waited for 1 for %d\n", count2);

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    distance = duration.count() * 0.0343 / 2;

    return distance;
}