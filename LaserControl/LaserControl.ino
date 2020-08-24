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

// constants
#define AMP 0.5f
#define FREQ 40 // hz
#define SPEED_TIMEOUT 50000  // if we don't move in this many microseconds assume we are stopped
#define STOP_VELO 1588 // hardcoded ADC of stopping velocity. Using the teensy encoder it is 1.25V + ~3mV noise = 1.28V/3.3V*4095 = 1588
#define VELO_THRESH 15 // maximum variation from stop: where each integer corresponds to 3.3/4096 mv
#define STOP_TIME 1000 // millisecond threshold for animal to remain stopped to trigger stop behavior
#define TIME_UNIT_VELO 10 // number of milliseconds between velocity readings to see if velocity is changing across this unit of time
#define ATTN_TIME 200.0 // millisecond time for sine output to ramp down

#define PI 3.14159265359

// initialize globals
float B = 2*PI*FREQ/1000; // 2pif * 1s/1000ms
float DACadjust = 4095/3.3; // dac output is val*3.3/4095.0
float AMP_DAC = AMP * DACadjust; // converts to 1240, sice dac output is val*3.3/4095.0.

// timing
unsigned long startTime = 0; // time when start signal received
unsigned long currTime = 0; // current time each loop
unsigned long stopTime = 0; // time when velocity is below STOP_VELO threshold for STOP_TIME milliseconds
unsigned long lastVeloTime = TIME_UNIT_VELO+1; // time of last check velocity. set above threshold to 
unsigned long t = 0; // time since start of sine
unsigned long stoppingTime; // time when sine wave begins attenuating

int stoppingTimeDiff = 0;

// read velocity
int lastVelo = 0;
int currVelo = STOP_VELO + 1; // initialize value to be above stopping threshold
bool isStopping = false; // true when animals velocity is below STOP_VELO threshold
bool veloChecked = false; //true every TIME_UNIT_VELO milliseconds when previous velocity is stored

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
    currTime=millis();
    t = currTime-startTime;
    // check velocity every 10 milliseconds
    currVelo = analogRead(veloPin);
    
    if(begin_flag){
      
        Serial.println("SHOULD BE SINING");
        // Active if the start signal given
        
        digitalWrite(trigPin, HIGH);
        int val;
        val =  AMP_DAC*(sin(B*t)+1);
        analogWrite(sinePin,val);
        bool isVeloLow;
        isVeloLow = (abs(currVelo-STOP_VELO) < VELO_THRESH);
        
        Serial.print("IS VELO LOW? "); Serial.print(isVeloLow); Serial.print(" DIFF IN VELO"); Serial.println(abs(currVelo-STOP_VELO));
        //Serial.print(" current velo"); Serial.print(currVelo); Serial.println( " ."); 
        
        if (isVeloLow){
            if (!isStopping){
                isStopping = true;
                stoppingTime = t;
                Serial.print("STOPPING TIME"); Serial.println(stoppingTime);
                stoppingTimeDiff = 0;
            }
            else{
                Serial.print("time "); Serial.print(t); Serial.print(" stopping time: "); Serial.println(stoppingTime);
                stoppingTimeDiff = t - stoppingTime;
                Serial.print("How long since stopping?"); Serial.println(stoppingTimeDiff);
                if (stoppingTimeDiff > STOP_TIME){
                    //animal is stopped, set stop_flag and clear other flags
                    stop_flag = true;
                    begin_flag = false;
                    stopTime = t;
                    Serial.print("STOP TIME"); Serial.println(stopTime);
                    isStopping = false;
                }
            }
        }
        else{
          Serial.println("MOVING");
          isStopping = false;
          stop_flag = false;
        }
    }
    else if(stop_flag){
        Serial.println("SHOULD BE STOPPING");
        // animal is stopped, attenuate sine for ATTN_TIME ms
        signed long timeSinceStop = t-stopTime;
        Serial.print("t:"); Serial.print(t); Serial.print("stopTime "); Serial.print(stopTime); Serial.print(" TIME DIFF: "); Serial.println(timeSinceStop);
        if (timeSinceStop<ATTN_TIME){
            float sineVal;
            sineVal = (-timeSinceStop/ATTN_TIME+1)*AMP_DAC*(sin(B*timeSinceStop)+1);
            Serial.print("DAMPED VALUE:"); Serial.println(sineVal);
            analogWrite(sinePin, sineVal); // decrease sine output linearly to 0
        }
        else{
            stop_flag=false;
            begin_flag=false;
            digitalWrite(trigPin, LOW);
        }
    }
    else{
        if (isStopping){
          Serial.println("DO NOTHING");
        }
        analogWrite(sinePin, 0);
        digitalWrite(trigPin, LOW);
        stopTime = 0; 
        stoppingTime = 0;
        startTime = 0;
    }
}

void begin_sine(){
    Serial.println("BEGIN SINE");
    begin_flag = true;
    startTime = millis();
}
