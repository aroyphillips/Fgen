// Inputs: 2 booleans to determine if ramp is up or down, distance, chirp up and down
// Output: ramp as function of distance, digital output at occurence of chirp
// define BNC pins
// Make sure port & board are correct before uploading

#define MAXSPEED    1000.0f  // maximum speed for dac out (mm/sec)
#define MAXDACVOLTS 2.5f    // DAC ouput voltage at maximum speed
#define MAXDACCNTS  4095.0f // maximum dac value

//Edit Parameters for slope
float upSlope = 4; // 3* voltage/distance (m)
float slopeFactor = 60.0/40.0; // distance up/distance down
float downSlope = upSlope*slopeFactor;  

float maxDACval = MAXDACVOLTS * MAXDACCNTS / 3.3; // limit dac output to max allowed


#define dacPin1 A22
#define startPin A9
#define stopPin A8
#define distPin A2

int soundOutPin = 33;

//void rampUp()

volatile int ramp = 0;
void setup()
{
  pinMode(startPin, INPUT_PULLUP);
  pinMode(stopPin, INPUT_PULLUP);
  pinMode(distPin, INPUT_PULLUP);cd..
  pinMode(chirpDownPin, INPUT);
  pinMode(chirpUpPin, INPUT);
  pinMode(soundOutPin, OUTPUT);
  analogWriteResolution(12);
  
}


void loop()
{

  // Read inputs
  volatile int up = digitalReadFast(startPin); 
  volatile int ramp = 0; 
  volatile int dist = analogRead(distPin);
  int startDist = analogRead(distPin);
  chirpUp = digitalRead(chirpUpPin);
  chirpDown = digitalRead(chirpDownPin);
  if (chirpUp || chirpDown){
    digitalWrite(soundOutPin, HIGH);
  }
  else{
    digitalWrite(soundOutPin, LOW);
  }
    
  
  while(up>0.5){    
    up = digitalReadFast(startPin); 
    dist = analogRead(distPin);
    ramp = upSlope*(dist-startDist);
    analogWrite(A22, ramp);
    delay(5);
    float downVolt = ramp;
  }
  volatile int down = digitalReadFast(stopPin);

  float downVolt = ramp;
  int midDist = analogRead(distPin);
  while (down>0.5)
   {
    down = digitalReadFast(stopPin); 
    dist = analogRead(distPin);
    ramp = downSlope*(midDist-dist)+downVolt;
    if (ramp<0) {
      ramp =0;
    }
    //analogWrite(A22,downDist);
    analogWrite(A22, ramp);
    delay(5);
   }
  ramp = 0;
  analogWrite(A22, ramp);
}
