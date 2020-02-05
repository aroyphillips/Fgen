#include <SPI.h>


#define AOut A22
#define MAXAOUT 5
float VAL2DAC = 4096/40;
char ramp_dlm = '^'; // followed by these numbers: is on, distancestart

// Incoming Bit Stream should look like this: '<s1,2,10p0,2,150,10!t0,2,150,10,20,10!g0,2,120,100,10!r0,3,60,10!^0,3,150,100!>'

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

int DCamp;

void setup() {
    Serial.begin(9600);
    Serial.println("<Teensy is ready>");
}

void loop() {
    recvWithStartEndMarkers();
    runNewData();
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
          
         
            if (rc != endMarker) {
                switch(rc) {
                  case 's':
                    // Receiving parameters for DC step
                    clearBools();
                    ndx = 0;
                    isDC = true;

                  case 'p':
                    // Receiving parameters for Pulse 
                    clearBools();
                    ndx = 0;
                    isPulse = true;

                  case 't':
                    // Receiving parameters for Pulse Train
                    clearBools();
                    ndx = 0;
                    isTrain = true;
        
                  case 'g':
                    // Receiving parameters for Gaussian
                    clearBools();
                    ndx = 0;
                    isGaus = true;

                  case 'r':
                    // Receiving parameters for Ramp
                    clearBools();
                    ndx = 0;
                    isRamp = true;

                  case '^':
                    // Receiving parameters for triangle pulse
                    clearBools();
                    ndx = 0;
                    isTri = true;

                  default:
                   // Receiving parameters based on flag
                   if (isDC){
                    if (rc == phraseEnd){
                      DC[ndx] = '/0'; // end string
                    }
                    else{
                      DC[ndx] = rc;
                    }
                   }
                   else if (isPulse){
                    if (rc == phraseEnd){
                      pulse[ndx] = '/0'; // end string
                    }
                    else{
                    pulse[ndx] =rc;
                    }
                   }
                   else if (isTrain){
                    if (rc == phraseEnd){
                      train[ndx] = '/0'; // end string
                    }
                    else{
                      train[ndx] = rc;
                    }
                   }
                   else if (isGaus){
                    if (rc == phraseEnd){
                      gaus[ndx] = '/0'; // end string
                    }
                    else{
                      gaus[ndx] = rc;
                    }
                   }
                   else if (isRamp){
                    if (rc == phraseEnd){
                      ramp[ndx] = '/0'; // end string
                    }
                    else{
                      ramp[ndx] = rc;
                    }
                   }
                   else if (isTri){
                    if (rc == phraseEnd){
                      tri[ndx] = '/0'; // end string
                    }
                    else{
                      tri[ndx] = rc;
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
    if (DC[0] == '1'){
      outputVolts(DC);
    }
}


void outputVolts(char DC[]){
   
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

void parseDCData() {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index
  
  strtokIndx = strtok(receivedChars,",");      // get the first part - the string
  strcpy(messageFromPC, strtokIndx); // copy it to messageFromPC
  
  strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
  amp = atoi(strtokIndx);     // convert this part to an integer
  
  strtokIndx = strtok(NULL, ",");
  floatFromPC = atof(strtokIndx);     // convert this part to a float

}
