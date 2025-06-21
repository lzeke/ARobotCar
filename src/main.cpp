#include <gpiod.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <PCA9685.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

// #include "ussensor.h"
#include "lasersensor.h"
#include "led.h"
#include "servo.h"
#include "car.h"
#include "texttospeech.h"
#include "speechtotext.h"
#include "pwm.h"
#include "lsm6dsox_lis3mdl.h"
#include "testing.h"

using namespace std;

char *voiceString; // global voice string
bool debug = false; // global debug flag to turn on debug logging

#include <sys/ioctl.h>
extern "C"
{
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
}

const char *myGoogleProjectId = "sprecforcar"; // my google project id

const int servoControlPin = 15; // the pin on the PCA9685 board to control the S90 servo

// GPIO pins for the VL53L0X laser sensors reset feature
const int rightSensorResetGpio = 17;
const int leftSensorResetGpio = 27;
const int forwardSensorResetGpio = 22;
const int floorSensorResetGpio = 23;

// I2C addresses for the VL53L0X laser sensors
const int rightSensorAddress = 0x31;
const int leftSensorAddress = 0x32;
const int forwardSensorAddress = 0x33;
const int floorSensorAddress = 0x34;

struct gpiod_chip *pChip;
PiPCA9685::PCA9685 *pPCA;

// global pointers
Testing *pTesting = NULL;
LaserSensor *pLeftSensor = NULL;
LaserSensor *pRightSensor = NULL;
LaserSensor *pForwardSensor = NULL;
LaserSensor *pFloorSensor = NULL;
Servo *pServo = NULL;
Car *pCar = NULL;
TextToSpeech *pTextToSpeech = NULL;
SpeechToText *pSpeechToText = NULL;
Lsm6dsoxLis3mdl *pLsmLis = NULL;
PWM *pPwm = NULL;

bool stopProgram; // if this is set to true, the program execution loop stops

// initializing the system
bool Init()
{ // returns false if success, true otherwise
  bool ret = false;

  voiceString = new char[100];

  pTesting = new Testing();

  const char *pChipName = "gpiochip0";
  if (::debug)
    printf("Init starting.\n");

  // Open GPIO chip
  if ((pChip = gpiod_chip_open_by_name(pChipName)) == NULL)
  {
    printf("ERROR: can't open gpio chip\n");
    ret = true;
  }
  else
  {
    if (::debug)
      printf("pChip is initialized.\n");
  }

  if (!ret)
  {
    pPwm = new PWM();
    ret = pPwm->Init();
  }

  if (!ret)
  {
    // Open PCA9685 board and set its frequency to 50Hz since the SG90 servo requires this
    try
    {
      pPCA = new PiPCA9685::PCA9685();
      pPCA->set_pwm_freq(50.0); 
      if (::debug)
        printf("PCA9685 is initialized.\n");
    }
    catch (std::exception e)
    {
      printf("ERROR: Exception %s \n", e.what());
    }
  }

  if (!ret)
  {
    pTextToSpeech = new TextToSpeech();
    ret = pTextToSpeech->Init();
  }

  if (!ret)
  {
    pSpeechToText = new SpeechToText();
  }

  if(!ret)
  {
    pLsmLis = new Lsm6dsoxLis3mdl();
    pLsmLis->Init();
  }

  stopProgram = false;

  if (!ret)
    printf("Init finished with SUCCESS.\n");
  return ret;
}

// Initially each laser sensor has the same I2C slave address,
// so we need to change this and set them to different addresses.
// First, we put all of them in a reset state, then bring them out
// of this state one-by-one and assign them a new I2C slave address
bool SetupLaserSensors()
{
  bool ret = false;

  pRightSensor = new LaserSensor();
  pLeftSensor = new LaserSensor();
  pForwardSensor = new LaserSensor();
  pFloorSensor = new LaserSensor();

  // put all of them in reset state
  if (!ret)
    ret = pRightSensor->ResetSensor(rightSensorResetGpio);
  if (!ret)
    ret = pLeftSensor->ResetSensor(leftSensorResetGpio);
  if (!ret)
    ret = pForwardSensor->ResetSensor(forwardSensorResetGpio);
  if (!ret)
    ret = pFloorSensor->ResetSensor(floorSensorResetGpio);

  // set address and initialize them one by one
  if (!ret)
    ret = pRightSensor->SetAddress(rightSensorAddress);
  if (!ret)
    ret = pRightSensor->Init();

  if (!ret)
    ret = pLeftSensor->SetAddress(leftSensorAddress);
  if (!ret)
    ret = pLeftSensor->Init();

  if (!ret)
    ret = pForwardSensor->SetAddress(forwardSensorAddress);
  if (!ret)
    ret = pForwardSensor->Init();

  if (!ret)
    ret = pFloorSensor->SetAddress(floorSensorAddress);
  if (!ret)
    ret = pFloorSensor->Init();

  return ret;
}

// sets up the laser sensors, the servo and the car
bool Setup()
{
  bool ret = false;

  ret = SetupLaserSensors();

  pServo = new Servo(servoControlPin);
  pCar = new Car(pPCA, pLeftSensor, pRightSensor, pForwardSensor, pFloorSensor, pServo);

  pServo->Move(90); // set servo to the middle
  printf("Setup done.\nREADY!\n");

  return ret;
}

