// EncoderInterfaceT3
//
//  Read the encoder and translate to a distance and send over USB-Serial and DAC
//  Teensy 3.2 Arduino 1.8.3 with Teensy Extensions
//  Modified by RP to reset distance calculation on beam break reset from treadmill
//  Made for Avago HEDM-55xx Rotary Encoder
//
//  This designed to be easy to assemble. The cables are soldered directly to Teensy 3.2.
//  Encoder A - pin 0
//  Encoder B - pin 1
//  Encoder VCC - Vin
//  Encoder ground - GND
//  Lap Reset - pin 120
//  Analog Out - A14/DAC
//  Analog ground - AGND
//
// Steve Sawtelle
// jET
// Janelia
// HHMI 
//
// Modified by Roy Phillips, Magee Lab, HHMI
// Simulate a setup with a start signal to begin activation of a neuronal silencer.
// Outputs Distance and Velocity

#define VERSION "20200818"
// ===== VERSIONS ======

#define MAXSPEED    1000.0f  // maximum speed for dac out (mm/sec)
#define MAXDACVOLTS 2.5f    // DAC ouput voltage at maximum speed
#define MAXDACCNTS  4095.0f // maximum dac value

float maxDACval = MAXDACVOLTS * MAXDACCNTS / 3.3; // limit dac output to max allowed
float DACadjust = MAXDACCNTS/3.3/1000.0; // dac output is val*3.3/4095.0

#define encAPin 0
#define encBPin 1
#define dacPin1  A21
#define dacPin2  A22
#define startLaserPin A9
#define triggerPin A8
//#define idxPin  2  // not used here

// counts per rotation of treadmill encoder wheel
// 512 counts per rev
// 4.00" per rev
// so - 4.00 in. * 25.4 mm/in. * pi / 512 ticks  = 0.6234097922 mm/cnt
// multiply by 10^6 to account for counts per second

#define MM_PER_COUNT 623410  // divides by 1/10^6mm per count since we divide by usecs, using 4.00" per rev
#define DIST_PER_COUNT ((float)MM_PER_COUNT/1000000.0) //in mm


#define SPEED_TIMEOUT 50000  // if we don't move in this many microseconds assume we are stopped

volatile float runSpeed = 0;
static float lastSpeed = 0;
volatile uint32_t lastUsecs;
volatile uint32_t thisUsecs;
volatile uint32_t encoderUsecs;
volatile float distance = 0;
volatile float test = 0;
volatile float trig = 0;

// variables needed for auto reset
unsigned long pulseTimer =0;
boolean resetFlag = false;
#define FW 1
#define BW -1
int dir = FW;

// ------------------------------------------
// interrupt routine for ENCODER_A rising edge
// ---------------------------------------------
void encoderInt()
{   
  int ENCA = digitalReadFast(encAPin);  // always update output 
  int ENCB = digitalReadFast(encBPin); 
  if (ENCA == ENCB )    // figure out the direction  
  {   
 //   Serial.print('B');
    dir = BW;
    thisUsecs = micros();
    encoderUsecs = thisUsecs - lastUsecs;
    lastUsecs = thisUsecs;
    runSpeed = dir*(float)MM_PER_COUNT / encoderUsecs;
    //Serial.print(runSpeed);
    //Serial.print("speed");
    distance -= DIST_PER_COUNT;
  }  
  else
  {
 //   Serial.print('F');
    dir = FW;
    thisUsecs = micros();
    encoderUsecs = thisUsecs - lastUsecs;
    lastUsecs = thisUsecs;
    runSpeed = (float)MM_PER_COUNT / encoderUsecs;
    distance += DIST_PER_COUNT;
  }  
}
void setup()
{
  //Serial.begin(192000);
 // while( !Serial);   // if no serial USB is connected, may need to comment this out
  pinMode(encAPin, INPUT_PULLUP); // sets the digital pin as input
  pinMode(encBPin, INPUT_PULLUP); // sets the digital pin as input
  pinMode(triggerPin, OUTPUT); //sets the digital pin as an output
  pinMode(startLaserPin, OUTPUT);
  analogWriteResolution(12);

 // Serial.print("Treadmill Interface V: ");
 // Serial.println(VERSION);
 // Serial.println("distance,speed");
 // Serial.println(maxDACval);
  digitalWrite(startLaserPin, LOW);
  lastUsecs = micros();
  attachInterrupt(encAPin, encoderInt, RISING); // check encoder every A pin rising edge
}

void loop() 
{ 
  unsigned long currTime;

  currTime = millis();
  noInterrupts();
  uint32_t now = micros();
  uint32_t lastU = lastUsecs;
  if( (now > lastU) && ((now - lastU) > SPEED_TIMEOUT)  )
  {   // now should never be < lastUsecs, but sometiems it is
      // I question if noInterupts works
     runSpeed = 0;
  }        
  interrupts(); 

  float dacval = (runSpeed/MAXSPEED+1) * maxDACval/2; 
  if( dacval > maxDACval) dacval = maxDACval;
  if( dacval < 0) dacval = 0;
  if( runSpeed == 0) digitalWrite(triggerPin, LOW);
  if( runSpeed > 0) digitalWrite(triggerPin, HIGH);
  delay(1);
  digitalWrite(triggerPin, LOW);
//      float dacdist = distance / 18000.0f * 3.3f; // scales the distance down to the output voltage of the DAC, division by 2000 to get portion of track length

  
  Serial.print(distance);
  Serial.print(",");
  Serial.print(runSpeed);   
  Serial.print(":");
  Serial.print(dacval);
  Serial.print(" : ");
  Serial.print(resetFlag);
  Serial.print(" : ");
  Serial.println(currTime-pulseTimer); 
  
  
  if ((currTime-pulseTimer>100)&& resetFlag){
     //Serial.println("RESET DONE");
     resetFlag = false;
     digitalWrite(startLaserPin, LOW);
  }
  
  if (distance > 1800) {
     distance = 0;  // Wrap around
     digitalWrite(startLaserPin, HIGH);
     pulseTimer = millis();
     resetFlag = true;
     //Serial.println("RESET");
  }

  
  analogWrite(A21,(uint16_t) distance * DACadjust);
  analogWrite(A22,(uint16_t) dacval);
}
