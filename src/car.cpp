#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <chrono>
#include <PCA9685.h>
#include "car.h"
#include "texttospeech.h"
#include "lsm6dsox_lis3mdl.h" 

#define ttLeftFrontSpeedPin 6
#define ttLeftFrontForwardPin 7
#define ttLeftFrontBackwardPin 8

#define ttRightFrontSpeedPin 11
#define ttRightFrontForwardPin 10
#define ttRightFrontBackwardPin 9

#define ttLeftBackSpeedPin 0
#define ttLeftBackForwardPin 1
#define ttLeftBackBackwardPin 2

#define ttRightBackSpeedPin 5
#define ttRightBackForwardPin 4
#define ttRightBackBackwardPin 3

extern bool debug;
extern TextToSpeech *pTextToSpeech;
extern Lsm6dsoxLis3mdl *pLsmLis;

enum class Command
{
    NONE = 0,
    LEFT = 1,
    RIGHT = 2,
    STOP = 3,
    FORWARD = 4,
    GO = 5,
    BACK = 6,
    BACKWORD = 7
};

// the main thread and the voice processing thread communicates through this variable
// yes, I know, this is not 100% thread-safe, but and enum is an integer and its 
// value change should be pretty much indivisible

Command currentCommand = Command::NONE;

Car::Car(PiPCA9685::PCA9685 *pPCA9685, LaserSensor *pLftSensor, LaserSensor *pRghtSensor,
         LaserSensor *pFwrdSensor, LaserSensor *pFlrSensor, Servo *pTurningServo)
{
    this->is_moving = false;
    this->last_turn_to_left = true;
    this->speed = 11;                 // 9 is the 50% of maximum
    this->max_floor_distance = 18;   // cm
    this->min_forward_distance = 30; // cm
    this->pPCA = pPCA9685;
    this->pLeftSensor = pLftSensor;
    this->pRightSensor = pRghtSensor;
    this->pForwardSensor = pFwrdSensor;
    this->pFloorSensor = pFlrSensor;
    this->pServo = pTurningServo;

    // initalize TT motors
    this->pLeftFrontMotor = new TTMotor(this->pPCA, ttLeftFrontSpeedPin, ttLeftFrontForwardPin, ttLeftFrontBackwardPin);
    this->pRightFrontMotor = new TTMotor(this->pPCA, ttRightFrontSpeedPin, ttRightFrontForwardPin, ttRightFrontBackwardPin);
    this->pLeftBackMotor = new TTMotor(this->pPCA, ttLeftBackSpeedPin, ttLeftBackForwardPin, ttLeftBackBackwardPin);
    this->pRightBackMotor = new TTMotor(this->pPCA, ttRightBackSpeedPin, ttRightBackForwardPin, ttRightBackBackwardPin);
    this->Stop();
}

bool Car::IsMoving()
{
    return this->is_moving;
}

void Car::SetSpeed(int percent)
{
    this->speed = (int)((((double)percent) / 100.0) * 19.0); // 19 max speed

    if (::debug)
        printf("Car speed is set to absolute %d\n", this->speed);
}

void Car::MoveForward()
{
    if (::debug)
        printf("Moving forward\n");

    pTextToSpeech->Talk("Moving forward.");
    this->is_moving = true;

    // drive TT motors
    this->pLeftFrontMotor->MoveForward(this->speed);
    this->pLeftBackMotor->MoveForward(this->speed);
    this->pRightFrontMotor->MoveForward(this->speed);
    this->pRightBackMotor->MoveForward(this->speed);
}

void Car::MoveBackward()
{
    if (::debug)
        printf("Moving backward\n");

    pTextToSpeech->Talk("Moving backward.");
    this->is_moving = true;

    // drive TT motors
    this->pLeftFrontMotor->MoveBackward(this->speed);
    this->pLeftBackMotor->MoveBackward(this->speed);
    this->pRightFrontMotor->MoveBackward(this->speed);
    this->pRightBackMotor->MoveBackward(this->speed);
}

void Car::Stop()
{
    if (this->is_moving)
    {
        if (::debug)
            printf("Stopping\n");

        pTextToSpeech->Talk("Stopping.");
        this->is_moving = false;
        // stop TT motors

        this->pLeftFrontMotor->Stop();
        this->pRightFrontMotor->Stop();
        this->pLeftBackMotor->Stop();
        this->pRightBackMotor->Stop();
    }
}

