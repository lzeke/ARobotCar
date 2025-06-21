#include <cstdint>
#include <cstddef>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include "lsm6dsox_lis3mdl.h"

extern "C"
{
#include <tof.h> // time of flight sensor library
}

#define LSM6DSOX_SLAVE 0x6A 
#define LSM6DSOX_SSTATUS 0x1E // 1: accel available, 2: gyro available, 4: temp available

#define LSM6DSOX_CTRL1_XL 0x10 //0b01010000 (0x50)// 0x50=208Hz accelerometer normal mode, 3.33Khz: 0x90, +-2G
#define LSM6DSOX_CTRL2_G 0x11 //:0b01010010 (0x52)// 0x52=208Hz high performance mode, 3.33Khz: 0x92, +-125dps
#define LSM6DSOX_CTRL3_C 0x12 //:0b00000100 (0x04) // Register address automatically incremented during a multiple byte access with a serial interface

#define EARTH_GRAVITY 9.81
#define ACCEL_SCALING_FOR_2G 0.06104 // sensitivy per LSM6DSOX data sheet in mm/s2
#define GYRO_SCALING_FOR_125DPS 4.375 // sensitivy per LSM6DSOX data sheet in mdsp

#define WHO_AM_I 0x0f // same address for both chips

#define LIS3MDL_SLAVE 0x1c 
#define LIS3MDL_STATUS 0x27 //1: X available, 2: Y available, 4: Z available, 8: XYZ available
#define LIS3MDL_CTRL_REG1 0x20 // 0b11011100 128: Temp sensor enable, 64: HIgh perf mode, 16+8+4: 80Hz output
#define LIS3MDL_CTRL_REG2 0x21 // 4: soft reset, 8: reboot memory content, +-4Gauss
#define LIS3MDL_CTRL_REG3 0x22 // 0: 4 wire i2c and continuous conversion (THIS IS NOT THE DEFAULT)
#define COMPASS_SCALING_FOR_4G 6842.0

#define LSM6DSOX_TEMP_OUT_LOW 0x20
#define LSM6DSOX_TEMP_OUT_HIGH 0x21

#define LIS3MDL_TEMP_OUT_LOW 0x2E
#define LIS3MDL_TEMP_OUT_HIGH 0x2F

#define ACCEL_X_OUT_LOW 0x28
#define ACCEL_X_OUT_HIGH 0x29
#define ACCEL_Y_OUT_LOW 0x2A
#define ACCEL_Y_OUT_HIGH 0x2B
#define ACCEL_Z_OUT_LOW 0x2C
#define ACCEL_Z_OUT_HIGH 0x2D

#define GYRO_X_OUT_LOW 0x22
#define GYRO_X_OUT_HIGH 0x23
#define GYRO_Y_OUT_LOW 0x24
#define GYRO_Y_OUT_HIGH 0x25
#define GYRO_Z_OUT_LOW 0x26
#define GYRO_Z_OUT_HIGH 0x27

#define COMPASS_X_OUT_LOW 0x28
#define COMPASS_X_OUT_HIGH 0x29
#define COMPASS_Y_OUT_LOW 0x2A
#define COMPASS_Y_OUT_HIGH 0x2B
#define COMPASS_Z_OUT_LOW 0x2C
#define COMPASS_Z_OUT_HIGH 0x2D

extern bool debug;

bool Lsm6dsoxLis3mdl::Init()
{
    bool ret = ::initI2C(1);

    if(!ret)
    { // init LSM6DSOX
        ret = switchSensor(LSM6DSOX_SLAVE);
        if(!ret)
        {
            ::writeReg(LSM6DSOX_CTRL1_XL, 0x90); // 0x50=208Hz, 0x90=3.3Khz
            ::writeReg(LSM6DSOX_CTRL2_G, 0x92); // 0x52=208Hz, 0x92=3.3Khz
            ::writeReg(LSM6DSOX_CTRL3_C, 0x04);

            unsigned char id = ::readReg(WHO_AM_I);
            if(::debug) printf("LSM6DSOX says its id is 0%x (should be 0x6c)\n", id);
        }
    }

    if(!ret)
    { // init LIS3MDL
        ret = ::switchSensor(LIS3MDL_SLAVE);
        if(!ret)
        {
            ::writeReg(LIS3MDL_CTRL_REG2, 0x0C); // reset and reboot memory content, and set range for +-4G
            usleep(1000);
            ::writeReg(LIS3MDL_CTRL_REG1, 0xDC);
            ::writeReg(LIS3MDL_CTRL_REG3, 0x00);

            unsigned char id = ::readReg(WHO_AM_I);
            if(::debug) printf("LIS3MDL says its id is 0%x (should be 0x3d)\n", id);
        }
    }

    return ret;
}

