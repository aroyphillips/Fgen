#include <SPI.h>


#define AOut A22
#define MAXAOUT 5
#define distPin A2;


float VAL2DAC = 4095/35; // Volt = val*3.3/4095 --> 

// Incoming Bit Stream should look like this: '<s1,2,10!p0,2,150,10!t0,2,150,10,20,10!^0,3,150,100!g0,2,120,100,10!r0,3,60,10!>'

const byte numChars = 32;
char DC[numChars];
char pulse[numChars];
char train[numChars];
char gaus[numChars];
char ramp[numChars];
char tri[numChars];

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

// Pulse Train params

boolean isTrainActive;
float trainAmp;
float trainDuration;
float trainDelay;
float trainFreq;
float trainWidth;

// Triangle Pulse params

boolean isTriActive;
float triAmp;
float triDuration;
float triPeak;
float triDelay;

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




void setup() {
    pinMode(AOut, OUTPUT);
    analogWriteResolution(12);
    Serial.begin(9600);
    Serial.println("<Teensy is ready>");
    analogWrite(AOut, 0);
}

void loop() {
    recvWithStartEndMarkers();
    buildNewData();
}

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char phraseEnd = '!';
    char rc;
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

       
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

        
        outputVolts(isDCactive, DCamp, DCdelay);
        newData = false;    
      }
       
}



void outputVolts(boolean DCactiv, float ampDC, float delayDC){
    if (DCactiv){
    delay(delayDC);
    analogWrite(AOut, ampDC*VAL2DAC);
    }
    else{
      analogWrite(AOut,0); 
    }
}  


void clearBools() {
  // Resets which shape the Teensy is reading
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
  Serial.print("Ramp slope:");
  Serial.println(rampSlope);
  Serial.println(isRampActive);
  Serial.print("Ramp Amp");
  Serial.println(rampAmp);
  Serial.print("Ramp Duration");
  Serial.println(rampDuration); 
  Serial.print("Ramp Delay");
  Serial.println(rampDelay); 
}


// to get variable type
void types(String a){Serial.println("it's a String");}
void types(int a)   {Serial.println("it's an int");}
void types(char* a) {Serial.println("it's a char*");}
void types(float a) {Serial.println("it's a float");} 
