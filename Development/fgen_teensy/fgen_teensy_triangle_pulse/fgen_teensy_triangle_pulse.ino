#include <SPI.h>


#define AOut A22
#define rstPin 25
#define distPin A21
 
#define MAXAOUT 5


float VAL2DAC = 4095/3.3; // Volt = val*3.3/4095 --> 

// Incoming Bit Stream should look like this: '<s0,2,10!p0,2,150,10!t0,2,150,10,20,10!^1,3,100,80,30!g0,2,120,100,10!r0,3,60,10!>'


// registers to store incoming data
const byte numChars = 32;
char DC[numChars];
char pulse[numChars];
char train[numChars];
char gaus[numChars];
char ramp[numChars];
char tri[numChars];


// booleans to guide reception of serial data
boolean newData = false;

boolean isDC = false;
boolean isPulse = false;
boolean isTrain = false;
boolean isGaus = false;
boolean isRamp = false;
boolean isTri = false;

//Incoming Paramters

// DC parameters
boolean isDCactive;
float DCamp;
float DCdelay;

// Pulse params
boolean isPulseActive;
float pulseAmp;
float pulseDuration;
float pulseDelay;
float pulseEnd;

// Pulse Train params

boolean isTrainActive;
float trainAmp;
float trainDuration;
float trainDelay;
float trainFreq;
float trainWidth;
float trainEnd;

// Triangle Pulse params

boolean isTriActive;
float triAmp;
float triDuration;
float triPeak;
float triDelay;
float triEnd;
float triUpSlope;
float triDownSlope;

// Gaussian params
boolean isGausActive;
float gausAmp;
float gausCenter;
float gausWidth;
float gausDelay;

// Ramp params
boolean isRampActive;
float rampSlope;
float rampAmp;
float rampDuration;
float rampDelay;



// Output Voltage variables
volatile float value;

boolean hasStepped;
boolean hasPulsed;

// Distance
float dist;
volatile float distCorrection;


void setup() {
    pinMode(AOut, OUTPUT);
    pinMode(rstPin, INPUT);
    pinMode(distPin, INPUT);
    analogWriteResolution(12);
    analogReadResolution(12);
    attachInterrupt(digitalPinToInterrupt(rstPin), reset_distance, RISING);
    Serial.begin(9600);
    Serial.println("<Teensy is ready>");
    analogWrite(AOut, 0);
}

void loop() {
    recvWithStartEndMarkers();
    outputVolts();
}



// Receives string from serial and stores values

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char phraseEnd = '!';
    char rc;
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();
        delay(100);
       
        if (rc == startMarker) {
          recvInProgress = true;
        }
        
        else if (recvInProgress == true) {
            
            // Read parameters for different output shapes
            if (rc != endMarker) {
                switch(rc) {
                  case 's':
                    // Receiving parameters for DC step
                    Serial.println("reading step");
                    clearBools();
                    ndx = 0;
                    isDC = true;
                    break;
                  case 'p':
                    // Receiving parameters for Pulse 
                    clearBools();
                    Serial.println("reading pulse");
                    ndx = 0;
                    isPulse = true;
                    break;
                  case 't':
                    // Receiving parameters for Pulse Train
                    clearBools();
                    ndx = 0;
                    isTrain = true;
                    break;    
                  case '^':
                    // Receiving parameters for triangle pulse
                    clearBools();
                    ndx = 0;
                    isTri = true;
                    break;  
                  case 'g':
                    // Receiving parameters for Gaussian
                    clearBools();
                    ndx = 0;
                    isGaus = true;
                    break;
                  case 'r':
                    // Receiving parameters for Ramp
                    clearBools();
                    ndx = 0;
                    isRamp = true;
                    break;

                  default:
                   // Receiving parameters based on flag
                   if (isDC){
                    if (rc == phraseEnd){
                      Serial.println("end step");
                      Serial.println(DC);
                    }
                    else if (rc!='s'){
                      DC[ndx] = rc;
                      ndx++;
                    }
                   }
                   else if (isPulse){
                    if (rc == phraseEnd){
                      Serial.println("end pulse");
                      Serial.println(pulse);
                    }
                    else if (rc!='p'){
                    pulse[ndx] =rc;
                    ndx++;
                    }
                   }
                   else if (isTrain){
                    if (rc == phraseEnd){
                    }
                    else if (rc!='t'){
                      train[ndx] = rc;
                      ndx++;
                    }
                   }
                   else if (isGaus){
                    if (rc == phraseEnd){
                    }
                    else if (rc!='g'){
                      gaus[ndx] = rc;
                      ndx++;
                    }
                   }
                   else if (isRamp){
                    if (rc == phraseEnd){
                    }
                    else if (rc!= 'r'){
                      ramp[ndx] = rc;
                      ndx++;
                    }
                   }
                   else if (isTri){
                    if (rc == phraseEnd){
                    }
                    else if (rc!='^'){
                      tri[ndx] = rc;
                      ndx++;
                    }
                   }
                   else{
                    noFlags();                                      
                   }
                   break;
                }
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }
    }
}