Lsm6dsoxLis3mdl::vector<int> Lsm6dsoxLis3mdl::GetAcceleratorRawValues()
{
    vector<int> accel = {0, 0, 0};

    if(!::switchSensor(LSM6DSOX_SLAVE))
    {
        unsigned char status = ::readReg(LSM6DSOX_SSTATUS);
        if(::debug)
        {
            printf("Status: %s,%s,%s\n", 
                ((status & 1) ? "accel is avail" : ""),
                ((status & 2) ? "gyro is avail" : ""),
                ((status & 4) ? "temp is avail" : ""));    
        }

        if(status & 1)
        {
            unsigned char buffer[6];
            ::readMulti(ACCEL_X_OUT_LOW, buffer, 6);
            accel.x = (int16_t)(buffer[1] << 8 | buffer[0]);
            accel.y = (int16_t)(buffer[3] << 8 | buffer[2]);
            accel.z = (int16_t)(buffer[5] << 8 | buffer[4]);

            lastAccelRawValues.x = accel.x;
            lastAccelRawValues.y = accel.y;
            lastAccelRawValues.z = accel.z;

            if(::debug) printf("Accel: x:%d y:%d z:%d\n", accel.x, accel.y, accel.z);
        }
        else
        {
            accel.x = lastAccelRawValues.x;
            accel.y = lastAccelRawValues.y;
            accel.z = lastAccelRawValues.z;
            printf("ERROR: can't read accelerator values - they are not ready.\n");
        }
    }

    return accel;
}

void Lsm6dsoxLis3mdl::CalculateAcceleratorAveBias()
{ // read values 10 times, 10ms apart
    this->lastAccelRawValues = { 0, 0, 0};
    this->accelRawBias = { 0, 0, 0};

    int count = 0;
    int max = 10;
    for(int i = 0; i < max; i++)
    {
        vector<int> accelRaw = this->GetAcceleratorRawValues();
        if(::debug) printf("Gyro: raw_x:%d, raw_y:%d raw_z:%d\n", accelRaw.x, accelRaw.y, accelRaw.z);
        this->accelRawBias.x += accelRaw.x;
        this->accelRawBias.y += accelRaw.y;
        this->accelRawBias.z += accelRaw.z;
        count++;
        usleep(10000);
    }

    this->accelRawBias.x /= count;
    this->accelRawBias.y /= count;
    this->accelRawBias.z /= count;
}

Lsm6dsoxLis3mdl::vector<double> Lsm6dsoxLis3mdl::GetAccelatorValues()
{ // get accelerator values, correct with average bias, and scale them
    vector<int> accelValues =  this->GetAcceleratorRawValues();

    accelValues.x -=  this->accelRawBias.x;
    accelValues.y -=  this->accelRawBias.y;
    accelValues.z -=  this->accelRawBias.z;

    // accelValue = rawValue * sensitivity * g / 1000; Because sensitivity is in mm/sec2
    // sensitivity is specified by the data sheet
    vector<double> aValues;    
    aValues.x = ((double) accelValues.x) * ACCEL_SCALING_FOR_2G * EARTH_GRAVITY / 1000.0; 
    aValues.y = ((double) accelValues.y) * ACCEL_SCALING_FOR_2G * EARTH_GRAVITY / 1000.0; 
    aValues.z = ((double) accelValues.z) * ACCEL_SCALING_FOR_2G * EARTH_GRAVITY / 1000.0; 

     return aValues;
}

Lsm6dsoxLis3mdl::vector<double> Lsm6dsoxLis3mdl::GetAcceleratorAngles()
{ // calculate the angles (in degrees) derived from acceleration
    const double PI = 3.14159265358979323846;
    vector<double> accelAngles;
    vector<double> accelValues = this->GetAccelatorValues();
    double x = accelValues.x;
    double y = accelValues.y;
    double z = accelValues.z;
    
    //double vector_length = sqrt(x*x + y*y + z*z);
    accelAngles.x = atan(y/sqrt(x*x + z*z));
    accelAngles.y = atan(-1.0 * x / sqrt(y*y + z*z));
    accelAngles.z = 0;

    // convert to degrees
    accelAngles.x *= 180.0/PI;
    accelAngles.y *= 180.0/PI;
    accelAngles.z *= 180.0/PI;

    return accelAngles;
}

