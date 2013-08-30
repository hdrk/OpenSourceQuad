//*****************************************************/
/*//*//*   OpenSourceQuad   *//*//*

Library designed to manage, update, and control the
state of quadcopter

Author  : Brandon Riches
Date     :	June 2013



******************************************************/
#ifndef OSQ_QUADCOPTER_H_INCLUDED
#define OSQ_QUADCOPTER_H_INCLUDED


#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <OSQ_QuadGlobalDefined.h>
#include <OSQ_Kinematics.h>
#include <OSQ_SENSORLIB.h>
#include <OSQ_Motors.h>

#include <I2C.h>
#include <Wire.h>
#include <math.h>
#include <Servo.h>




/*===============================================
Time keeping for polling and interrupts
-----------------------------------------------------------------------*/
static double tpoll1;
static double tpoll2;
static double tpoll3;
static double tpoll4;


/***************************************************************************
 *! @FUNCTIONS
 ***************************************************************************/
bool initSensor(	SENSORLIB_accel accel,
					SENSORLIB_mag mag,
					SENSORLIB_gyro gyro,
					struct kinematicData *kinematics);

void mainProcess(	double pitchPID_out,
					double rollPID_out,
					class SENSORLIB_accel *accel,
					class SENSORLIB_mag	*mag,
					class SENSORLIB_gyro	*gyro,
					struct kinematicData *kinematics,
					struct fourthOrderData *fourthOrderXAXIS,
					struct fourthOrderData *fourthOrderYAXIS,
					struct fourthOrderData *fourthOrderZAXIS,
					struct OSQ_MotorControl *MotorControl);


void ERROR_LED(		int LED_SEL);

void getInitialOffsets(struct kinematicData *kinematics,
						SENSORLIB_accel accel,
						SENSORLIB_mag mag,
						SENSORLIB_gyro gyro);





#endif // OSQ_QUADCOPTER_H_INCLUDED