void buildNewData() {
    if (newData == true) {
        Serial.println("Output");
        parseDCData(DC);
        parsePulseData(pulse);
        parseTrainData(train);
        parseTriData(tri);
        parseGausData(gaus);
        parseRampData(ramp);
        newData = false;  
      }
       
}


void outputVolts(){
    //float currDist;
    
    float currDist;
    currDist = analogRead(distPin)/VAL2DAC*100;
    dist = currDist-distCorrection;
    
    //dist = analogRead(distPin)/VAL2DAC*100;

    // check each possible shape output and modify output volt accordingly
    /*
    // DC Step
    if (isDCactive && (dist > DCdelay) && !hasStepped){
      value = value + DCamp;
      hasStepped = true;
    }

    // Pulse
    if (isPulseActive && (dist>pulseDelay) && (dist<pulseEnd) && !hasPulsed){
      value = value + pulseAmp;
      hasPulsed = true;
    }
    else if (isPulseActive && (dist>pulseEnd) && hasPulsed){
      value = value - pulseAmp;
      hasPulsed = false;
    }
    */
    // Triangle Pulse

    if (isTriActive && (dist>triDelay) && (dist<triPeak)){
      //Serial.print("going up:");
      //Serial.println(value);
        value = triUpSlope*(dist-triDelay);
    }
    else if(isTriActive && (dist>triPeak) && (dist<triEnd)){
      //Serial.println("going Down");
        value = triDownSlope*(dist-triEnd);
    }
    else if (dist<triDelay || dist>triEnd){
      value = 0;
    }

    analogWrite(AOut, value*VAL2DAC);
    //Serial.println(dist);
}  


void clearBools() {
  isDC = false;
  isPulse = false;
  isTrain = false;
  isGaus = false;
  isRamp = false;
  isTri = false;
}

void noFlags() {
  Serial.println("No shape flags read");
}


// Parsing functions to read serial data
void parseDCData(char DCstr[]) {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(DCstr, ","); // this continues where the previous call left off
  isDCactive =(*strtokIndx=='1');     // converts char* to a boolean


  strtokIndx = strtok(NULL, ",");
  DCamp = atof(strtokIndx);     // convert char* to a float

  strtokIndx = strtok(NULL, ",");
  DCdelay = atof(strtokIndx);     // convert this part to a float
  Serial.print("Is DC Step?");
  Serial.println(isDCactive);
  Serial.print("DC Amp");
  Serial.println(DCamp);
}


void parsePulseData(char pulse_str[]) {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(pulse_str, ","); // this continues where the previous call left off
  isPulseActive =(*strtokIndx=='1');     // converts char* to a boolean


  strtokIndx = strtok(NULL, ",");
  pulseAmp = atof(strtokIndx);     // convert char* to a float

  strtokIndx = strtok(NULL, ",");
  pulseDuration = atof(strtokIndx);

  strtokIndx = strtok(NULL, ",");
  pulseDelay = atof(strtokIndx);     // convert this part to a float

  pulseEnd = pulseDelay + pulseDuration;
  
  Serial.print("Is Pulse?");
  Serial.println(isPulseActive);
  Serial.print("Pulse Amp");
  Serial.println(pulseAmp);
  Serial.print("Pulse Duration:");
  Serial.println(pulseDuration);
  Serial.print("Pulse Delay");
  Serial.println(pulseDelay); 
}


void parseTrainData(char train_str[]) {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(train_str, ","); // this continues where the previous call left off
  isTrainActive =(*strtokIndx=='1');     // converts char* to a boolean


  strtokIndx = strtok(NULL, ",");
  trainAmp = atof(strtokIndx);     // convert char* to a float

  strtokIndx = strtok(NULL, ",");
  trainDuration = atof(strtokIndx);

  strtokIndx = strtok(NULL, ",");
  trainDelay = atof(strtokIndx);     // convert this part to a float

  strtokIndx = strtok(NULL, ",");
  trainFreq = atof(strtokIndx);     // convert this part to a float

  strtokIndx = strtok(NULL, ",");
  trainWidth = atof(strtokIndx);     // convert this part to a float
  
  trainEnd = trainDelay + trainDuration;
  Serial.print("Is Train?");
  Serial.println(isTrainActive);
  Serial.print("Train Amp");
  Serial.println(trainAmp);
  Serial.print("Train Duration:");
  Serial.println(trainDuration);
  Serial.print("Train Delay");
  Serial.println(trainDelay);
  Serial.print("Train Frequency:");
  Serial.println(trainFreq);
  Serial.print("Train Width");
  Serial.println(trainWidth);
   
}