Lsm6dsoxLis3mdl::vector<int> Lsm6dsoxLis3mdl::GetGyroRawValues()
{
    vector<int> gyro =  {0, 0, 0};

     if(!::switchSensor(LSM6DSOX_SLAVE))
    {
        unsigned char status = ::readReg(LSM6DSOX_SSTATUS);
        if(::debug)
        {
            printf("Status: %s,%s,%s\n", 
                ((status & 1) ? "accel is avail" : ""),
                ((status & 2) ? "gyro is avail" : ""),
                ((status & 4) ? "temp is avail" : ""));  
        }  

        if(status & 2)
        {
            unsigned char buffer[6];
            ::readMulti(GYRO_X_OUT_LOW, buffer, 6);
            gyro.x = (int16_t)(buffer[1] << 8 | buffer[0]);
            gyro.y = (int16_t)(buffer[3] << 8 | buffer[2]);
            gyro.z = (int16_t)(buffer[5] << 8 | buffer[4]);

            lastGyroRawValues.x = gyro.x;
            lastGyroRawValues.y = gyro.y;
            lastGyroRawValues.z = gyro.z;

            if(::debug) printf("Gyro raw w/o bias: x:%d y:%d z:%d\n", gyro.x, gyro.y, gyro.z);
        }
        else
        {
            gyro.x = lastGyroRawValues.x;
            gyro.y = lastGyroRawValues.y;
            gyro.z = lastGyroRawValues.z;
            printf("ERROR: can't read gyro values - they are not ready.\n");
        }
    }

    return gyro;
}

void Lsm6dsoxLis3mdl::CalculateGyroAveBias()
{ // read values 10 times, 10ms apart
    this->lastGyroRawValues = { 0, 0, 0};
    this->gyroRawBias = { 0, 0, 0 };

    int count = 0;
    int max = 10;
    for(int i = 0; i < max; i++)
    {
        vector<int> gyroRaw = this->GetGyroRawValues();
        if(::debug) printf("Gyro: raw_x:%d, raw_y:%d raw_z:%d\n", gyroRaw.x, gyroRaw.y, gyroRaw.z);
        this->gyroRawBias.x += gyroRaw.x;
        this->gyroRawBias.y += gyroRaw.y;
        this->gyroRawBias.z += gyroRaw.z;
        count++;
        usleep(10000);
    }

    this->gyroRawBias.x /= count;
    this->gyroRawBias.y /= count;
    this->gyroRawBias.z /= count;
}

Lsm6dsoxLis3mdl::vector<double> Lsm6dsoxLis3mdl::GetGyroValues()
{ // get gyro values, correct them with average bias, and scale them
    vector<int> gyroValues =  this->GetGyroRawValues();

    gyroValues.x -=  this->gyroRawBias.x;
    gyroValues.y -=  this->gyroRawBias.y;
    gyroValues.z -=  this->gyroRawBias.z;

    // gyroValue = rawValue * sensitivity / 1000; Because sensitivity is in mDPS
    // sensitivity is specified by the data sheet
    vector<double> gValues;
    gValues.x = ((double) gyroValues.x) * GYRO_SCALING_FOR_125DPS / 1000.0; 
    gValues.y = ((double) gyroValues.y) * GYRO_SCALING_FOR_125DPS / 1000.0; 
    gValues.z = ((double) gyroValues.z) * GYRO_SCALING_FOR_125DPS / 1000.0; 

    //printf("Gyro scaled values: x:%f, y:%f z:%f\n", gValues.x, gValues.y, gValues.z);

    return gValues;
}

Lsm6dsoxLis3mdl::vector<double> Lsm6dsoxLis3mdl::GetGyroAngles(int storeValueStepMs, Lsm6dsoxLis3mdl::vector<double> lastGyroAngles)
{ // integrate over time
    vector<double> gyroAngles;
    vector<double> gyroValues = this->GetGyroValues();
    double timeUnit = ((double) storeValueStepMs) / 1000.0;

    gyroAngles.x = gyroValues.x * timeUnit + lastGyroAngles.x;
    gyroAngles.y = gyroValues.y * timeUnit + lastGyroAngles.y;
    gyroAngles.z = gyroValues.z * timeUnit + lastGyroAngles.z;

    return gyroAngles;
}

// I tried to use the compass with hard and soft iron correction, but
// the environment with 4 motors is way to "noisy" (magnetically), so
// it seems the compass is useless