bool Car::IsTheRoadClear()
{ // returns true if the road ahead is clear by all 3 forward facing sensors
    bool ret = false;
    int dist = 0;
    pServo->Move(90); // set servo to middle position facing forward

    if ((dist = this->pForwardSensor->GetDistanceCm()) > this->min_forward_distance)
    {
        if ((dist = this->pLeftSensor->GetDistanceCm()) > this->min_forward_distance)
        {
            if ((dist = this->pRightSensor->GetDistanceCm()) > this->min_forward_distance)
            {
                if (::debug)
                    printf("Road is clear\n");
                ret = true;
            }
            else
            {
                if (::debug)
                    printf("Road is not clear: right distance: %dcm\n", dist);
            }
        }
        else
        {
            if (::debug)
                printf("Road is not clear: left distance: %dcm\n", dist);
        }
    }
    else
    {
        if (::debug)
            printf("Road is not clear: forward distance: %dcm\n", dist);
    }

    if (!ret)
        pTextToSpeech->Talk("Road is not clear");

    return ret;
}

void Car::Turn(int direction)
{ // This turns the car. First, it goes back a little bit (since we assume that it saw an obstacle
  // then it turns. 90 degrees is straight ahead.
  // Below 90 is turning right, above 90 is turning left.
  // It uses the Gyro as a feedback about how much it already turned
  // it is pretty useless - the gyro is not reliable
  //
  // I also tried to use various "fusion" algorithms to improve the results, that is,
  // Mahony, Madgwick, and NXPfusion but none of them made any significant improvement.
    printf("**********Turning starts\n");

    // turn TT motors
    if (::debug)
        printf("Turning %s\n", (direction > 90 ? "left" : "right"));

    this->MoveBackward();
    usleep(200000); // delay 200ms
    pLsmLis->CalculateGyroAveBias(); // this takes 100ms
    
    if (direction > 90) // turning left
    {
        pTextToSpeech->Talk("Turning left");
        this->last_turn_to_left = true;
        this->pLeftFrontMotor->MoveBackward(10);
        this->pLeftBackMotor->MoveBackward(10);
        this->pRightFrontMotor->MoveForward(10);
        this->pRightBackMotor->MoveForward(10);
    }
    else // turning right
    {
        pTextToSpeech->Talk("Turning right");
        this->last_turn_to_left = false;
        this->pLeftFrontMotor->MoveForward(10);
        this->pLeftBackMotor->MoveForward(10);
        this->pRightFrontMotor->MoveBackward(10);
        this->pRightBackMotor->MoveBackward(10);
    }
    double gyroGoal = (double) (direction - 90); // for Gyro, a left turn is pozitive, and right turn is negative
    pLsmLis->lastGyroAngles = { 0, 0, 0 };
    // now, the car is continuously turning

    std::chrono::steady_clock::time_point turnStartTime = std::chrono::steady_clock::now();
    for (int i = 0; i < 50; i++)
    {
        std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

        std::chrono::milliseconds turnDuration = std::chrono::duration_cast<std::chrono::milliseconds>(startTime - turnStartTime);
        if(turnDuration.count() >= 3000)
        {
            printf("Turn took more than 2s, so we stop and get out.\n");
            break;
        }
        if (::debug)
            printf("Checking for obstacles\n");

        if (this->IsTheRoadClear()) // this takes at least 100ms, at most 360ms
        {
            std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
            std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            pLsmLis->lastGyroAngles = pLsmLis->GetGyroAngles(duration.count(), pLsmLis->lastGyroAngles);
            // printf("Road is clear: current turn angle is %f, goal is %f elapsed time:%dms\n",
            //         pLsmLis->lastGyroAngles.z, gyroGoal, duration.count());
            if((gyroGoal > 0 && pLsmLis->lastGyroAngles.z >= gyroGoal) ||
                (gyroGoal < 0 && pLsmLis->lastGyroAngles.z <= gyroGoal))
            {
                //printf("Turning goal reached\n");
                this->Stop();
                break;
            }            
        }
        else
        {
            std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
            std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            pLsmLis->lastGyroAngles = pLsmLis->GetGyroAngles(duration.count(), pLsmLis->lastGyroAngles);
            // printf("Road is NOT clear: current turn angle is %f, goal is %f elapsed time:%dms\n",
            //         pLsmLis->lastGyroAngles.z, gyroGoal, duration.count());
        }
    }
    this->Stop();
}

