#include <SPI.h>


#define AOut A22
#define rstPin 19
#define distPin A21
#define digitalPin 35
 
#define MAXAOUT 3.3


float VAL2DAC = 4095/3.3; // Volt = val*3.3/4095 --> 

// Incoming Bit Stream should look like this: '<b0,100,8,0500!p0,2.00,010,7,0150!t0,2.00,050,7,100,8,020,040!^1,2.50,100,080,030!g0,2,120,100,10!>'


// registers to store incoming data
const byte numChars = 32;
char digit[12];
char pulse[17];
char train[27];
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

//Incoming Paramters

//Dependent parameter
char distSym = 'd';
char timeSym = 's';
const char* distPtr = &distSym;
const char* timePtr = &timeSym;

// Digital Pulse params
boolean isDigActive;
float digDelay;
char* digParam;
float digDuration;
boolean isDigTime;
boolean isDigDist;
boolean digPulseOn = false; // tells whether to pulse or not
boolean hasDigPulsed; // flag to mark start of pulse
unsigned long digTime; // marks the start time of the pulse

// Pulse params
boolean isPulseActive;
float pulseAmp;
char* pulseParam;
int pulseDuration;
int pulseDelay;
float pulseEnd;
boolean isPulseTime;
boolean isPulseDist;
unsigned long pulseStartTime; 

// Pulse Train params

boolean isTrainActive;
float trainAmp;
char* trainParam;
float trainDuration;
char* spikeParam;
float trainDelay;
float trainWidth;
float trainEnd;
unsigned long trainStartTime;
float ptStart; // sliding location of pulse starts
float trainT; // cm period of pulse
float trainTs; // ms seconds period
float trainWidthTime; // ms width of pulse
unsigned long ptStartTime;

boolean hasStartedTrain =false;
boolean isTrainTime;
boolean isTrainDist;
boolean isSpikeTime;
boolean isSpikeDist;
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

byte readyToReceive = 0x00;

