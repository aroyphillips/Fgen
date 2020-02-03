#include <SPI.h>  //SPI library for Arduino
#define AOut A22
#define MAXAOUT 5
float VAL2DAC = 4096/40;
char ramp_dlm = '^'; // followed by these numbers: is on, distancestart

// Incoming Bit Stream should look like this: '<s1,2,10p0,2,150,10t0,2,150,10,20,10g0,2,120,100,10r0,3,60,10^0,3,150,100>'

const byte numChars = 32;
char DC[numChars];
char pulse[numChars];
char train[numChars];
char gaus[numChars];
char ramp[numChars];
char tri[numChars];

boolean newData = false;

void setup() {
    Serial.begin(9600);
    Serial.println("<Arduino is ready>");
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
    char rc;
 
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

       
        if (rc == startMarker) {
          recvInProgress = true;
        }
        
        else if (recvInProgress == true) {
            if (rc != endMarker) {
                switch(n) {
                  case 's':
                    
        
                }
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }
    }
}

void runNewData() {
    if (newData == true) {
        newData = false;
    }
    
}
