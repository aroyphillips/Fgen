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
#define STOP_VELO 5 // maximum threshold for velocity change between time
#define STOP_TIME 100 // millisecond threshold for animal to remain stopped to trigger stop behavior
#define ATTN_TIME 200 // millisecond time for sine output to ramp down

#define PI 3.14159265359

// initialize globals
float AMP_DAC = AMP * 4095/3.3; // convert 1240, sice dac output is val*3.3/4095.0.
float B = 2*PI*FREQ/10e3; // 2pif * 1s/1000ms
float DACadjust = 4095/3.3; // dac output is val*3.3/4095.0

// timing
unsigned long startTime = 0; // time when start signal received
unsigned long currTime = 0; // current time each loop
unsigned long stopTime = 0;
unsigned long t = 0;

// read velocity
int lastVelo = 0;
bool isStopping = False; // True when animals velocity is below STOP_VELO threshold


// flags to mark begin and end
bool begin_flag = False; // triggers output
bool stop_flag = False; // marks the end of unattenuated sinusoid and the beginning of the damped sine

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
    analogWrite(AOut, 0);
}



void loop() {
    currTime=millis();
    int currVelo = analogRead(veloPin);
    unsigned long stoppingTime; // time when sine wave begins attenuating

    t = currTime-sineTime;

    if(begin_flag){
        // Active if the start signal given
        digitalWrite(trigPin, HIGH)
        analogWrite(sinePin, AMP_DAC*(sin(B*t)+1))

        if ((abs(tempVelo-currVelo) < STOP_VELO)){
            if (!isStopping){
                isStopping = True;
                stoppingTime = currTime;
            }
            else{
                if ((currTime - stoppingTime) > STOP_TIME{
                    //animal is stopping, set stop_flag and clear other flags
                    stop_flag = True;
                    begin_flag = False;
                    stopTime = currTime;

                    isStopping = False;
                }
            }
        }
    }
    else if(stop_flag){
        // animal is stopped, attenuate sine for ATTN_TIME ms
        if (currTime-stopTime)<ATTN_TIME{
            analogWrite(sinePin, (-1/ATTN_TIM)*(t+1)*AMP_DAC*(sin(B*t)+1)); // decrease sine output linearly to 0
        }
        else{
            stop_flag=False;
        }
    }
    else{
        analogWrite(sinePin, 0);
        digitalWrite(trigPin, LOW);

    }

    lastVelo = tempVelo;
}

void begin_sine(){
    begin_flag = True;
    startTime = millis();
}