void setup() {
    pinMode(AOut, OUTPUT);
    pinMode(rstPin, INPUT);
    pinMode(distPin, INPUT);
    pinMode(digitalPin, OUTPUT);
    analogWriteResolution(12);
    analogReadResolution(12);
    attachInterrupt(digitalPinToInterrupt(rstPin), reset_distance, RISING);
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
    if (digPulseOn){
      //Serial.println("Pulsing Digital");
      digitalWrite(digitalPin, HIGH);
    }
    else{
      digitalWrite(digitalPin, LOW);
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
        //delay(100);
        //Serial.print(rc);
        if (rc == startMarker) {
          recvInProgress = true;
        }
        
        else if (recvInProgress == true) {
            
            // Read parameters for different output shapes
            if (rc != endMarker) {
                switch(rc) {

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
    dist = currDist; //-distCorrection;
    //Serial.print("Distance: ");
    //Serial.println(dist);
    //Serial.println(dist> digDelay);
    unsigned long currTime = micros();
    
    // check each possible shape output and modify output volt accordingly
    // Serial.println(readyToReceive);
    if (dist>170){
      readyToReceive = 0xFF; // hex code for 0b 1111 1111
    }
    else{
      readyToReceive = 0x00;
    }

    // DIGITAL PULSE FLAG

    if(isDigTime){
      if (isDigActive && dist> digDelay && !hasDigPulsed){
       //Serial.print("Turning dig pulse on, duration ");
       //Serial.println(digDuration);
        digPulseOn = true;
        digTime = micros();
        hasDigPulsed = true;
      }
      else if (isDigActive && ((currTime - digTime) >= (unsigned long)digDuration && hasDigPulsed)){
        // Serial.println("dig pulse off");
        digPulseOn = false;
      }
      else if ((!isDigActive || (dist < digDelay)) && !hasDigPulsed){
        digPulseOn = false;
      }
    }
    else if (isDigDist){
      if (isDigActive && dist> digDelay && !hasDigPulsed){
       //Serial.print("Turning dig pulse on, duration ");
       //Serial.println(digDuration);
        digPulseOn = true;
        hasDigPulsed = true;
      }
      else if (isDigActive && (dist >= (digDuration+digDelay)) && hasDigPulsed){
        //Serial.println("dig pulse off");
        digPulseOn = false;
      }
      else if ((!isDigActive || (dist < digDelay) || (dist >=(unsigned long)(digDuration+digDelay))) && !hasDigPulsed){
        //Serial.print("dist < digDelay : ");
        //Serial.print(dist < digDelay);
        //Serial.print(" past dur: ");
        //Serial.println(dist >=(digDuration +digDelay));
        digPulseOn = false;
      }
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
    if(isPulseDist){
      if (isPulseActive && (dist>pulseDelay) && (dist<pulseEnd) && !hasPulsed && !hasPulsedThisLap){
  
        value = value + pulseAmp;
        hasPulsed = true;
        hasPulsedThisLap = true;
        //Serial.print("pulsing: ");
        //Serial.println(dist);
      }
      else if (isPulseActive && (dist>pulseEnd) && hasPulsed){
        //Serial.print("pulse down: ");
        //Serial.println(dist);
        value = value - pulseAmp;
        hasPulsed = false;
      }
    }
    else if(isPulseTime){
     if (isPulseActive && (dist>pulseDelay) && !hasPulsed && !hasPulsedThisLap){

        pulseStartTime = micros();
        value = value + pulseAmp;
        hasPulsed = true;
        hasPulsedThisLap = true;
       //Serial.print("pulsing: ");
       //Serial.println(dist);
      }
      else if (isPulseActive && ((currTime-pulseStartTime)>=(unsigned long)pulseDuration) && hasPulsed){
        //Serial.print("pulse down: ");
        //Serial.println(dist);
        value = value - pulseAmp;
        hasPulsed = false;
      }
    }


    // Pulse Train
    if(isTrainActive){
      
      // DISTANCE DURATION
      if (isTrainDist){
        if(((dist>trainDelay) || hasStartedTrain)&& !doneSpiking){
          // Condition makes stimulus continue after animal reaches start point, even if distance fluctuates below
          hasStartedTrain = true;
          
          if ((dist<trainEnd)){
            if(isSpikeDist){
              //Distance-based Spikes
              
              if (!hasSpiked && ((dist-ptStart)<trainWidth) && ((dist-ptStart)>0)){
                value = value + trainAmp;
                hasSpiked = true;
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
            
            else if (isSpikeTime){
    
              //Time-Based Spikes
              if (!hasSpikedThisLap){
                hasSpikedThisLap = true;
                ptStartTime = micros();
                currTime = micros();
      
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
                hasSpiked = true;
  
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
                
                
              }
              else if(hasSpiked && (currTime-ptStartTime)<=trainTs && ((currTime-ptStartTime)>trainWidthTime)){
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
          }
          else if (hasSpiked && (dist>trainEnd)){
            value = value - trainAmp;
            hasSpiked = false;
            doneSpiking = true;
          }
        }
      }
  
      // TIME DURATION
         else if(isTrainTime){
    
          if ((dist>trainDelay) && !doneSpiking){
            // start train
            if(!hasStartedTrain){
              //Serial.println("Starting train");
              trainStartTime = micros();
              hasStartedTrain = true;
            }
            // HZ SPIKES
            if (isSpikeTime){
                if (!hasSpikedThisLap){
                  hasSpikedThisLap = true;
                  ptStartTime = micros();
                  currTime = micros();
                 
                }

                if (currTime < (trainStartTime +trainDuration)){
                  //Serial.print("Condition:");
                  //Serial.println(currTime-ptStartTime);
                  if (!hasSpiked && ((currTime-ptStartTime)<=trainWidthTime) && ((currTime-ptStartTime)>=0)){
    
                    //Serial.print("spiking up:");
                    //Serial.println(currTime);
                    value = value + trainAmp; 
                    hasSpiked = true;
                    
                  }
                  else if(hasSpiked && (currTime-ptStartTime)<=trainTs && ((currTime-ptStartTime)>trainWidthTime)){
                    value = value - trainAmp;
                    hasSpiked = false;
                    ptStartTime = ptStartTime + trainTs;
                    //Serial.print("spiking down:");
                    //Serial.println(currTime);
                    //Serial.println(ptStartTime);
                  }
                  
                }
                else if (currTime >= (trainStartTime + trainDuration)){
                  if(hasSpiked) {
                    value = value - trainAmp;
                    hasSpiked = false;
                  }
                  doneSpiking =true;
                  //Serial.println("Done spiking");
                }
            }
  
              //DIST SPIKES
            else if(isSpikeDist){
  
              if (currTime < (trainStartTime +trainDuration)){
                if (!hasSpiked && ((dist-ptStart)<trainWidth) && ((dist-ptStart)>0)){
                //Serial.println("Spiking Up");
                value = value + trainAmp;
                hasSpiked = true;
                }
                else if(hasSpiked && (dist-ptStart)<trainT && ((dist-ptStart)>trainWidth)){
                  //Serial.println("Spiking Down");
                  value = value - trainAmp;
                  hasSpiked = false;
                  ptStart = ptStart + trainT;
                } 
              }                                
              else if (currTime >= (trainStartTime + trainDuration)){
                if(hasSpiked) {
                  value = value - trainAmp;
                  hasSpiked = false;
                }
                doneSpiking =true;
                //Serial.println("Done spiking");
              }         
            }
          }
       }
    }
    analogWrite(AOut, value*VAL2DAC);
    //Serial.println(dist);   
}  


void clearBools() {
  // resets all reception flags so only receiving one at a time
  isPulse = false;
  isTrain = false;
  isGaus = false;
  isTri = false;
  isDig = false;
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
  digDelay = atof(strtokIndx);     // convert this part to a float

  strtokIndx = strtok(NULL, ",");
  digParam = strtokIndx; // char* assigned to parameter

  strtokIndx = strtok(NULL, ",");
  digDuration = atof(strtokIndx);

  //strcmp returns -15 if false, -12 if true
  if ((strcmp(digParam, timePtr))>-15){
     //Serial.println(" time");  
     isDigTime = true;
     isDigDist = false;
     digDuration = digDuration *1000;
  }
  else{
     //Serial.println(" dist");
     isDigTime = false;
     isDigDist = true;
  }

  /*
  Serial.println("DIGITAL PULSE PARAMETERS");
  Serial.print("Is Digital Pulse? ");
  Serial.print(isDigActive);
  Serial.print(" Digital Pulse Duration:");
  Serial.print(digDuration);
  Serial.print(" Digital Pulse Delay:");
  Serial.print(digDelay);
  Serial.print(" parameter:");
  Serial.print(digParam);
  Serial.print(" Time-based?");
  Serial.print(isDigTime);
  Serial.print( " Distance-based?");
  Serial.println(isDigDist);
  */
}


void parsePulseData(char pulse_str[]) {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(pulse_str, ","); // this continues where the previous call left off
  isPulseActive =(*strtokIndx=='1');     // converts char* to a boolean


  strtokIndx = strtok(NULL, ",");
  pulseAmp = atof(strtokIndx);     // convert char* to a float

  strtokIndx = strtok(NULL, ",");
  pulseDelay = atof(strtokIndx);     // convert this part to a float

  strtokIndx = strtok(NULL, ",");
  pulseParam = strtokIndx;
  
  strtokIndx = strtok(NULL, ",");
  pulseDuration = atof(strtokIndx);

  pulseEnd = pulseDelay + pulseDuration;

  
  if ((strcmp(pulseParam, timePtr))>-15){
       isPulseTime = true;
       isPulseDist = false;
       pulseDuration = pulseDuration *1000;
  }
  else{
       isPulseTime = false;
       isPulseDist = true;
  }
  
  /*
  Serial.print("Is Pulse?");
  Serial.print(isPulseActive);
  Serial.print(" Pulse Amp:");
  Serial.print(pulseAmp);
  Serial.print(" Pulse Duration:");
  Serial.println(pulseDuration);
  Serial.print(" Pulse Delay:");
  Serial.print(pulseDelay);
  Serial.print(" Time-based?");
  Serial.print(isPulseTime);
  Serial.print( " Distance-based?");
  Serial.println(isPulseDist);
  */
}


void parseTrainData(char train_str[]) {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(train_str, ","); // this continues where the previous call left off
  isTrainActive =(*strtokIndx=='1');     // converts char* to a boolean


  strtokIndx = strtok(NULL, ",");
  trainAmp = atof(strtokIndx);     // convert char* to a float


  strtokIndx = strtok(NULL, ",");
  trainDelay = atof(strtokIndx);     // convert this part to a float

  strtokIndx = strtok(NULL, ",");
  trainParam = strtokIndx;
  
  strtokIndx = strtok(NULL, ",");
  trainDuration = atof(strtokIndx);

  strtokIndx = strtok(NULL, ",");
  spikeParam = strtokIndx;
  
  strtokIndx = strtok(NULL, ",");
  trainT = atof(strtokIndx);     // convert this part to a float

  strtokIndx = strtok(NULL, ",");
  trainWidth = atof(strtokIndx);     // convert this part to a float

  if ((strcmp(trainParam, timePtr))>-15){
       isTrainTime = true;
       isTrainDist = false;
       trainDuration = trainDuration*1000;
  }
  else{
       isTrainTime = false;
       isTrainDist = true;
  }

  if ((strcmp(spikeParam, timePtr))>-15){
       isSpikeTime = true;
       isSpikeDist = false;
  }
  else{
       isSpikeTime = false;
       isSpikeDist = true;
  }

  
  trainTs = 1/trainT *1000000;
  trainWidthTime = 1/trainWidth *1000000;
  trainEnd = trainDelay + trainDuration;
  ptStart = trainDelay;
  hasSpiked = false;

  /*
  Serial.println("TRAIN");
  Serial.print("Is Train?");
  Serial.print(isTrainActive);
  Serial.print(" Train Amp:");
  Serial.print(trainAmp);
  Serial.print(" Train Duration:");
  Serial.print(trainDuration);
  Serial.print(" Train Delay:");
  Serial.print(trainDelay);
  Serial.print(" Train Ts:");
  Serial.print(trainTs);
  Serial.print(" Train Prd:");
  Serial.print(trainT);
  Serial.print(" Width time:");
  Serial.print(trainWidthTime);
  Serial.print(" Train Width:");
  Serial.print(trainWidth);
  Serial.print(" Time-based Duration?");
  Serial.print(isTrainTime);
  Serial.print( " Distance-based?");
  Serial.print(isTrainDist);
  Serial.print(" Time-based Spikes?");
  Serial.print(isSpikeTime);
  Serial.print( " Distance-based?");
  Serial.println(isSpikeDist);
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
  hasStartedTrain = false;
  doneSpiking = false;
  hasGaussed = false;
  readyToReceive = 0x00;
  digPulseOn = false;
  hasDigPulsed = false;
  
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
void types(boolean a) {Serial.println("it's a boolean");} 