bool Finish()
{
  bool ret = false;
  printf("Finish\n");
  if (pCar != NULL)
    pCar->Stop();
  if (pServo != NULL)
    pServo->Move(90);
  // release all lines
  if (pRightSensor != NULL)
    pRightSensor->Finish();
  if (pLeftSensor != NULL)
    pLeftSensor->Finish();
  if (pForwardSensor != NULL)
    pForwardSensor->Finish();
  if (pFloorSensor != NULL)
    pFloorSensor->Finish();
  gpiod_chip_close(pChip);
  sleep(1);
  return ret;
}

// a thread that is waiting for an Enter keystroke and sets that the stopProgram variable to true
// all other threads watch this variable
void Interruptor()
{
  getchar();
  stopProgram = true;
  printf("Interrupted\n");
}

// another thread that is listening through the microphone and processes the speech through a
// combination of local pre-processing and a remote google speech-to-text processing
void VoiceCommandProcessing(bool voiceCommandEnabled)
{
  if (!voiceCommandEnabled)
    return;

  while (true)
  {
    const char *commandString = pSpeechToText->ProcessSpeech(myGoogleProjectId);
    if (commandString != NULL && strlen(commandString) > 0)
    {
      printf("Command string:%s\n", commandString);
      pCar->ParseVoiceCommand(commandString);
    }
    if (stopProgram)
      break;
    usleep(10000);
  }
  printf("Main loop finished.\n");
}

// the main method for car movement: it launches two threads and then either
// lets the car move on its own executing the pCar->MovingStepForward() method periodically
// or listens to voice commands and executing them using the pCar->FollowVoiceCommands() method
// periodically.
// After a while it stops, and waits for the other threads to finish.
void MoveCar(bool voiceCommandEnabled)
{
  thread ThreadInterruptor(Interruptor);
  thread ThreadVoiceProcessing(VoiceCommandProcessing, voiceCommandEnabled);

  for (int i = 0; i < 500; i++)
  {
    if (voiceCommandEnabled)
    {
      pCar->FollowVoiceCommands();
    }
    else
    {
      pCar->MovingStepForward();
    }

    if (stopProgram)
      break;
  }

  pCar->Stop();
  if (!stopProgram)
  {
    stopProgram = true;
    sleep(3);
  }

  printf("Main loop finished. Waiting for other threads to finish.\n");
  ThreadVoiceProcessing.join();
  ThreadInterruptor.join();
}

int main(int argc, char **argv)
{
  bool ret = false;
  bool voiceCommandEnabled = false;
  bool runCar = false;

  if (::debug)
    printf("Start\n");
  if (!ret)
    ret = Init();
  if (!ret)
    ret = Setup();

  if (!ret)
  {
    if (argc >= 2)
    {
      for (int i = 1; i < argc; i++)
      {
        if (argv[i][0] == '-')
        {
          switch (argv[i][1])
          {
          case 'c':
            pTesting->TestCompass();
            break;
          case 'g':
            pTesting->TestGyro();
            break;
          case 'd':
            debug = true;
            break;
          case 'm':
            if (strlen(argv[i]) > 2)
            {
            std:
              string str = argv[i];
              int speed = stoi(str.substr(2));
              pCar->SetSpeed(speed);
              if (::debug)
                printf("Speed is set to %d\n", speed);
              pTesting->TestTTMotor(5);
            }
            else
            {
              printf("ERROR: no speed is specified\n");
            }
            break;
          case 'l':
            pTesting->TestLaserSensors();
            break;
          case 'f':
            printf("Floor distance: %d\n", pFloorSensor->GetDistanceCm());
            printf("Is the road clear: %B\n", pCar->IsTheRoadClear());
            break;
          case 'r':
            pTesting->TestServo();
            break;
          case 't':
            pTesting->TestTextToSpeech();
            break;
          case 's':
            pTesting->TestSpeechToText();
            break;
          case 'v':
            voiceCommandEnabled = true;
            runCar = true;
            break;
          case 'x':
            voiceCommandEnabled = false;
            runCar = true;
            break;
          case 'h':
            printf("Usage: %s -h(elp)\n", argv[0]);
            printf("Usage: %s -d(ebug messages on)\n", argv[0]);
            printf("Usage: %s -c(ompass testing)\n", argv[0]);
            printf("Usage: %s -m<number:0-100>(otor testing with given speed percentage)\n", argv[0]);
            printf("Usage: %s -l(lasersensor testing)\n", argv[0]);
            printf("Usage: %s -f(loor distance and road-clear testing)\n", argv[0]);
            printf("Usage: %s -r(servo testing)\n", argv[0]);
            printf("Usage: %s -t(ext-to-speech testing)\n", argv[0]);
            printf("Usage: %s -s(peech-to-text testing)\n", argv[0]);
            printf("Usage: %s -v(oice commands: the car follows voice commands. Hit enter to stop.)\n", argv[0]);
            printf("Usage: %s -x(go: the car runs on its own. Hit enter to stop.)\n", argv[0]);
            break;
          default:
            printf("Option not implemented\n");
            break;
          }
        }
      }

      if (runCar)
        MoveCar(voiceCommandEnabled);
    }
  }
  else
  {
    printf("ERROR: Initilization and/or Setup failed\n");
  }

  Finish();
  return 0;
}