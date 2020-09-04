// LaserControl
// Controls the output of a laser used to activate a neuronal silencer, archaerhodopsin.
// A sine wave has been demonstrated to be more effective for that control, so this firmware generates
// the sinusoidal output and a trigger to control the shutter.
// To control for rebound excitation, the sinusoid is gradually reduced when the animal's velocity remains at 0
// for a specified length of time.
// Written by Anderson Roy Phillips for the Magee Lab

// pins

#define startPin 19
#define trigPin 35
#define veloPin  A21
#define sinePin  A22

// constants: NOTE: modify these values to best fit setup.
#define AMP 0.5f
#define FREQ 40 // hz
#define STOP_VELO 1588 // hardcoded ADC of stopping velocity. Using the teensy encoder it is 1.25V + ~3mV noise = 1.28V/3.3V*4095 = 1588
#define VELO_THRESH 15 // maximum variation from stop: where each integer corresponds to 3.3/4096 mv. 15--> 0.012mV
#define STOP_TIME 100 // millisecond threshold for animal to remain stopped to trigger stop behavior
#define ATTN_TIME 200.0 // millisecond time for sine output to ramp down. must be float for math

#define PI 3.14159265359

// initialize globals
float B = 2*PI*FREQ/1000; // 2pif * 1s/1000ms
float DACadjust = 4095/3.3; // dac output is val*3.3/4095.0
float AMP_DAC = AMP * DACadjust; // converts to 1240, sice dac output is val*3.3/4095.0.

// timing
unsigned long startTime = 0; // time when start signal received
unsigned long currTime = 0; // current time each loop
unsigned long stopTime = 0; // time when velocity is below STOP_VELO threshold for STOP_TIME milliseconds
unsigned long t = 0; // time since start of sine
unsigned long stoppingTime; // time when sine wave begins attenuating

int stoppingTimeDiff = 0;

// read velocity
int lastVelo = 0;
int currVelo = STOP_VELO + 1; // initialize value to be above stopping threshold
bool isStopping = false; // true when animals velocity is below STOP_VELO threshold

// flags to mark begin and end
bool begin_flag = false; // triggers output
bool stop_flag = false; // marks the end of unattenuated sinusoid and the beginning of the damped sine

void setup()
{
    pinMode(startPin, INPUT);
    pinMode(veloPin, INPUT);
    pinMode(sinePin, OUTPUT);
    pinMode(trigPin, OUTPUT);
    analogWriteResolution(12);
    analogReadResolution(12);
    attachInterrupt(digitalPinToInterrupt(startPin), begin_sine, RISING);
    Serial.begin(12000000); //Teensy ignores parameter and runs at 12MB/sec
}



void loop() {

    // check time and velocity
    currTime=millis();
    t = currTime-startTime;
    currVelo = analogRead(veloPin);

    // if interrupt passed, begin output logic
    if(begin_flag){ // Active if the start signal given
        
        digitalWrite(trigPin, HIGH); // trigger output

        // Output sinusoid
        int val;
        val =  AMP_DAC*(sin(B*t)+1);
        analogWrite(sinePin,val);

        //Check if velocity is low
        bool isVeloLow;
        isVeloLow = (abs(currVelo-STOP_VELO) < VELO_THRESH);
        
        //Serial.print("IS VELO LOW? "); Serial.print(isVeloLow); Serial.print(" DIFF IN VELO"); Serial.println(abs(currVelo-STOP_VELO));

        if (isVeloLow){
            if (!isStopping){
                //If velocity is low, begin timing length of stopping
                isStopping = true;
                stoppingTime = t;
                stoppingTimeDiff = 0;
            }
            else{

                // Count time since first began stopping 
                stoppingTimeDiff = t - stoppingTime;
                
                if (stoppingTimeDiff > STOP_TIME){//animal is stopped, set stop_flag and clear other flags
                    stop_flag = true;
                    begin_flag = false;
                    stopTime = t;
                    isStopping = false;
                }
            }
        }
        else{
          // If velocity is not low, the animal is moving
          isStopping = false;
          stop_flag = false;
        }
    }
    else if(stop_flag){
        // animal is stopped, attenuate sine for ATTN_TIME ms
        
        signed long timeSinceStop = t-stopTime; // make sure time is signed for math purposes
        if (timeSinceStop<ATTN_TIME){ // damp the sinusoid output for specified length of time
          
            float sineVal;
            sineVal = (-timeSinceStop/ATTN_TIME+1)*AMP_DAC*(sin(B*timeSinceStop)+1); // multiply sine by a line with negative slope
            analogWrite(sinePin, sineVal); // decrease sine output linearly to 0
        }
        else{ // stopped longer than attenuate time
            stop_flag=false;
            begin_flag=false;
            digitalWrite(trigPin, LOW);
        }
    }
    else{ // if no trigger to start or stop, do nothing
        analogWrite(sinePin, 0);
        digitalWrite(trigPin, LOW);
        stopTime = 0; 
        stoppingTime = 0;
        startTime = 0;
    }
}

// Interrupt on pin 19 to start output
void begin_sine(){
    begin_flag = true;
    startTime = millis();
}
