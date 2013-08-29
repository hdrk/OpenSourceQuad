 /*=========================================================================
Name: QCopterMain.ino
Authors: Brandon Riches, Branden Yue, Andrew Coulthard
Date: 2013


TODO:
1) Chebyshev filter. 
2) Fix gyro DC drift. (Measure over time, possibly)
3) Tune PID using Zieglerâ€“Nichols method
  - http://en.wikipedia.org/wiki/Ziegler%E2%80%93Nichols_method .
        
Copyright stuff from all included libraries that I didn't write

  I2C.h   - I2C library
  Copyright (c) 2011-2012 Wayne Truchsess.  All right reserved.
  
  PID Library
  Brett Beauregard, br3ttb@gmail.com
  
  Adafruit_GPS
  
  Servo
  
  SoftwareSerial
  
  Wire
  
  SD
    -----------------------------------------------------------------------*/
#include <QuadGlobalDefined.h>
#include <Kinematics.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SENSORLIB.h>
#include <Quadcopter.h>
#include <PID_v1.h>
#include <OseppGyro.h>
#include <I2C.h>
#include <Wire.h>
#include <Servo.h>
#include <SD.h>

File logfile;

/*=========================================================================
    PID output variables and desired setpoints, and settings
    -----------------------------------------------------------------------*/
/*=========================================================================
    State variables
    - The program didnt like having these in the class.
    -----------------------------------------------------------------------*/
// PID output for alpha and beta, PID setpoint vars for alpha and beta
double set_a, aPID_out; 		  
double set_b, bPID_out;	

// Sample time for PID controllers in ms	      	
int PID_SampleTime = 10;

// Output limits on the PID controllers.
// Both the alpha and the beta controller use these limits.
// They represent the maximum absolute value that the PID equation could reach,
// regardless of what the gain coefficients are. 
int PID_OutLims[] = {-1000,1000};


/*=========================================================================
    Classes and important structures
    -----------------------------------------------------------------------*/
SENSORLIB_accel   accel;       		// Accel class
SENSORLIB_mag	  mag;			// Mag class
OseppGyro         gyro;			// Gyro class
Servo             motor1;		// Motor 1
Servo             motor2;		// Motor 2
Servo             motor3;		// Motor 3
Servo             motor4;		// Motor 4
fourthOrderData   fourthOrderXAXIS,
		  fourthOrderYAXIS,
		  fourthOrderZAXIS;

kinematicData	  kinematics;
    
// This is the main class for the Quadcopter driver. 
// Contains most of the methods, variables and other information about state. 
// Refer to Quadcopter.h for info, or @ http://tinyurl.com/kl6uk87 . 
Quadcopter Quadcopter;

// Constructors for the PID controllers
// The 4th, 5th, and 6th args in the constructors are, respectively:
// - Proportional gain
// - Integral gain
// - Derivative gain
#define Kp 24    // Was 24, 48, 0.75
#define Ki 48    // 5, 1, 1
#define Kd 0.75  // Classic PID Z-N method
PID aPID(&kinematics.pitch,  &aPID_out,  &set_a,   Kp,  Ki,  Kd,  DIRECT);
PID bPID(&kinematics.roll,   &bPID_out,  &set_b,   Kp,  Ki,  Kd,  DIRECT);


/*=========================================================================
    Function declarations
    -----------------------------------------------------------------------*/
void PID_init();           
boolean XBee_read();
int XBee_send(String data);
void get_telemetry(double* set_a, double* set_b);
void logfileStart();


/*=========================================================================
    Telemetry vars and codes
    -----------------------------------------------------------------------*/
    
SoftwareSerial XBeeSerial =  SoftwareSerial(43, 41);
// Radio transmitted instructions are stored in this character array.
char XBeeArray[3];

boolean   STOP_FLAG;
boolean   START_FLAG;
int       dataInXBA;

// Telemetry codes
String      STOP_MSG  =    "!SS";
String      START_MSG =    "!GG";


/*=========================================================================
    Main Setup
    -----------------------------------------------------------------------*/