Lsm6dsoxLis3mdl::vector<int> Lsm6dsoxLis3mdl::GetCompassRawValues()
{
    vector<int> compass = {0, 0, 0};

    if(!::switchSensor(LIS3MDL_SLAVE))
    {
        unsigned char status = ::readReg(LIS3MDL_STATUS);
        if(::debug)
        {
            printf("Status: %s,%s,%s,%s\n", 
                ((status & 1) ? "x is avail" : ""),
                ((status & 2) ? "y is avail" : ""),
                ((status & 4) ? "z is avail" : ""),
                ((status & 8) ? "xyz is avail" : ""));    
        }

        if(status & 8) {
            unsigned char buffer[6];
            ::readMulti(COMPASS_X_OUT_LOW, buffer, 6);
            compass.x = (int16_t)(buffer[1] << 8 | buffer[0]);
            compass.y = (int16_t)(buffer[3] << 8 | buffer[2]);
            compass.z = (int16_t)(buffer[5] << 8 | buffer[4]);
        }
        else
        {
            printf("ERROR: can't read compass - data is not ready\n");
        }
    }

    return compass;
}

Lsm6dsoxLis3mdl::vector<double> Lsm6dsoxLis3mdl::GetCompassValues()
{
    vector<double> values;
    vector<int> rawValues = this->GetCompassRawValues();

    values.x = ((double) rawValues.x) / COMPASS_SCALING_FOR_4G;
    values.y = ((double) rawValues.y) / COMPASS_SCALING_FOR_4G;
    values.z = ((double) rawValues.z) / COMPASS_SCALING_FOR_4G;

    // gauss to micro-tesla conversion
    // 1 tesla is 10,000 gauss, so 1 microtesla 0.01 gauss, therefore 1 gauss is 100 microtesla

    values.x *= 100.0;
    values.y *= 100.0;
    values.z *= 100.0;

    return values;
}

void Lsm6dsoxLis3mdl::CalculateCompassMinMax()
{
    int x_min = 10000, x_max = -10000;
    int y_min = 10000, y_max = -10000;

    printf("Rotate compass:\n");
    for(int i = 0; i < 1000; i++) {
      Lsm6dsoxLis3mdl::vector<int> compass = this->GetCompassRawValues();
      if(compass.x < x_min) x_min = compass.x;
      if(compass.x > x_max) x_max = compass.x;
      if(compass.y < y_min) y_min = compass.y;
      if(compass.y > y_max) y_max = compass.y;
      usleep(5000);
    }

    printf("x_min:%d x_max:%d y_min:%d y_max:%d\n", x_min, x_max, y_min, y_max);
}

Lsm6dsoxLis3mdl::vector<double> Lsm6dsoxLis3mdl::HardAndSoftIronCorrectedHeading(vector<int> rawValues)
{ // this uses correction matrix and offset calculated by MagMaster 1.0
    vector<double> correctedValues;
    const double PI = 3.14159265358979323846;

    // values from MagMaster when compass is in my hand
    // double m11 = 1.501, m12 = 0.018, m13 = 0.018;
    // double m21 = 0, m22 = 1.278, m23 = -0.093;
    // double m31 = 0.083, m32 = -0.085, m33 = 0.781;
    // double bx = -134.357, by = -1457.287, bz = 143.429;

    // values from MagMaster when compass is in the robotcar
    double m11 = 2.205, m12 = 0.075, m13 = 0.003;
    double m21 = 0.046, m22 = 1.754, m23 = 0.017;
    double m31 = 0.469, m32 = 0.02, m33 = 1.874;
    double bx = -2402.718, by =  269.093, bz = -5749.172;

    double xnc = (double) rawValues.x;
    double ync = (double) rawValues.y;
    double znc = (double) rawValues.z;

    correctedValues.x = m11 * xnc + m12 * ync + m13 * znc - bx;
    correctedValues.y = m21 * xnc + m22 * ync + m23 * znc - by;
    correctedValues.z = m31 * xnc + m32 * ync + m33 * znc - bz;

    return correctedValues;
}

Lsm6dsoxLis3mdl::vector<double> Lsm6dsoxLis3mdl::GetCompassValuesHSCorrected()
{
    vector<double> values;
    vector<int> rawValues = this->GetCompassRawValues();

    vector<double> correctedValues = HardAndSoftIronCorrectedHeading(rawValues);

    values.x = ((double) correctedValues.x) / COMPASS_SCALING_FOR_4G;
    values.y = ((double) correctedValues.y) / COMPASS_SCALING_FOR_4G;
    values.z = ((double) correctedValues.z) / COMPASS_SCALING_FOR_4G;

    // gauss to micro-tesla conversion
    // 1 tesla is 10,000 gauss, so 1 microtesla 0.01 gauss, therefore 1 gauss is 100 microtesla

    values.x *= 100.0;
    values.y *= 100.0;
    values.z *= 100.0;

    return values;
}

