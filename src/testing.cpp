#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "testing.h"
#include "servo.h"
#include "car.h"
#include "lasersensor.h"
#include "texttospeech.h"
#include "speechtotext.h"
#include "lsm6dsox_lis3mdl.h"

#define PI 3.14159265358979323846

extern Servo *pServo;
extern Car *pCar;
extern LaserSensor *pLeftSensor;
extern LaserSensor *pRightSensor;
extern LaserSensor *pForwardSensor;
extern LaserSensor *pFloorSensor;
extern TextToSpeech *pTextToSpeech;
extern SpeechToText *pSpeechToText;
extern Lsm6dsoxLis3mdl *pLsmLis;
extern const char *myGoogleProjectId;

// various testing routines to test the individual features one by one

void Testing::TestServo()
{
  printf("Moving servo\n");
  pServo->Move(0);
  sleep(2);
  pServo->Move(90);
  sleep(2);
  pServo->Move(180);
  sleep(2);
  pServo->Move(90);
}

void Testing::TestTTMotor(int testLengthInSeconds)
{
  pCar->MoveForward();
  printf("The car should be moving\n");
  sleep(testLengthInSeconds);
  pCar->MoveBackward();
  sleep(testLengthInSeconds);
  pCar->Stop();
}

void Testing::TestLaserSensor(const char *text, LaserSensor *pSensor, int repeatCount)
{
  for (int i = 0; i < repeatCount; i++)
  {
    float d = pSensor->GetDistanceCm();
    printf("%s Distance: %f cm\n", text, d);
    sleep(1); // wait 1 second
  }
}

void Testing::TestLaserSensors()
{
  TestLaserSensor("Left", pLeftSensor, 5);
  TestLaserSensor("Right", pRightSensor, 5);
  TestLaserSensor("Forward", pForwardSensor, 5);
  TestLaserSensor("Floor", pFloorSensor, 5);
}

void Testing::TestTextToSpeech()
{
  pTextToSpeech->Talk("This is my first sentence. And this is the second.");
  sleep(3);
}

void Testing::TestSpeechToText()
{
  printf("Say something:\n");
  const char *textOfSpeech = pSpeechToText->ProcessSpeech(myGoogleProjectId);
  if (textOfSpeech != NULL && strlen(textOfSpeech) > 0)
  {
    printf("Transcript of speech: %s\n", textOfSpeech);
  }
  else
  {
    printf("Nothing was said.\n");
  }
}

void Testing::TestCompass()
{
  int max = 5;
  double prevHeading = 1000;
  for(int i = 0; i < max; i++)
  {
    Lsm6dsoxLis3mdl::vector<double> correctedValues = pLsmLis->GetCompassValuesHSCorrected();

    double heading = atan2(correctedValues.y, correctedValues.x) * 180 / PI;
    printf("Current heading:%f\n", heading);

    if(prevHeading != 1000)
    {
      double diff = heading - prevHeading;
      printf("Turned by %f\n", (diff));
    }
    prevHeading = heading;

    if(i < max - 1)
    {
      printf("Turn it now, then hit enter.\n");
      getchar();
    }
  }
}

void Testing::TestGyro()
{
  int max = 5;
  int freq = 10; // Hz
  int dt = (int) (1000.0 / (double) freq); // millisecond
  int loopLength = 2 * 1000 / dt; // 2 sec

  for(int i = 0; i < max; i++)
  {
    printf("Hit enter when ready to turn and start turning.\n");
    getchar();
    pLsmLis->CalculateAcceleratorAveBias();
    pLsmLis->CalculateGyroAveBias();
    pLsmLis->lastGyroAngles = { 0, 0, 0 };
    printf("Loop starts\n");
    for(int t = 0; t < loopLength; t++)
    {
      pLsmLis->lastGyroAngles = pLsmLis->GetGyroAngles(dt, pLsmLis->lastGyroAngles);
      //printf("Gyro angles: x=%f y=%f z=%f\n", pLsmLis->lastGyroAngles.x, pLsmLis->lastGyroAngles.y, pLsmLis->lastGyroAngles.z);      
      usleep(dt * 1000);
    }
    printf("Stop turning\n");
    printf("Final Gyro angles: x=%f y=%f z=%f\n", pLsmLis->lastGyroAngles.x, pLsmLis->lastGyroAngles.y, pLsmLis->lastGyroAngles.z);
  }
}

