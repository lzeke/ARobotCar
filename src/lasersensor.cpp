#include <stdio.h>
#include <unistd.h>
#include <gpiod.h>
#include "lasersensor.h"

extern struct gpiod_chip *pChip;
#define default_address 0x29 // default address of a VL53L0X chip

extern "C"
{
#include <tof.h> // time of flight sensor library
}

LaserSensor::LaserSensor()
{
    this->i2cSlaveAddress = default_address;
    this->pGpioLine = NULL;
}

bool LaserSensor::ResetSensor(int gpioPin)
{
    bool ret = false;

    this->gpioRestPin = gpioPin;

    // reset the sensor
    if ((this->pGpioLine = gpiod_chip_get_line(::pChip, this->gpioRestPin)) == NULL)
    {
        printf("ERROR:%s(): gpiod_chip_get_line failed.\n", __func__);
        ret = true;
    }

    if (!ret && gpiod_line_request_output(this->pGpioLine, "example1", 1) != 0)
    {
        printf("ERROR:%s(): gpiod_line_request_output failed.\n", __func__);
        ret = true;
    }

    usleep(10000);
    if (!ret && gpiod_line_set_value(this->pGpioLine, 0) != 0)
    {
        printf("ERROR:%s(): gpiod_line_set_value failed.\n", __func__);
        ret = true;
    }
    usleep(10000);

    return ret;
}

// This method is not used, just for testing purposes
bool LaserSensor::FinishResetting()
{
    bool ret = false;

    if (!ret && gpiod_line_set_value(this->pGpioLine, 1) != 0)
    {
        printf("ERROR:%s(): gpiod_line_set_value failed.\n", __func__);
        ret = true;
    }
    usleep(10000);

    return ret;
}

bool LaserSensor::SetAddress(int newSlaveAddress)
{
    bool ret = false;

    if (!ret && gpiod_line_set_value(this->pGpioLine, 1) != 0)
    {
        printf("ERROR:%s(): gpiod_line_set_value failed.\n", __func__);
        ret = true;
    }
    usleep(10000);

    this->i2cSlaveAddress = default_address;
    if (!ret)
        ret = this->Init();

    if (!ret)
        setSensorAddress(1, newSlaveAddress);

    this->i2cSlaveAddress = newSlaveAddress;

    return ret;
}

bool LaserSensor::Init()
{
    bool ret = false;
    int model, revision;

    // to set long range mode (up to 2m) last parameter should be 1
    int i = tofInit(1, this->i2cSlaveAddress, 0); 
    if (i != 1)
    {
        printf("ERROR: %s(): tofInit() fails\n", __func__);
        ret = true;
    }
    else
    {
        usleep(10000); // sleep 10ms
        // printf("VL53L0X device successfully opened.\n");
        i = tofGetModel(&model, &revision);
        // printf("Model ID - %d\n", model);
        // printf("Revision ID - %d\n", revision);
    }
    return ret;
}

int LaserSensor::GetDistanceCm()
{
    switchSensor(this->i2cSlaveAddress);
    usleep(1000); // sleep 1ms
    int iDistance = tofReadDistance();
    // if iDistance > 4096, then it is invalid
    int distanceInCm = iDistance / 10;

    return distanceInCm;
}

void LaserSensor::Finish()
{
    if (this->pGpioLine != NULL)
        gpiod_line_release(this->pGpioLine);
}