void setup()
{ 
  unsigned long t1, t2;
  double elaps;
  t1 = micros();
  
  // Initialize the main serial UART for output. 
  Serial.begin(19200); 
  
  // Initialize the radio comms serial port for communication with the XBee radios
  //XBeeSerial.begin(19200);
  
  Serial.println(" ");
  
  // Initialize these pins for digital output.
  // They are used in the ERROR_LED function
  // Use ERROR_LED(1) for success,
  //     ERROR_LED(2) for warnings,
  //     ERROR_LED(3) for critical fail (has a while(1)).
  pinMode(GREEN_LED,   OUTPUT);
  pinMode(YELLOW_LED,  OUTPUT);
  pinMode(RED_LED,     OUTPUT);
  
  // Turn on the yellow LED to signify start of setup
  Quadcopter.ERROR_LED(2);
  
  // Open a .txt file for data logging and debugging
  logfileStart();
    
  // Initialize the sensors.
  // Sensors include: 
  //   - Gyro (InvenSense MPU3050)
  //   - Accel/Magnetometer (LSM303)
  //   - GPS module (Adafruit Ultimate)
  //   - RTC Module
  while(!Quadcopter.initSensor());
  
  // Initialize the PID controllers. This is a sub-function, below loop.
  PID_init();   
  
  // Initialize motors. This turns the motors on, and sets them all to a speed
  // just below take-off speed.
  // Enter a %, from 0 - 100
  // If you enter more than 40%, thats pushing it. 
  Quadcopter.ERROR_LED(2);
  Quadcopter.initMotors(20);
  delay(1000);
  
  
  // Set both the alpha and beta setpoints to 0. 
  // This is just for starters, eventually, pathing will modify the
  // setpoints en-route to various locations. 
  set_a = 0;		               
  set_b = 0;  
  
  // Initialize the fourth order struct
  setupFourthOrder();
  
  // Wait for start confirmation from computer
  START_FLAG = false;

  Serial.println("Got past here no problem");
  
  // Turn on the green LED to signify end of setup. 
  Quadcopter.ERROR_LED(1);  
  
  
  // Timekeeping
  t2 = micros();
  elaps = (t2 - t1)/1000;
  Serial.print("Setup complete!  Total time (ms): ");
  Serial.println(elaps,4);
  Serial.println("");
  
}

/*=========================================================================
    MAIN CONTROL LOOP
    -----------------------------------------------------------------------*/
void loop()                          
{
  // This is the main runtime function included in the Quadcopter class.
  //
  // It includes a number of different priorities, based on the time elapsed
  // since it was last called (This is because various sensors or events may
  // have different time intervals between them). 
  // 
  // In the most basic priority, the function gathers new magnetometer, 
  // accelerometer, and gryo data. It then runs a basic moving average on these
  // data to smooth them. Then, it uses a complementary filter to help obtain 
  // more accurate readings of angle
  Quadcopter.update(aPID_out, bPID_out);  

  // Updates the PID controllers. They return new outputs based on current
  // and past data. These outputs are used to decide what the motor speeds should be set to.
  
  if (millis() > 10000)
  {
    aPID.Compute();
    bPID.Compute();
  }  
  // We print all the data to the SD logfile, for debugging purposes. 
  // Once a completely working build is finished, this may or may not be removed for
  // the sake of speed.
  logfile = SD.open("run_log.txt", FILE_WRITE);
  if (logfile)
  {
    logfile.print(micros());
    logfile.print(",");
    logfile.print(Quadcopter.ax);
    logfile.print(",");
    logfile.print(Quadcopter.ay);
    logfile.print(",");
    logfile.print(Quadcopter.az);
    logfile.print(",");
//    logfile.print(Quadcopter.wx);
//    logfile.print(",");
//    logfile.print(Quadcopter.wy);
//    logfile.print(",");
//    logfile.print(Quadcopter.wz);
//    logfile.print(",");
    logfile.print(kinematics.pitch);
    logfile.print(",");
    logfile.print(kinematics.roll);
    logfile.print(",");
//    logfile.print(Quadcopter.motor1s);
//    logfile.print(",");
//    logfile.print(Quadcopter.motor2s);
//    logfile.print(",");
//    logfile.print(Quadcopter.motor3s);
//    logfile.print(",");
//    logfile.print(Quadcopter.motor4s);
//    logfile.print(",");
    logfile.print(aPID_out);
    logfile.print(",");
    logfile.println(bPID_out);
  }
  logfile.close();
  
  // Close the file after sometime  of logging.
  if (millis() >= 60000)
  {
    while(1);
  }

  Serial.print(kinematics.pitch);
  Serial.print(" ");
  Serial.print(kinematics.roll);
  Serial.print(" ");
  Serial.print(Quadcopter.az);
  Serial.print(" ");
  Serial.println(Quadcopter.ay);
  
}
/**! @ END MAIN CONTROL LOOP. @ !**/


