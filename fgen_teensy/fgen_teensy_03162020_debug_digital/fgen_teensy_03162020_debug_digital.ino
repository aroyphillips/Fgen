#include <SPI.h>


#define AOut A22
#define rstPin 19
#define distPin A21
#define digitalPin 6
 
#define MAXAOUT 3.3


float VAL2DAC = 4095/3.3; // Volt = val*3.3/4095 --> 

// Incoming Bit Stream should look like this: '<sb0,100,500!p0,2.00,0150,010!t0,2.00,150,010,060,030!^1,2.50,100,080,030!g0,2,120,100,10!r0,3,60,10!>'


// registers to store incoming data
const byte numChars = 32;
char digit[9];
char pulse[15];
char train[22];
char gaus[18];
char tri[18];


// booleans to guide reception of serial data
boolean newData = false;

boolean isDig = false;
boolean isPulse = false;
boolean isTrain = false;
boolean isGaus = false;
boolean isTri = false;

// store timing to avoid using delays
unsigned long beforeRxMillis;
unsigned long afterRxMillis;

//Incoming Paramters

//Dependent parameter
boolean isDist = false;
boolean isTime = false;


// Digital Pulse params
boolean isDigActive;
int digDelay;
int digDuration;
boolean digPulseOn; // tells whether to pulse or not
boolean hasDigPulsed; // flag to mark start of pulse
unsigned long digTime; // marks the start time of the pulse

// Pulse params
boolean isPulseActive;
float pulseAmp;
int pulseDuration;
int pulseDelay;
float pulseEnd;
unsigned long pulseStartTime; 

// Pulse Train params

boolean isTrainActive;
float trainAmp;
float trainDuration;
float trainDelay;
float trainWidth;
float trainEnd;
float ptStart; // sliding location of pulse starts
float trainT; // cm period of pulse
float trainTs; // ms seconds period
float trainWidthTime; // ms width of pulse
unsigned long ptStartTime;

boolean hasSpiked;
boolean hasSpikedThisLap = false;
boolean doneSpiking;

// Triangle Pulse params

boolean isTriActive;
float triAmp;
float triDuration;
float triPeak;
float triDelay;
float triEnd;
float triUpSlope;
static float triDownSlope;
boolean wasTriangle;

// Gaussian params
boolean isGausActive;
float gausAmp;
float gausCenter;
float gausLeftW;
float gausRightW;
boolean hasGaussed;
float WIDCONST = -log(2);


// Output Voltage variables
volatile float value;

boolean hasPulsedThisLap = false; //Controls pulse, so only pulses max once per lap
boolean hasPulsed = false; // indicates whether pulse needs to be brought to resting output


// Distance
float dist;
volatile float distCorrection;

byte readyToReceive = 0;

void setup() {
    pinMode(AOut, OUTPUT);
    pinMode(rstPin, INPUT);
    pinMode(distPin, INPUT);
    pinMode(digitalPin, OUTPUT);
    analogWriteResolution(12);
    analogReadResolution(12);
    attachInterrupt(digitalPinToInterrupt(rstPin), reset_distance, FALLING);
    Serial.begin(9600); //Teensy ignores parameter and runs at 12MB/sec
    //Serial.println("<Teensy is ready>");
    analogWrite(AOut, 0);
}