void parseTriData(char tri_str[]) {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(tri_str, ","); // this continues where the previous call left off
  isTriActive =(*strtokIndx=='1');     // converts char* to a boolean


  strtokIndx = strtok(NULL, ",");
  triAmp = atof(strtokIndx);     // convert char* to a float

  strtokIndx = strtok(NULL, ",");
  triDuration = atof(strtokIndx);

  strtokIndx = strtok(NULL, ",");
  triPeak = atof(strtokIndx);     // convert this part to a float

  strtokIndx = strtok(NULL, ",");
  triDelay = atof(strtokIndx);     // convert this part to a float



  triEnd = triDuration + triDelay;
  triUpSlope = triAmp / (triPeak-triDelay);
  triDownSlope = triAmp /(triPeak-triEnd);
  
  Serial.print("Is Triangle?");
  Serial.println(isTriActive);
  Serial.print("Triangle Amp");
  Serial.println(triAmp);
  Serial.print("Triange Duration:");
  Serial.println(triDuration);
  Serial.print("Triangle Peak");
  Serial.println(triPeak); 
  Serial.print("Triangle Delay");
  Serial.println(triDelay); 
  Serial.print("Triangle UpSlope");
  Serial.println(triUpSlope); 
  Serial.print("Triangle DownSlope");
  Serial.println(triDownSlope);
  Serial.print("Triangle End");
  Serial.println(triEnd); 
  
}

void parseGausData(char g_str[]) {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(g_str, ","); // this continues where the previous call left off
  isGausActive =(*strtokIndx=='1');     // converts char* to a boolean


  strtokIndx = strtok(NULL, ",");
  gausAmp = atof(strtokIndx);     // convert char* to a float

  strtokIndx = strtok(NULL, ",");
  gausCenter = atof(strtokIndx);

  strtokIndx = strtok(NULL, ",");
  gausWidth = atof(strtokIndx);     // convert this part to a float

  strtokIndx = strtok(NULL, ",");
  gausDelay = atof(strtokIndx);     // convert this part to a float
  
  
  Serial.print("Is Gaussian?");
  Serial.println(isGausActive);
  Serial.print("Guassian Amp");
  Serial.println(gausAmp);
  Serial.print("Gaussian Center:");
  Serial.println(gausCenter);
  Serial.print("Gaussian Width");
  Serial.println(gausWidth); 
  Serial.print("Gaussian Delay");
  Serial.println(gausDelay); 
}

void parseRampData(char r_str[]) {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(r_str, ","); // this continues where the previous call left off
  isRampActive =(*strtokIndx=='1');     // converts char* to a boolean


  strtokIndx = strtok(NULL, ",");
  rampSlope = atof(strtokIndx);     // convert char* to a float

  strtokIndx = strtok(NULL, ",");
  rampAmp = atof(strtokIndx);

  strtokIndx = strtok(NULL, ",");
  rampDuration = atof(strtokIndx);     // convert this part to a float

  strtokIndx = strtok(NULL, ",");
  rampDelay = atof(strtokIndx);     // convert this part to a float
  
  
  Serial.print("Is Ramp?");
  Serial.println(isRampActive);
  Serial.print("Ramp slope:");
  Serial.println(rampSlope);
  Serial.print("Ramp Amp");
  Serial.println(rampAmp);
  Serial.print("Ramp Duration");
  Serial.println(rampDuration); 
  Serial.print("Ramp Delay");
  Serial.println(rampDelay); 
}

// INTERUPT SERVICE ROUTINE

void reset_distance(){
  value = 0;
  float distRead = analogRead(distPin)/VAL2DAC*100;
    if (distRead<20){
      distCorrection = distRead;
    } 
  analogWrite(AOut,0);
  buildNewData();
}




// to get variable type
void types(String a){Serial.println("it's a String");}
void types(int a)   {Serial.println("it's an int");}
void types(char* a) {Serial.println("it's a char*");}
void types(float a) {Serial.println("it's a float");} 
