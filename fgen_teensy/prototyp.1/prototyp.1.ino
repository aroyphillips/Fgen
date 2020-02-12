#include <SPI.h>


#define AOut A22
#define MAXAOUT 5


float VAL2DAC = 4096/40;

// Incoming Bit Stream should look like this: '<s1,2,10!p0,2,150,10!t0,2,150,10,20,10!g0,2,120,100,10!r0,3,60,10!^0,3,150,100!>'

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

// DC parameters
boolean isDCactive;
float DCamp;
float DCdelay;


void setup() {
    pinMode(AOut, OUTPUT);
    analogWriteResolution(12);
    Serial.begin(9600);
    Serial.println("<Teensy is ready>");
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
                  case '^':
                    // Receiving parameters for triangle pulse
                    clearBools();
                    ndx = 0;
                    isTri = true;
                    break;
                  default:
                   // Receiving parameters based on flag
                   if (isDC){
                    if (rc == phraseEnd){
                      Serial.println("end step");
                      DC[ndx] = '/0'; // end string
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
                      pulse[ndx] = '/0'; // end string
                      Serial.println(pulse);
                    }
                    else if (rc!='p'){
                    pulse[ndx] =rc;
                    ndx++;
                    }
                   }
                   else if (isTrain){
                    if (rc == phraseEnd){
                      train[ndx] = '/0'; // end string
                    }
                    else if (rc!='t'){
                      train[ndx] = rc;
                      ndx++;
                    }
                   }
                   else if (isGaus){
                    if (rc == phraseEnd){
                      gaus[ndx] = '/0'; // end string
                    }
                    else if (rc!='g'){
                      gaus[ndx] = rc;
                      ndx++;
                    }
                   }
                   else if (isRamp){
                    if (rc == phraseEnd){
                      ramp[ndx] = '/0'; // end string
                    }
                    else if (rc!= 'r'){
                      ramp[ndx] = rc;
                      ndx++;
                    }
                   }
                   else if (isTri){
                    if (rc == phraseEnd){
                      tri[ndx] = '/0'; // end string
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
  isDC = false;
  isPulse = false;
  isTrain = false;
  isGaus = false;
  isRamp = false;
  isTri = false;
}

void noFlags() {
  Serial.print("No shape flags read");
}

void parseDCData(char DCstr[]) {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(DCstr, ","); // this continues where the previous call left off
  isDCactive =(*strtokIndx=='1');     // converts char* to a vboolean
  Serial.println(strtokIndx);
  types(strtokIndx);
  Serial.println(isDCactive);
  strtokIndx = strtok(NULL, ",");
  DCamp = atof(strtokIndx);     // convert this part to a integer
  Serial.println(DCamp);
  strtokIndx = strtok(NULL, ",");
  DCdelay = atof(strtokIndx);     // convert this part to a integer

}

// to get variable type
void types(String a){Serial.println("it's a String");}
void types(int a)   {Serial.println("it's an int");}
void types(char* a) {Serial.println("it's a char*");}
void types(float a) {Serial.println("it's a float");} 