void loop() {
    
    recvWithStartEndMarkers();
    outputVolts();
    if (Serial.availableForWrite()>0){
      Serial.write(readyToReceive);
    }
    
    //Serial.println(readyToReceive);
    //analogWrite(AOut,3*VAL2DAC);
    buildNewData();
    if (digPulseOn){
      digitalWrite(digitalPin, HIGH);
    }
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
          beforeRxMillis = millis();
        }
        
        else if (recvInProgress == true) {
            
            // Read parameters for different output shapes
            if (rc != endMarker) {
                switch(rc) {
                  case 's':
                    // Time parameter
                    clearParamBools();
                    isTime = true;
                    break;

                  case 'd':
                    // Dist parameter
                    clearParamBools();
                    isDist = true;
                    break;

                  case 'b':
                    // Receiving parameters for Digital (b)inary pulse 
                    clearBools();
                    ndx = 0;
                    isDig = true;
                    break;
                    
                  case 'p':
                    // Receiving parameters for Pulse 
                    clearBools();
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
                    // Receiving parameters for triangle pulse
                    clearBools();
                    ndx = 0;
                    isGaus = true;
                    break;  

                  default:
                   // Receiving parameters based on flag
                   if (isDig){
                    if (rc == phraseEnd){
                    }
                    else if (rc!='b'){
                      digit[ndx] =rc;
                      ndx++;
                    }
                   }
                   else if (isPulse){
                    if (rc == phraseEnd){
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
        parseDigitalData(digit);
        parsePulseData(pulse);
        parseTrainData(train);
        parseTriData(tri);
        parseGausData(gaus);
        newData = false;  
      }
       
}


void outputVolts(){
    //float currDist;
    
    float currDist;
    currDist = analogRead(distPin)/VAL2DAC*100;
    dist = currDist-distCorrection;
    //Serial.print("Distance: ");
    //Serial.println(dist);
    unsigned long currTime = millis();
    
    // check each possible shape output and modify output volt accordingly
    // Serial.println(readyToReceive);
    if (dist>170){
      readyToReceive = '1';
    }

    // Digital Pulse flag

    if (isDig && dist> digDelay && !hasDigPulsed){
      digPulseOn = true;
      digTime = millis();
      hasDigPulsed = true;
    }
    else if (isDig && ((currTime - digTime) >= digDuration && hasDigPulsed)){
      digPulseOn = false;
    }
    else if (!isDig || dist < digDelay){
      digPulseOn = false;
    }
    
    // Triangle Pulse (right now needs to be only activated on its own
    if(isTriActive){
      if ((dist>triDelay) && (dist<triPeak)){
        value = triUpSlope*(dist-triDelay);
        //Serial.println("going up:");
        //Serial.println(value);
      }
      else if((dist>triPeak) && (dist<triEnd)){
       
        value = triDownSlope*(dist-triEnd);
        //Serial.print("going Down");
        //Serial.println(triDownSlope);
      }
      else{
        //Serial.println("end");
        value = 0;
      }
    }

    // Gaussian
    else if(isGausActive){
      if (dist<=gausCenter && !hasGaussed){
        
        value = gausAmp * exp(WIDCONST*sq((dist-gausCenter)/gausLeftW));
        //Serial.print("going up:");
        //Serial.println(value);
      }
      else if((dist>gausCenter && !hasGaussed) && dist<170){
        value = gausAmp * exp(WIDCONST*sq((dist-gausCenter)/gausRightW));
        //Serial.print("going Down");
        //Serial.println(value);
        }
      else{
        value = 0;
        hasGaussed = true;
        //Serial.println("drive to 0");
      }
    }

        
    // Pulse
    if(isDist){
      if (isPulseActive && (dist>pulseDelay) && (dist<pulseEnd) && !hasPulsed && !hasPulsedThisLap){
  
        value = value + pulseAmp;
        hasPulsed = true;
        hasPulsedThisLap = true;
        Serial.print("pulsing: ");
        Serial.println(dist);
      }
      else if (isPulseActive && (dist>pulseEnd) && hasPulsed){
        Serial.print("pulse down: ");
        Serial.println(dist);
        value = value - pulseAmp;
        hasPulsed = false;
      }
    }
    else if(isTime){
     if (isPulseActive && (dist>pulseDelay) && ((currTime-pulseStartTime)<pulseDuration) && !hasPulsed && !hasPulsedThisLap){

        pulseStartTime = millis();
        value = value + pulseAmp;
        hasPulsed = true;
        hasPulsedThisLap = true;
        Serial.print("pulsing: ");
        Serial.println(dist);
      }
      else if (isPulseActive && ((currTime-pulseStartTime)>=pulseDuration) && hasPulsed){
        Serial.print("pulse down: ");
        Serial.println(dist);
        value = value - pulseAmp;
        hasPulsed = false;
      }
    }


    // Pulse Train
    if (isDist){
      if (isTrainActive && (dist>trainDelay) && (dist<trainEnd) && !doneSpiking){
        if (!hasSpiked && ((dist-ptStart)<trainWidth) && ((dist-ptStart)>0)){
          value = value + trainAmp;
  
          /*
          Serial.print("Spiking up:");
          Serial.print(dist);
          Serial.print(" start:");
          Serial.print(ptStart);
          Serial.print(" width: ");
          Serial.print(trainWidth);
          Serial.print(" Output: ");
          Serial.println(value);
          */
          hasSpiked = true;
          
        }
        else if(hasSpiked && (dist-ptStart)<trainT && ((dist-ptStart)>trainWidth)){
          value = value - trainAmp;
          hasSpiked = false;
          ptStart = ptStart + trainT;
  
          /*
          Serial.print("Spiking Down:");
          Serial.print(dist);
          Serial.print(" start:");
          Serial.print(ptStart);
          Serial.print(" period: ");
          Serial.print(trainT);
          Serial.print(" Output: ");
          Serial.println(value);
          */
        }
      }
      else if (hasSpiked && (dist>trainEnd)){
        value = value - trainAmp;
        hasSpiked = false;
        doneSpiking = true;
      }
    }

    // Time based train
     else if(isTime){
      if (isTrainActive && (dist>trainDelay) && (dist<trainEnd) && !doneSpiking){
        if (!hasSpikedThisLap){
          hasSpikedThisLap = true;
          ptStartTime = millis();
          currTime = millis();

          /*
          Serial.println(ptStart);
          Serial.println(currTime); 
          Serial.println((currTime-ptStart)<=trainWidthTime);
          Serial.println((currTime-ptStart)>=0);
          Serial.println(!hasSpiked);
          */
        }
        //Serial.println(ptStartTime);
        //Serial.println(currTime);
        if (!hasSpiked && ((currTime-ptStartTime)<=trainWidthTime) && ((currTime-ptStartTime)>=0)){
          value = value + trainAmp;
  
          /*
          Serial.print("Spiking up:");
          Serial.print(currTime);
          Serial.print(" start:");
          Serial.print(ptStartTime);
          Serial.print(" width: ");
          Serial.print(trainWidthTime);
          Serial.print(" train T:");
          Serial.print(trainTs);
          Serial.print(" Output: ");
          Serial.println(value);
          */
          hasSpiked = true;
          
        }
        else if(hasSpiked && (currTime-ptStartTime)<trainTs && ((currTime-ptStartTime)>trainWidthTime)){
          value = value - trainAmp;
          hasSpiked = false;
          ptStartTime = ptStartTime + trainTs;
  
          /*
          Serial.print("Spiking Down:");
          Serial.print(currTime);
          Serial.print(" start:");
          Serial.print(ptStart);
          Serial.print(" period: ");
          Serial.print(trainTs);
          Serial.print(" Output: ");
          Serial.println(value);
          */
        }
      }
      else if (hasSpiked && (dist>trainEnd)){
        value = value - trainAmp;
        hasSpiked = false;
        doneSpiking = true;
      }
    }


    
    analogWrite(AOut, value*VAL2DAC);
    //Serial.println(dist);
    
}  


void clearBools() {
  isPulse = false;
  isTrain = false;
  isGaus = false;
  isTri = false;
}

void clearParamBools(){
  // Clears dependent parameter booleans
  isTime = false;
  isDist = false;
}

void noFlags() {
  //Serial.println("No shape flags read");
}


// Parsing functions to read serial data

void parseDigitalData(char digit_str[]) {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(digit_str, ","); // this continues where the previous call left off
  isDigActive =(*strtokIndx=='1');     // converts char* to a boolean


  strtokIndx = strtok(NULL, ",");
  digDuration = atof(strtokIndx);

  strtokIndx = strtok(NULL, ",");
  digDelay = atof(strtokIndx);     // convert this part to a float

  
  Serial.print("Is Digital Pulse?");
  Serial.println(isDigActive);
  Serial.print("Digital Pulse Duration:");
  Serial.println(digDuration);
  Serial.print("Digital Pulse Delay");
  Serial.println(digDelay);
  
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
  trainT = atof(strtokIndx);     // convert this part to a float

  strtokIndx = strtok(NULL, ",");
  trainWidth = atof(strtokIndx);     // convert this part to a float
  
  trainTs = 1/trainT *1000;
  trainWidthTime = 1/trainWidth *1000;
  trainEnd = trainDelay + trainDuration;
  ptStart = trainDelay;
  hasSpiked = false;

  /*
  Serial.print("Is Train?");
  Serial.println(isTrainActive);
  Serial.print("Train Amp");
  Serial.println(trainAmp);
  Serial.print("Train Duration:");
  Serial.println(trainDuration);
  Serial.print("Train Delay");
  Serial.println(trainDelay);
  Serial.print("Train Ts:");
  Serial.println(trainTs);
  Serial.print("Train Width");
  Serial.println(trainWidth);
  */
  
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

  /*
  Serial.println("_______________________");
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
  */
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
  gausLeftW = atof(strtokIndx);     // convert this part to a float

  strtokIndx = strtok(NULL, ",");
  gausRightW = atof(strtokIndx);     // convert this part to a float


  
  /*
  Serial.print("Is Gaussian?");
  Serial.println(isGausActive);
  Serial.print("Guassian Amp");
  Serial.println(gausAmp);
  Serial.print("Gaussian Center:");
  Serial.println(gausCenter);
  Serial.print("Gaussian LWidth");
  Serial.println(gausLeftW); 
  Serial.print("Gaussian Right Width");
  Serial.println(gausRightW);
  */
}

// INTERUPT SERVICE ROUTINE

void reset_distance(){
  value = 0;
  analogWrite(AOut,0);

  buildNewData();
  analogWrite(AOut,0);
  ptStart = trainDelay;
  hasSpiked = false;
  hasPulsed = false;
  hasPulsedThisLap = false;
  hasSpikedThisLap = false;
  doneSpiking = false;
  hasGaussed = false;
  readyToReceive = 0;
  
  float distRead = analogRead(distPin)/VAL2DAC*100;
  // Adjust for distance offset, ensure distance read is after reset
  if (distRead<20){
    distCorrection = distRead;
  } 
  analogWrite(AOut,0);
}




// to get variable type
void types(String a){Serial.println("it's a String");}
void types(int a)   {Serial.println("it's an int");}
void types(char* a) {Serial.println("it's a char*");}
void types(float a) {Serial.println("it's a float");} 
