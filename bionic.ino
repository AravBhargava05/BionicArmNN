#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>


// Only one EMG channel now: A0
#define NUM_CHANNELS     1  
// Only two gestures: Open and Close
#define NUM_GESTURES     2  
#define NUM_MOTORS       5  


// Calibration params
#define BASELINE_SAMPLES 200    // samples for baseline
#define CAL_STD_SAMPLES  200    // samples per gesture to calibrate SD
#define STD_WINDOW       30     // rolling window size for live SD


const unsigned long GESTURE_HOLD_MS = 300; 
const unsigned int  ADC_SETTLE_US    = 10;  
const float SMOOTH_ALPHA            = 0.1; 


// EMG pin
const int emgPin = A0;


// Servo driver and channels
Adafruit_PWMServoDriver pwm;
#define SERVOMIN 150
#define SERVOMAX 600
const float angleOpen   =  30.0;   // degrees for “Open”
const float angleClosed = 150.0;   // degrees for “Close”


// Gesture names
const char* gestureNames[NUM_GESTURES] = { "Open", "Close" };


// For baseline & envelope
float baseline;
float env;


// Calibration storage
float gestureStd[NUM_GESTURES];
float thresholdSD;


// Rolling buffer for live SD
float stdBuffer[STD_WINDOW];
int   bufIndex     = 0;
bool  bufferFilled = false;


//—— Read & process single EMG channel ——
float readProcessed() {
 // throw away one reading for settling
 analogRead(emgPin);
 delayMicroseconds(ADC_SETTLE_US);
 float raw = analogRead(emgPin);
 float corr = raw - baseline;
 return corr > 0 ? corr : 0;  // rectify
}


//—— Send angle to all servos ——
void applyGesture(int g) {
 float target = (g == 0) ? angleOpen : angleClosed;
 uint16_t span = SERVOMAX - SERVOMIN;
 uint16_t pulse = SERVOMIN + uint16_t(target/180.0 * span);
 for (int m = 0; m < NUM_MOTORS; m++) {
   pwm.setPWM(m, 0, pulse);
 }
}


//—— Compute SD of an array ——
float computeSD(const float* data, int n) {
 float sum = 0, sumSq = 0;
 for (int i = 0; i < n; i++) {
   sum   += data[i];
   sumSq += data[i]*data[i];
 }
 float mean = sum / n;
 float var  = (sumSq / n) - (mean * mean);
 return sqrt(var);
}


//—— Baseline calibration ——
void calibrateBaseline() {
 Serial.println("Calibrating baseline—relax...");
 long acc = 0;
 for (int i = 0; i < BASELINE_SAMPLES; i++) {
   analogRead(emgPin);
   delayMicroseconds(ADC_SETTLE_US);
   acc += analogRead(emgPin);
   delay(5);
 }
 baseline = float(acc) / BASELINE_SAMPLES;
 Serial.print("Baseline = ");
 Serial.println(baseline, 1);
}


//—— SD calibration for Open & Close ——
void calibrateStdDev() {
 Serial.println("=== SD Calibration ===");
 for (int g = 0; g < NUM_GESTURES; g++) {
   Serial.print("Hold “");
   Serial.print(gestureNames[g]);
   Serial.println("” firmly until done...");
   // wait for user
   while (!Serial.available()) delay(10);
   Serial.read(); // clear keypress


   // temporary buffer
   float tmp[CAL_STD_SAMPLES];
   env = 0;
   for (int i = 0; i < CAL_STD_SAMPLES; i++) {
     tmp[i] = readProcessed();
     delay(10);
     Serial.print('.');
   }
   Serial.println(" done");


   gestureStd[g] = computeSD(tmp, CAL_STD_SAMPLES);
   Serial.print(" SD "); Serial.print(gestureNames[g]);
   Serial.print(" = "); Serial.println(gestureStd[g], 2);
 }
 // set threshold at midpoint
 thresholdSD = (gestureStd[0] + gestureStd[1]) / 2.0;
 Serial.print("Threshold SD = "); Serial.println(thresholdSD, 2);
 Serial.println("Calibration complete!\n");
}


void setup() {
 Serial.begin(115200);
 while (!Serial) { }
 pwm.begin();
 pwm.setPWMFreq(60);


 // start open
 applyGesture(0);


 calibrateBaseline();
 env = 0.0;
 calibrateStdDev();
}


void loop() {
 // —— allow dynamic threshold setting ——
 if (Serial.available()) {
   float newT = Serial.parseFloat();
   if (newT > 0) {
     thresholdSD = newT;
     Serial.print(">>> New thresholdSD = ");
     Serial.println(thresholdSD, 2);
   }
   while (Serial.available()) Serial.read(); // flush any extra chars
 }


 // read & envelope-smooth
 float x = readProcessed();
 env = SMOOTH_ALPHA * x + (1 - SMOOTH_ALPHA) * env;
  // insert into rolling buffer
 stdBuffer[bufIndex++] = env;
 if (bufIndex >= STD_WINDOW) {
   bufIndex = 0;
   bufferFilled = true;
 }
  // once buffer filled, compute live SD and classify
 if (bufferFilled) {
   float liveSD = computeSD(stdBuffer, STD_WINDOW);
   Serial.print("Live SD: "); Serial.print(liveSD, 2);
   Serial.print("  (Thresh: "); Serial.print(thresholdSD, 2);
   Serial.print(") -> ");


   int g = (liveSD > thresholdSD) ? 1 : 0;
   Serial.println(gestureNames[g]);
   applyGesture(g);
 }


 delay(GESTURE_HOLD_MS);
}