void Car::FindDirection()
{ // this looks around with the servo and the front-middle sensor
  // and tries to figure out which direction to proceed
    if (::debug)
        printf("FindDirection\n");
    int directions1[] = {60, 30, 0, 120, 150, 180};
    int directions2[] = {120, 150, 180, 60, 30, 0};
    int size = sizeof(directions1) / sizeof(directions1[0]);

    // to make sure that we don't try to turn all the time in the same direction
    int *directions = this->last_turn_to_left ? directions1 : directions2;

    for (int i = 0; i < size; i++)
    {
        pServo->Move(directions[i]);
        if (this->pForwardSensor->GetDistanceCm() > this->min_forward_distance)
        {
            if (::debug)
                printf("Forward sensor found way forward in direction %d, that is: %s\n",
                       directions[i], (directions[i] > 90 ? "left" : "right"));
            pServo->Move(90); // turn servo ahead
            this->Turn(directions[i]);
            if (this->IsTheRoadClear())
            {
                if (this->pFloorSensor->GetDistanceCm() <= this->max_floor_distance)
                {
                    this->MoveForward();
                }
                else
                {
                    if (::debug)
                        printf("No floor: distance: %d\n", this->pFloorSensor->GetDistanceCm());
                    pTextToSpeech->Talk("No floor ahead");
                }
            }
            break;
        }
    }
}

void Car::MovingStepForward()
{ // this is the main method for the car to move forward on its own.
  // The car moves forward while checking if there is floor ahead of it.
  // If it finds an obstacle, it will stop and try to find a new direction
    if (::debug)
        printf("MovingStepForward\n");

    float floorDistance = this->pFloorSensor->GetDistanceCm();

    if (floorDistance > this->max_floor_distance)
    {
        if (::debug)
            printf("No floor: distance: %d\n", this->pFloorSensor->GetDistanceCm());

        pTextToSpeech->Talk("No floor ahead");
        if (this->is_moving)
            this->Stop();
    }
    else
    {
        if (!this->IsTheRoadClear())
        {
            if (this->is_moving)
                this->Stop();
            this->FindDirection();
        }
        else
        {
            if (::debug)
                printf("Should be moving forward\n");

            if (!this->IsMoving())
                this->MoveForward();
        }
    }
    usleep(100000); // sleep 0.1 seconds
}

void Car::FollowVoiceCommands()
{ // the main method to follow voice commands
    Command localCommand = currentCommand;
    if (localCommand == Command::NONE)
    { // no command, so just check the floor and check for obstacles
        if (this->IsMoving())
        {
            float floorDistance = this->pFloorSensor->GetDistanceCm();

            if (floorDistance > this->max_floor_distance)
            {
                if (::debug)
                    printf("No floor: distance: %d\n", this->pFloorSensor->GetDistanceCm());
                pTextToSpeech->Talk("No floor ahead");
                if (this->is_moving)
                    this->Stop();
            }
            else
            {
                if (!this->IsTheRoadClear())
                {
                    this->Stop();
                }
            }
        }
        else
        {
            // there is nothing to do, we are just waiting for a voice command
        }
    }
    else
    {
        if (::debug)
            printf("Processing voice command: %d\n", localCommand);

        switch ((int)localCommand)
        {
        case 0:
            break;
        case 1:
            this->Turn(135);
            break;
        case 2:
            this->Turn(45);
            break;
        case 3:
            this->Stop();
            break;
        case 4:
        case 5:
            this->MoveForward();
            break;
        case 6:
        case 7:
            this->MoveBackward();
            sleep(2);
            this->Stop(); // since we are blind backward
            break;
        default:
            // nothing to do
            break;
        }

        currentCommand = Command::NONE;
    }
    usleep(250000); // sleep 0.25 seconds
}

void Car::ParseVoiceCommand(const char *voiceString)
{ // this method is called from the voice processing thread
  // and just translates the voice command string to an integer (enum)
    const char *commandStrings[] = {"left", "right", "stop", "forward", "go", "back", "backward"};
    int size = sizeof(commandStrings) / sizeof(commandStrings[0]);
    for (int i = 0; i < size; i++)
    {
        if (strstr(voiceString, commandStrings[i]) != NULL)
        {
            currentCommand = (Command)(i + 1);
            break;
        }
    }
}
