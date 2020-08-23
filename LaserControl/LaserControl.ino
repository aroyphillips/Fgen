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
#define STOP_VELO 15 // maximum threshold for velocity change between time -- velo conversion: (3.3/4096*STOP_VELO mv - 1.25) * 80cm/s --> 0.012 mV change = 1 cm/s
#define STOP_TIME 1000 // millisecond threshold for animal to remain stopped to trigger stop behavior
#define TIME_UNIT_VELO 10 // number of milliseconds between velocity readings to see if velocity is changing across this unit of time
#define ATTN_TIME 200 // millisecond time for sine output to ramp down

#define PI 3.14159265359

// initialize globals
float AMP_DAC = AMP * 4095/3.3; // convert 1240, sice dac output is val*3.3/4095.0.
float B = 2*PI*FREQ/10e3; // 2pif * 1s/1000ms
float DACadjust = 4095/3.3; // dac output is val*3.3/4095.0

// timing
unsigned long startTime = 0; // time when start signal received
unsigned long currTime = 0; // current time each loop
unsigned long stopTime = 0; // time when velocity is below STOP_VELO threshold for STOP_TIME milliseconds
unsigned long lastVeloTime = TIME_UNIT_VELO+1; // time of last check velocity. set above threshold to 
unsigned long t = 0; // time since start of sine

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
    pinMode(trigPin, INPUT);
    analogWriteResolution(12);
    analogReadResolution(12);
    attachInterrupt(digitalPinToInterrupt(startPin), begin_sine, RISING);
    Serial.begin(12000000); //Teensy ignores parameter and runs at 12MB/sec
}



void loop() {
    currTime=millis();
    unsigned long stoppingTime; // time when sine wave begins attenuating
    t = currTime-startTime;
    // check velocity every 10 milliseconds
    if (((t - lastVeloTime) % TIME_UNIT_VELO == 0) && veloChecked){
      currVelo = analogRead(veloPin);
      if (begin_flag){
        Serial.print("Checking Time: "); Serial.println(lastVeloTime);
      }
      veloChecked = false;
    }
    
    if(begin_flag){
        Serial.println("SHOULD BE SINING");
        // Active if the start signal given
        digitalWrite(trigPin, HIGH);
        int val;
        val =  AMP_DAC*(sin(B*t)+1);
        analogWrite(sinePin,val);
        bool isVeloLow;
        isVeloLow = (abs(lastVelo-currVelo) < STOP_VELO);
        
        Serial.print("IS VELO LOW? "); Serial.print(isVeloLow); Serial.print(" DIFF IN VELO"); Serial.println(abs(lastVelo-currVelo));
        Serial.print("last velo: "); Serial.print(lastVelo); Serial.print(" current velo"); Serial.print(currVelo); Serial.println( " difference ??"); 
        if (isVeloLow){
            if (!isStopping){
                isStopping = true;
                stoppingTime = t;
                Serial.print("STOPPING TIME"); Serial.println(stoppingTime);
            }
            else{
                Serial.print("How long stop?"); Serial.println(t-stoppingTime);
                if ((t - stoppingTime) > STOP_TIME){
                    //animal is stopping, set stop_flag and clear other flags
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
        }
    }
    else if(stop_flag){
        Serial.println("SHOULD BE STOPPING");
        // animal is stopped, attenuate sine for ATTN_TIME ms
        Serial.print("TIME DIFF: "); Serial.println(t-stopTime);
        if ((t-stopTime)<ATTN_TIME){
            analogWrite(sinePin, (-1/ATTN_TIME)*(t+1)*AMP_DAC*(sin(B*t)+1)); // decrease sine output linearly to 0
        }
        else{
            stop_flag=false;
            begin_flag=false;
            digitalWrite(trigPin, LOW);
        }
    }
    else{
        //Serial.println("NOTHING");
        analogWrite(sinePin, 0);
        digitalWrite(trigPin, LOW);
        stopTime = 0; 
        stoppingTime = 0;
        startTime = 0;
    }
    if (t % TIME_UNIT_VELO == 0){
      lastVelo = currVelo; // store velocity for previous 10 seconds
      lastVeloTime = t;
      veloChecked = true;
    }
    delay(5);
}

void begin_sine(){
    Serial.println("BEGIN SINE");
    begin_flag = true;
    startTime = millis();
}
