#pragma once

#include <PCA9685.h>
#include "lasersensor.h"
#include "servo.h"
#include "ttMotor.h"

class Car
{
public:
  Car(PiPCA9685::PCA9685 *pPCA9685, LaserSensor *pLftSensor, LaserSensor *pRghtSensor,
      LaserSensor *pFwrdSensor, LaserSensor *pFlrSensor, Servo *pTurningServo);
  bool IsMoving();
  void SetSpeed(int percent);
  void MoveForward();
  void MoveBackward();
  void Stop();
  bool IsTheRoadClear();
  void Turn(int direction);
  void FindDirection();
  void MovingStepForward();
  void FollowVoiceCommands();
  void ParseVoiceCommand(const char *voiceString);

private:
  bool is_moving;
  bool last_turn_to_left;
  int speed;
  int max_floor_distance;
  int min_forward_distance;
  LaserSensor *pLeftSensor;
  LaserSensor *pRightSensor;
  LaserSensor *pForwardSensor;
  LaserSensor *pFloorSensor;
  Servo *pServo;
  PiPCA9685::PCA9685 *pPCA;
  TTMotor *pLeftFrontMotor;
  TTMotor *pRightFrontMotor;
  TTMotor *pLeftBackMotor;
  TTMotor *pRightBackMotor;
};