/*=========================================================================
    logfileStart
    - Initializes a .txt on the uSD
    -----------------------------------------------------------------------*/
void logfileStart()
{
  // Initialize SD card
  Serial.print("Initializing SD card...");
  
  // Hardware SS pin must be output. 
  pinMode(SS, OUTPUT);
  
  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    Quadcopter.ERROR_LED(3);
    return;
  }
  Serial.println("initialization done.");
  
  // If the file exists, we want to delete it. 
  if (SD.exists("run_log.txt"))
  {
    SD.remove("run_log.txt");
  }
  
  // Open the file for writing, here just for a title.
  logfile = SD.open("run_log.txt", FILE_WRITE);
  
  // if the file opened okay, write to it:
  if (logfile) {
    Serial.print("Writing to run_log.txt...");
    logfile.println("Time,Ax,Ay,Az,Wx,Wy,Wz,Alpha,Beta,Motor 1,Motor 2,Motor 3,Motor 4,APID,BPID");
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening run_log.txt");
  }
  
  logfile.close();

}

/*=========================================================================
    get_telemetry
    - Retrieves the latest serial data available on the buffer
    - Parses the data if enough bytes are available
    -----------------------------------------------------------------------*/
void get_telemetry(double* set_a, double* set_b)
{
  String myMsg = "";
  Quadcopter.ERROR_LED(2);
  // If there is data: read, interpret and act on it
  if(XBee_read())
  {
    // Interpret data
    for (int i = 0; i < 3; i++)
    {
      myMsg += XBeeArray[i];
    }
    
    if (myMsg == STOP_MSG)
    {
      Quadcopter.initMotors(0);
      STOP_FLAG = true;
      
      // STOP Message recieved. This is the software implementation of a safety switch
      while (STOP_FLAG)
      {
      }
    }
    
    
    
    
    
  }
  
  // Done interpreting and reading. 
  Quadcopter.ERROR_LED(1);
}
/*=========================================================================
    XBee_send(String data)
    ----------------------------------------------------------------------*/
int XBee_send(String data)
{
  int bytesSent = XBeeSerial.print(data);
  
  return bytesSent;
}

/*=========================================================================
    XBee_read()
    - Reads serialdata from the modem into a storage buffer
    -----------------------------------------------------------------------*/
boolean XBee_read()
{
  boolean check = false;
  // Get the number of bytes available to read
  int bytes = XBeeSerial.available();
  if (bytes >= 3)
  {
    check = true;
    // Read the serial data from the modem into the array
    // Read three bytes at a time (message length = 3)
    for(int i = 0; i < 3; i++)
    {
      XBeeArray[i] = XBeeSerial.read();
    }
  }
  // The first char has to be '!'
  
  for (int i = 0; i < 3; i++)
  {
    if (XBeeArray[i] == '!')
    {
      char reorganize[3];
      int start_msg_pos = i;
      
      reorganize[0] = '!';
      reorganize[1] = XBeeArray[(start_msg_pos + 1) % 3];
      reorganize[2] = XBeeArray[(start_msg_pos + 2) % 3];
      
      XBeeArray[0] = reorganize[0];
      XBeeArray[1] = reorganize[1];
      XBeeArray[2] = reorganize[2];
    }
  } 
  
  return check;
}


// Initializes the PID controllers.
// The function calls in here are pretty straightforwared, and have
// been explained already, essentially.
void PID_init()                                          
{
  unsigned long t1, t2;
  double elaps;
  t1 = micros();
  
  Serial.print("Initializing PID controllers...    ");
  aPID.SetMode(AUTOMATIC);
  aPID.SetSampleTime(PID_SampleTime);	                 
  aPID.SetOutputLimits(PID_OutLims[0],PID_OutLims[1]);	
  bPID.SetMode(AUTOMATIC);
  bPID.SetSampleTime(PID_SampleTime);	               
  bPID.SetOutputLimits(PID_OutLims[0],PID_OutLims[1]);
  
  t2 = micros();
  elaps = (t2-t1);
  Serial.print("Done! Elapsed time (us): ");
  Serial.println(elaps,6);
}



