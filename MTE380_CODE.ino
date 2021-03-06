#include <NewPing.h>
#include <Servo.h>
#include <Wire.h>
#include "MPU6050.h"

// #define DEBUG_PRINT // Enable for sensor data prints

/*
 *  Pin Definitions 
 */
#define US_FRONT_POWER 29
#define US_FRONT_GROUND 23
#define US_FRONT_TRIGGER_PIN 25
#define US_FRONT_ECHO_PIN 27
#define US_LEFT_POWER 37
#define US_LEFT_GROUND 31
#define US_LEFT_TRIGGER_PIN 35
#define US_LEFT_ECHO_PIN 33
#define US_RIGHT_POWER 28
#define US_RIGHT_GROUND 22
#define US_RIGHT_TRIGGER_PIN 26
#define US_RIGHT_ECHO_PIN 24
#define MOTOR_LEFT_PWM_PIN 9
#define MOTOR_RIGHT_PWM_PIN 6
#define INT_PIN 19
#define SWITCH_PIN 53
#define SWITCH_POWER 51

/*
 *  Ultrasonic 
 */
#define US_MAX_DISTANCE 300 // Maximum distance (in cm) to ping. width 7.5 x length 8.5 ft = 230x260
#define US_NUM_SAMPLE 7
#define CM_WALL_TO_RAMP 34
#define CM_POLE_TO_WALL 49
#define DETECT_TOLERANCE 3

#define NUM_PITCH_SAMPLE 5

/*
 *  Preset Motor Speed 
 */
typedef enum 
{
    L_STOP = 92,
    L_FWD_SLOW = 85,
    L_FWD_50 = 55,
    L_FWD_75 = 27,
    L_FWD_MAX = 0,
    L_REV_25=106,
    L_REV_50=135,
    L_REV_75,
    L_REV_100 = 180,
    L_FWD_TEST = 0
} LeftMotorSpeed;

typedef enum 
{
    R_STOP = 92,
    R_FWD_SLOW = 100,
    R_FWD_50 = 131,
    R_FWD_75 = 136,
    R_FWD_MAX = 143,
    R_REV_25=80,
    R_REV_50=46,
    R_REV_75,
    R_REV_100 = 0,
    R_FWD_TEST = 180
} RightMotorSpeed;

/*
 *  Object Initialization
 */
NewPing frontUS(US_FRONT_TRIGGER_PIN, US_FRONT_ECHO_PIN, US_MAX_DISTANCE);
NewPing leftUS(US_LEFT_TRIGGER_PIN, US_LEFT_ECHO_PIN, US_MAX_DISTANCE);
NewPing rightUS(US_RIGHT_TRIGGER_PIN, US_RIGHT_ECHO_PIN, US_MAX_DISTANCE);
Servo lMotor;
Servo rMotor;

/*
 *  Machine States
 */
typedef enum {
    ST_STOP = 0,
    ST_DRIVE_TO_WALL,
    ST_UP_WALL,
    ST_TOP_WALL,
    ST_DOWN_WALL_1,
    ST_DOWN_WALL_2,
    ST_PRE_POLE_DETECTION_1,
    ST_PRE_POLE_DETECTION_2,
    ST_PRE_POLE_DETECTION_3,
    ST_PRE_POLE_DETECTION_4,
    ST_POLE_DETECT,
    ST_TURN_TOWARD_POLE,
    ST_DRIVE_TO_POLE,
    ST_FINISH,
    ST_DEBUG
} States;

/*
 *  Global Variables
 */
int switchReading;
//States INITIAL_STATE = ST_PRE_POLE_DETECTION_4;

States INITIAL_STATE = ST_DRIVE_TO_WALL;
States GLOBAL_STATE = INITIAL_STATE;
unsigned long STATE_START_TIME = 0;

//Ultrasonic variables
int leftDis_old = 1, frontDis_old = 1, rightDis_old = 1;
int leftDis = 0, frontDis = 0, rightDis = 0;

//Driving logic variables
float targetYaw = 0;
float tmpYaw;
float startingPitch = 0;
long endingTime;
int distToPole = 20;
float prePitch[NUM_PITCH_SAMPLE];
int prePitchIndex = 0;

/*
 *  Helper Functions
 */
void pinSetup()
{
  // Set up the interrupt pin, its set as active high, push-pull
  pinMode(INT_PIN, INPUT);
  digitalWrite(INT_PIN, LOW);

  pinMode(SWITCH_PIN, INPUT);
  digitalWrite(SWITCH_POWER, HIGH);

  pinMode(US_FRONT_GROUND, OUTPUT);
  pinMode(US_LEFT_GROUND, OUTPUT);
  pinMode(US_RIGHT_GROUND, OUTPUT);
  pinMode(US_FRONT_POWER, OUTPUT);
  pinMode(US_LEFT_POWER, OUTPUT);
  pinMode(US_RIGHT_POWER, OUTPUT);

  digitalWrite(US_FRONT_GROUND, LOW);
  digitalWrite(US_LEFT_GROUND, LOW);
  digitalWrite(US_RIGHT_GROUND, LOW);
  digitalWrite(US_FRONT_POWER, HIGH);
  digitalWrite(US_LEFT_POWER, HIGH);
  digitalWrite(US_RIGHT_POWER, HIGH);

  lMotor.attach(MOTOR_LEFT_PWM_PIN);
  lMotor.write(L_STOP);
  rMotor.attach(MOTOR_RIGHT_PWM_PIN);
  rMotor.write(R_STOP);
}

// Call this whenever polling occurs
void update()
{
  switchReading = digitalRead(SWITCH_PIN);
  if (switchReading == HIGH)
  {
    GLOBAL_STATE = ST_STOP;
  } else 
  {
    if (GLOBAL_STATE == ST_STOP)
      GLOBAL_STATE = INITIAL_STATE;
  }
  
  imuUpdate();
  motorUpdate();
}

/*
 *  Main Loop */
void setup()
{
  Serial.begin(38400);
  Wire.begin();
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega8__) || defined(__AVR_ATmega328P__)
  cbi(PORTC, 4);
  cbi(PORTC, 5);
#else
  cbi(PORTD, 0);
  cbi(PORTD, 1);
#endif  
  
  pinSetup();
  imuSetup();
}

void loop()
{  
  update();

  switch(GLOBAL_STATE)
  {
    case ST_STOP: 
    {
      tmpYaw = yaw;
      stopBothMotors();
      break;
    } 
    case ST_DRIVE_TO_WALL:
      driveToWallState();
      break;
    case ST_UP_WALL:
      upWallState();
      break;
    case ST_TOP_WALL:
      topWallState();
      break;
    case ST_DOWN_WALL_1:
      downWall1State();
      break;
    case ST_DOWN_WALL_2:
      downWall2State();
      break;
    case ST_PRE_POLE_DETECTION_1:
      prePoleDetection1();
      break;
    case ST_PRE_POLE_DETECTION_2:
      prePoleDetection2();
      break;
    case ST_PRE_POLE_DETECTION_3:
      prePoleDetection3();
      break;
    case ST_PRE_POLE_DETECTION_4:
      prePoleDetection4();
      break;
    case ST_POLE_DETECT:
      poleDection();
      break;
    case ST_TURN_TOWARD_POLE:
      turnTowardPole();
      break;
    case ST_DRIVE_TO_POLE:
      driveToPole();
      break;
    case ST_FINISH:
      stopRobot();
      break;
    case ST_DEBUG:
    {
      setLeftMotorSpeed(L_FWD_SLOW);
      setRightMotorSpeed(R_FWD_SLOW);
      break;
    } 
    default: 
      break;    
  }
}
