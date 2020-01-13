
#include <SPI.h>  //SPI library for Arduino
#include <math.h>
#include <string.h>

#define dacPin1 A22

//Teensy 3.6 Constants
uint8_t             chipSelectPin     =               15;
bool                GO                =            false;

char                startMarker       =              '<';
char                endMarker         =              '>';

const int           ArbMem            =            10e3;
uint16_t            Istim             =               0;

uint16_t LD;
uint32_t  N;
double  ArbHeap[ArbMem] = {};
double freq;
double  amp;

//========================== Setup Function ============================
void setup() {
  SPI.begin();
  delay(10);
  Serial.begin(9600);
  analogWriteResolution(12);
  pinMode (chipSelectPin, OUTPUT);

  SPI.beginTransaction(SPISettings(17e6, MSBFIRST, SPI_MODE0));
  digitalWrite(chipSelectPin, HIGH);
  delay(1000);
}

void loop() {
  if (GO) {
    UpdateStim();
  }   
  if (Serial.available() > 0) {
    UNOSerialEvent();
  }
}

void UNOSerialEvent() {
  int                NB=4;
  char           temp[NB];
  String incoming = "";
 
  char rb = Serial.read();
  if (rb == startMarker) {
    // get frequency value first
    incoming = Serial.readString();
    incoming.toCharArray(temp, sizeof(temp));
    freq = (double)atof(temp);
    byte * b = (byte *) &freq;
    Serial.write(b,8);

    // get amplitude value second
    incoming = Serial.readString();
    incoming.toCharArray(temp, sizeof(temp));
    amp = (double)atof(temp);
    byte * d = (byte *) &amp;
    Serial.write(d,8);
    makeSine(freq, amp);
    GO = true;
  }

  rb = Serial.read();
}

void UpdateStim(void) {

  double Amp = ArbHeap[Istim];
  byte * b = (byte *) &Amp;
  Serial.write(b,8);
  volatile int intAmp = (int)round(Amp);
  intAmp = intAmp/10^308*5;
  //analogWrite(A22, intAmp);
  Istim++;
  if (Istim >= N)  {
    Istim = 0;
  }
}

void makeSine(double Freq, double Amp) {
  double   Stim;
  double   tld        =                        14.25;
  uint32_t FreqUpdate =          floor(1/(tld*1e-6));
  uint16_t FreqB      =                           50;
  N                   = (uint32_t)(FreqUpdate/FreqB);
  LD                  =  floor(tld*((FreqB/Freq)-1));

  for ( int c = 0; c < (int)N; c++) {
    Stim = Amp*sin((2.0*PI*((double)c))/(double)N);
    ArbHeap[c] = Stim;
  }
}
  
