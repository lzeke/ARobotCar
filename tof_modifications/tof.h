#ifndef _TOFLIB_H_
#define _TOFLIB_H_
//
// Copyright (c) 2017 Larry Bank
// email: bitbank@pobox.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <stdbool.h>
//
// Read the model and revision of the
// tof sensor
//
int tofGetModel(int* model, int* revision);

//
// Read the current distance in mm
//
int tofReadDistance(void);

//
// Opens a file system handle to the I2C device
// sets the device continous capture mode
//
int tofInit(int iChan, int iAddr, int bLongRange);

// LZ
// 
bool initI2C(int iChan); // just opens the i2c file if it is not already open
void setSensorAddress(int iChan, int new_addr); // sets the sensor address for the VL53L0X sensor
bool switchSensor(int iAddr); // switches the i2c bus to the specified VL53L0X sensor

// functions implemented in the tof library, but exposed for reuse
// therefore the static keyword is removed from their declarations
unsigned char readReg(unsigned char ucAddr);
unsigned short readReg16(unsigned char ucAddr);
void readMulti(unsigned char ucAddr, unsigned char* pBuf, int iCount);
void writeReg16(unsigned char ucAddr, unsigned short usValue);
void writeReg(unsigned char ucAddr, unsigned char ucValue);
void writeMulti(unsigned char ucAddr, unsigned char* pBuf, int iCount);
void writeRegList(unsigned char* ucList);
// end LZ

#endif // _TOFLIB_H
