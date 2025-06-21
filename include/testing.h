#pragma once
#include "lasersensor.h"

class Testing
{
public:
    void TestServo();
    void TestTTMotor(int length);
    void TestLaserSensors();
    void TestTextToSpeech();
    void TestSpeechToText();
    void TestCompass();
    void TestGyro();

private:
    void TestLaserSensor(const char *text, LaserSensor *pSensor, int repeatCount);
};