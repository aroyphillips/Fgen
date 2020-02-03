#include <ArduinoJson.h>

#include <SPI.h>  //SPI library for Arduino
#include <math.h>
#include <string.h>



#define AOut A22
#define MAXAOUT 5
float VAL2DAC = 4096/40;
String ramp_dlm = "^"; // followed by these numbers: is on, distancestart

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  SPI.begin();
  pinMode(AOut, OUTPUT);
  analogWriteResolution(12);
  SPI.beginTransaction(SPISettings(100e6, MSBFIRST, SPI_MODE0)); // begins using the SPI bus at specified clock spped, data order and data mode

}

void loop() {
  // put your main code here, to run repeatedly:
  String incoming = "";
  if (Serial.available() >0) {
    incoming = Serial.readString();

    // Find Ramp Parameters
    size_t ramp_dlm_pos = incoming.find(ramp_dlm);
    String ramp_params = incoming.substring(ramp_dlm_pos, 4);
    if (ramp_params.at(0) == '1') {
      stepUp(ramp_params.substring(1,3));
    }
  }

}



void stepUp(String in) {
  
   int amp = in.at(0) -'0';
   int amp2 = in.at(1) - '0';
   int amp3 = in.at(2) - '0'; 
  analogWrite(AOut, amp*VAL2DAC);
  delay(1000)
  analogWrite(AOut, amp2*VAL2DAC);
  analogWrite(AOut, amp3*VAL2DAC);
}
