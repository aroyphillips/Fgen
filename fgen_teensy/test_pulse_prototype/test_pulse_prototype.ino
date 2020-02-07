#include <SPI.h>


#define AOut A22
#define MAXAOUT 5


float VAL2DAC = 4096/40;
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
 
// Pulse Parameters
boolean isPulseActive;
float pulseAmp;
float pulseDuration;
float pulseDelay;


void setup() {
  // put your setup code here, to run once:
    pinMode(AOut, OUTPUT);
    analogWriteResolution(12);
    Serial.begin(9600);
    Serial.println("<Teensy is ready>");  
}

void loop() {
  // put your main code here, to run repeatedly:
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

                  case 'p':
                    // Receiving parameters for Pulse 
                    clearBools();
                    ndx = 0;
                    isPulse = true;

                  default:
                   // Receiving parameters based on flag
                  if (isPulse){
                    if (rc == phraseEnd){
                      pulse[ndx] = '/0'; // end string
                    }
                    else{
                    pulse[ndx] =rc;
                    }
                   }
                   else{
                    noFlags();                                      
                   }
                   ndx++;
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
        newData = false;
    }  
    if (isPulseActive){
      parsePulseData(pulse);
      outputVolts();
    }
}


void outputVolts(){
    delay(pulseDelay);
    analogWrite(AOut, pulseAmp*VAL2DAC);
    delay(pulseDuration);
    analogWrite(AOut, 0); 
}
  


void clearBools() {
  boolean isDC = false;
  boolean isPulse = false;
  boolean isTrain = false;
  boolean isGaus = false;
  boolean isRamp = false;
  boolean isTri = false;
}

void noFlags() {
  Serial.print("No shape flags read");
}

void parsePulseData(char Pulsestr[]) {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index
  
  
  strtokIndx = strtok(Pulsestr, ","); // this continues where the previous call left off
  isPulseActive =(strtokIndx=='1');     // converts first parameter to boolean
  
  strtokIndx = strtok(NULL, ",");
  pulseAmp = atof(strtokIndx);     // convert this part to a float

  strtokIndx = strtok(NULL, ",");
  pulseDuration = atof(strtokIndx);     // convert this part to a float

  strtokIndx = strtok(NULL, ",");
  pulseDelay = atof(strtokIndx);     // convert this part to a float

}
