/*=====================================================================
     Quadcopter library
     OpenSourceQuad
     -------------------------------------------------------------------*/
/*================================================================================
 
     Author		: Brandon Riches
     Date		: August 2013
     License		: GNU Public License
 
     Outdated library, has some basic functions related to LEDs and sensor calibration
 
     Copyright (C) 2013  Brandon Riches
 
     This program is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.
     
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.
     
     You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
     -----------------------------------------------------------------------------*/
#ifndef QUADCOPTER_H_INCLUDED
#define QUADCOPTER_H_INCLUDED


#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Wire.h>
#include <math.h>

/*=========================================================================
 General IO pins
 -----------------------------------------------------------------------*/
#define GREEN_LED1 22
#define GREEN_LED2 24
#define GREEN_LED3 26
#define YELLOW_LED1 23
#define YELLOW_LED2 25
#define YELLOW_LED3 27


/*=========================================================================
 Polling Rates.
 -----------------------------------------------------------------------*/

#define fastPeriod (0)
#define	_70HzPeriod (14286)
#define _20HzPeriod (50000)
#define _10HzPeriod (100000)
#define _1HzPeriod  (1000000)


/***************************************************************************
 *! FUNCTIONS
 ***************************************************************************/

void statusLED(int LED_SEL);

void getInitialOffsets( struct kinematicData *kinematics, IMU_accel accel, IMU_mag mag, IMU_gyro gyro)
{
        /**************************************************************************/
        //! Gets the initial offsets in both sensors to accomodate board mount
        /**************************************************************************/
        // # of data sets to consider when finding offsets
        int offset_counter = 1000;  
        int counter = 0;
        double acceldata[3];
        double gyrodata[3];

        while(counter < offset_counter)
        {
		sensors_event_t accel_event;
		accel.getEvent(&accel_event);

		acceldata[0] = accel_event.acceleration.x; // NOTE: Quick fix! Careful!
		acceldata[1] = accel_event.acceleration.y;
		acceldata[2] = accel_event.acceleration.z - SENSORS_GRAVITY_STANDARD;

		sensors_event_t gyro_event;
		gyro.getEvent(&gyro_event);

		gyrodata[0] = gyro_event.gyro.x;
		gyrodata[1] = gyro_event.gyro.y;
		gyrodata[2] = gyro_event.gyro.z;

		kinematics->io_ax += acceldata[0];
		kinematics->io_ay += acceldata[1];
		kinematics->io_az += acceldata[2];

		kinematics->io_wx += gyrodata[0];
		kinematics->io_wy += gyrodata[1];
		kinematics->io_wz += gyrodata[2];

		if ((kinematics->io_ax==0)&&(kinematics->io_ay == 0)&&(kinematics->io_az == 0))
		{
			statusLED(-1);	 // Critical error, accelerometer is NOT working
			while(1);
		}

		counter = counter + 1 ;
		delay(1);
        }

        kinematics->io_ax /= offset_counter;
        kinematics->io_ay /= offset_counter;
        kinematics->io_az /= offset_counter;
        
        kinematics->io_wx /= offset_counter;
        kinematics->io_wy /= offset_counter;
        kinematics->io_wz /= offset_counter;
};

bool initSensor(IMU_accel accel, IMU_mag mag, IMU_gyro gyro, struct kinematicData *kinematics)
{
	/**************************************************************************/
	//! Initializes the various sensors and instruments
	/**************************************************************************/
	gyro.begin();
	accel.begin();
	mag.begin();

	return true;
};


void statusLED(int LED_SEL)
{
	switch(LED_SEL)
	{
	
	case -1:
		digitalWrite(GREEN_LED1, LOW);
		digitalWrite(GREEN_LED2, LOW);
		digitalWrite(GREEN_LED3, LOW);
		digitalWrite(YELLOW_LED1, HIGH);
		digitalWrite(YELLOW_LED2, HIGH);
		digitalWrite(YELLOW_LED3, HIGH);
		break;
			
			
	case 0:
		digitalWrite(GREEN_LED1, HIGH);
		digitalWrite(GREEN_LED2, HIGH);
		digitalWrite(GREEN_LED3, HIGH);
		digitalWrite(YELLOW_LED1, LOW);
		digitalWrite(YELLOW_LED2, LOW);
		digitalWrite(YELLOW_LED3, LOW);
		break;
			
	case 1:
		// Green LED. Should indicate successes
		digitalWrite(GREEN_LED1, HIGH);
		digitalWrite(GREEN_LED2, LOW);
		digitalWrite(GREEN_LED3, LOW);
		digitalWrite(YELLOW_LED1, LOW);
		digitalWrite(YELLOW_LED2, LOW);
		digitalWrite(YELLOW_LED3, LOW);
		break;

	case 2:
		digitalWrite(GREEN_LED1, LOW);
		digitalWrite(GREEN_LED2, HIGH);
		digitalWrite(GREEN_LED3, LOW);
		digitalWrite(YELLOW_LED1, LOW);
		digitalWrite(YELLOW_LED2, LOW);
		digitalWrite(YELLOW_LED3, LOW);
		break;

	case 3:
		digitalWrite(GREEN_LED1, LOW);
		digitalWrite(GREEN_LED2, LOW);
		digitalWrite(GREEN_LED3, HIGH);
		digitalWrite(YELLOW_LED1, LOW);
		digitalWrite(YELLOW_LED2, LOW);
		digitalWrite(YELLOW_LED3, LOW);
		break;

	case 4:
		digitalWrite(GREEN_LED1, LOW);
		digitalWrite(GREEN_LED2, LOW);
		digitalWrite(GREEN_LED3, LOW);
		digitalWrite(YELLOW_LED1, HIGH);
		digitalWrite(YELLOW_LED2, LOW);
		digitalWrite(YELLOW_LED3, LOW);
		break;
			
	case 5:
		digitalWrite(GREEN_LED1, LOW);
		digitalWrite(GREEN_LED2, LOW);
		digitalWrite(GREEN_LED3, LOW);
		digitalWrite(YELLOW_LED1, LOW);
		digitalWrite(YELLOW_LED2, HIGH);
		digitalWrite(YELLOW_LED3, LOW);
		break;
			
	case 6:
		digitalWrite(GREEN_LED1, LOW);
		digitalWrite(GREEN_LED2, LOW);
		digitalWrite(GREEN_LED3, LOW);
		digitalWrite(YELLOW_LED1, LOW);
		digitalWrite(YELLOW_LED2, LOW);
		digitalWrite(YELLOW_LED3, HIGH);
		break;
	}
};

void heartbeat()
{
    static int mag = 10;
    static int dir = 1;
    static unsigned long timestamp = 0;
    int pin = 9;
    
    if(millis() - timestamp > 5)
    {
        
        if( (mag + 2*dir) >= 255 || (mag+2*dir) <= 1)
            dir *= -1;
        mag += 2*dir;
        timestamp = millis();
    }
    
    analogWrite(pin,mag);
};


#endif // QUADCOPTER_H_INCLUDED

