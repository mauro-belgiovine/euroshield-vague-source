/* 
 *  
 *

Mauro Belgiovine, Sunday 24/11/2019
 
Vague Source

Copyright © 2020 Mauro Belgiovine

Permission is hereby granted, free of charge, to any person 
obtaining a copy of this software and associated documentation 
files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, 
publish, distribute, sublicense, and/or sell copies of the Software, 
and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 *
 *
 *
 */
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h> 

enum { MODE_A, MODE_B, MODE_C, MODE_D, MODE_E, MODE_F, MODE_G, MODE_H, MODE_I, MODE_J, MODE_K, MODE_M, MODE_N, MODE_O, MODE_P, MODE_Q };
#define NMode 4     // IMPORTANT: this will define the number of modes available (up to 16 for now, due to number of LEDs used as indicators (4))
#define intrvlUI 1000

#define buttonPin     2
#define upperPotInput 20
#define lowerPotInput 21
#define kMaxPotInput  1024
#define ledPinCount   4
#define debounceMS    100

//#define VERBOSE
bool led_LOCK = false;

int     ledPins[ledPinCount] = { 3, 4, 5, 6 };
int     clockUp = 0;
int     clockUp2 = 0;
float   seqSteps[32];
float   chance = 0;
int     tmMode = MODE_A;  // mode at startup
int     stepNumber = 0;
float   lastAmplitude = 0.5;
int     lastSeqLength = 16;
float   last_input_1 = 0;
float   last_input_2 = 0;
long    lastBeatMS = 0;
long    lastButtonMS = 0;
int     lastButtonState = -1;
int     clockRate = 250;


AudioInputI2S            audioInput;
AudioAnalyzeRMS          input_1;
AudioAnalyzeRMS          input_2;
AudioSynthWaveformDc     dc2;
AudioSynthWaveformDc     dc1;
AudioOutputI2S           audioOutput;
AudioConnection          patchCord1(audioInput, 0, input_1, 0);
AudioConnection          patchCord2(audioInput, 1, input_2, 0);
AudioConnection          patchCord3(dc1, 0, audioOutput, 0);
AudioConnection          patchCord4(dc2, 0, audioOutput, 1);
AudioControlSGTL5000     sgtl5000_1;

AudioMixer4              inputDC1;
AudioMixer4              inputDC2;
AudioConnection          patchCord5(audioInput, 0, inputDC1, 0);
AudioConnection          patchCord6(audioInput, 1, inputDC2, 0);

#define regBits 16  // num of bits considered for register
#define voctType uint16_t
byte b[4] = { random(16), random(16), random(16), random(16) } ; // change with appropriate function
uint32_t register_32b = (b[3] << 24) | (b[2] << 16) | ( b[1] << 8 ) | (b[0]); //register initial status (random)
voctType voct = register_32b >> regBits;
byte last = 0;
byte flip = 0; //DEBUG only
float trig1;
float trig2;
float probKnob = 0;
float probKnob2 = 0;
bool isPulse = false;
bool isPulse2 = false;
uint32_t pulse_duration = 25; //ms
uint32_t pulse_last;
uint32_t pulse_last2;

float in_offset = 0;


void setup()
{
  AudioMemory(12);

  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.volume(0.82);
  sgtl5000_1.adcHighPassFilterDisable();
  sgtl5000_1.lineInLevel(0,0);
  sgtl5000_1.unmuteHeadphone();

  pinMode(buttonPin, INPUT);

  for (int i = 0; i < ledPinCount; i++)
    pinMode(ledPins[i], OUTPUT);

  // populate sequencer memory with random values
  for (int i = 0; i < 32; ++i) {
    seqSteps[i] = random(1024)/1024.0;
  }
  lastBeatMS = millis();

  #ifdef VERBOSE
  Serial.begin(38400);
  #endif
}


// LEDs UI
void writeLED(unsigned int p, unsigned int state){
  if (!led_LOCK) {
    digitalWrite(ledPins[p], state);
  }
}

void showState(unsigned int mode)
{
  int N = 4; //log2(NumPins)
  for (unsigned int n=0; n < N; n++){
    // first reset the current pin state
    digitalWrite(ledPins[n], LOW);
    bool b = bitRead(mode, n);
    if (b) digitalWrite(ledPins[n], HIGH);
  }
}


void printBin(unsigned n) 
{ 
    unsigned int i; 
    for (i = 1 << 31; i > 0; i = i / 2) 
        (n & i)? Serial.print("1"): Serial.print("0"); 
} 

/* 
* TODO
* Put all the functions of each mode in separate source files. 
* Put all these files in a separate folder.
*/


void BernoulliGate(AudioAnalyzeRMS *input, float *trig, float *last_input,  int *clockUp, bool *isPulse, int *pulse_last, AudioSynthWaveformDc *dc, int potInput, int lp){

  /* 
   *  input_N = CLK/PULSE/GATE, 
   *  out_N = a pulse after a probability set by knob_N
  */
  
  float probKnob = 0;
  float chance = 0;
  
  if (input->available()) {
    *last_input = input->read();
  }

  // LED 0-1 are assigned to lower input, LED 2-3 to upper. 
  // We invert LED pin (lp) for better visualization (pins order goes from lower to higher)
  lp = 1 - lp; // basically a NOT if lp can be either [0,1]
  
  *trig = *last_input;
  
  if (*trig > 0.75) {
    
    // we are starting a new beat
    if (*clockUp == 0) { 
      writeLED(lp*2, HIGH);    // this LED monitors the input pulses
      *clockUp = 1;
      chance = (random(1024)/1023.0); // NOTE random(m) function returns an integer in the interval [0,m-1]
      probKnob = analogRead(potInput)/1023.0; // 0-1023 
      
      if (chance < probKnob) { //  if change must occur, flip the last value
        *isPulse = true;
        *pulse_last = millis();
      }
    }

  }else{
    // else trig is down
    if (*clockUp > 0) {   
      writeLED(lp*2, LOW);
      *clockUp = 0;       
    }
  }

  if((millis() - *pulse_last) >= pulse_duration)
    *isPulse = false;

  if (*isPulse){
    dc->amplitude(1.0);
    writeLED(lp*2+1, HIGH);  // the LED above the current clock monitor is used 
                                          // to give visual feedback of output pulses 
                                          // (i.e. after probability is applied to IN pulse)
  }else{
    dc->amplitude(0.0);
    writeLED(lp*2+1, LOW);
  }

}

#define Nmem 5  //important!! change the init of cv_mem variable (next line) if you want dynamic allocation
float cv_mem[Nmem] =  {0, 0, 0, 0, 0};
float offset_max = 0;
unsigned int mem_wridx = 0;

float memMax(){
  
  float max_v = 0;
  /*for(int i=0; i<Nmem; i++){
      Serial.print("mem[");
      Serial.print(i);
      Serial.print("]=");
      Serial.print(cv_mem[i]);
      Serial.println(" ");
    }*/
  for ( int i = 0; i < Nmem; i++ )
  {
    if ( cv_mem[i] > max_v )
    {
      max_v = cv_mem[i];
    }
    
  }
  /*Serial.print("Max is ");
  Serial.println(max_v);*/
  return max_v;
  
}

void turingMachine()
{
  /* 
   *  input_1 = CLK, 
   *  input_2 = offset/sequence, 
   *  out1 = sequence, 
   *  out2 = pulse1 (first bit of register)
  */
  if (input_1.available()) {
      last_input_1 = input_1.read();
  }

  if (input_2.available()) {
      in_offset = input_2.read();   // TODO WE ARE READING THE RMS, NOT THE REAL INPUT
      //in_offset = inputDC2.read();
  }
  
  
  trig1 = last_input_1;    
  if (trig1 > 0.5) {
    if (clockUp == 0) { // we are starting a new beat
      clockUp = 1;
      writeLED(0, HIGH);
      chance = (random(1024)/1023.0); // NOTE random(m) function returns an integer in the interval [0,m-1]
      
      flip = 0; // debug only 
      last = register_32b & 1;
      //  compute probability. determine if change is going to occur
      probKnob = analogRead(upperPotInput)/1023.0; // 0-1023 
      if (chance < probKnob) { //  if change must occur, flip the last value
        flip = 1;
        last = ~last & 1;
      }
      
      register_32b = register_32b >> 1; // right shift, 1 bit
      register_32b = (last << 31) | register_32b;
      
      if ((register_32b >> 31)) {
        isPulse = true;
        pulse_last = millis();
      }

      //moving maxima on offset (so that when it stays 0., the output value is not constrained to 0.5)
      mem_wridx = (mem_wridx + 1) % Nmem;  // compute the current memory 
      
      cv_mem[mem_wridx] = in_offset;   // save current offset input into the mem_wridx-th array cell
      offset_max = memMax(); // get current max

      float lowerPotVal = analogRead(lowerPotInput)/1024.0; // [0..1]
      voct = register_32b >> regBits;
      float max_voct = pow(2.0, regBits) - 1;
      float min_voct = pow(2.0, 0) - 1; // i.e. 0
      float norm_voct = ((((float) voct) - min_voct)/ (max_voct - min_voct)); // [0..1] from 16 bit variable
      //norm_voct = (2*norm_voct-1) *(lowerPotVal);
      //dc1.amplitude(write_val);
      
      //float norm_writeval = (norm_voct + in_offset)/(1.0+offset_max);  // constrain to [0,1] based on current 
      float norm_writeval = norm_voct;
      float write_val = (2* -1)*lowerPotVal + in_offset; // move to [-1,1] range and apply scaling. Add 
      // avoid clipping 
      if (write_val > 1.0) write_val = 1.0;   
      if (write_val < -1.0) write_val = -1.0;  
      // write the output value
      dc1.amplitude(write_val);

      Serial.print("norm_voct: ");
      Serial.print(norm_voct);
      Serial.print(" in_offset: ");
      Serial.print(in_offset);
      Serial.print(" offset_max: ");
      Serial.println(offset_max);
      Serial.print(" writeval  ");
      Serial.println(write_val);
    }
     
  }else {
    // else trig is down 
    if (clockUp > 0) {  
            writeLED(0, LOW);
            clockUp = 0;   
        }
  }

  if((millis() - pulse_last) >= pulse_duration)
        isPulse = false;
  
  if (isPulse){
    dc2.amplitude(1.0); 
  }else{
    dc2.amplitude(0.0);
  }

  #ifdef VERBOSE
  Serial.println(last_input_1); // TODO: SOMEHOW THE INPUT from MN 0-coast "CLK" IS NOT PERFECTLY SQARE AND AFFECTS THE NOTE PITCH (1 V/OCT) WITH A SORT OF BENDING OF THE NOTE. 
  Serial.println(last_input_2);
  
  // PRINT register bits 
  printBin(register_32b);
  //if (flip) Serial.print(" -- Flip");
  if ((register_32b >> 31)) Serial.print(" -- pulse");
  Serial.println();
  Serial.println(dc2.read());
  #endif

  //dc1.amplitude((write_val + in_offset)/2.0); // [0..1] from 16 bit variable
  // TODO:  not really what I want  ^^^    --->   if we remove the offset, the max value would be 0.5. 
  //        Find another solution that permits also normal turing [0..1 values]  when nothing is connected to input_2
  //  dc1.amplitude(((write_val + in_offset) - min_v) /(max_v - min_v)) 
  //            where min_v and max_v are the minimum/maximum value of (write_val + in_offset)
  //            in this way we normalize everything between 0-1
  // QUESTION: what's the output range for the turing machine? Maybe we should set that as well through an offset somehow?

  
}

void dualBernoulliGate()
{
  BernoulliGate(&input_1, &trig1, &last_input_1,  &clockUp, &isPulse, &pulse_last, &dc1, upperPotInput, 0);
  BernoulliGate(&input_2, &trig2, &last_input_2,  &clockUp2, &isPulse2, &pulse_last2, &dc2, lowerPotInput, 1);
}

int currentShownState = 0;

void loop()
{

  // Check for button presses here...
  int buttonState = digitalRead(buttonPin);

  long ms = millis();
  
  if (buttonState != lastButtonState) {
      if (ms - lastButtonMS > debounceMS) {
        if (buttonState == LOW) {

          //visual feedback          
          if (!led_LOCK){
            // acquire lock
            led_LOCK = true;
            showState(tmMode); // state will be shown until intrvlUI expires
            Serial.println("lock LED - ON");
            currentShownState = tmMode;
          }else{
            // show next state
            currentShownState = (currentShownState + 1) % NMode;
            showState(currentShownState); // state will be shown until intrvlUI 
          }
          Serial.print("Showing State ");
          Serial.println(currentShownState);
         
        }
      }
      lastButtonMS = ms;
      lastButtonState = buttonState;
  }

  // check timer to unlock leds
  if (((ms - lastButtonMS) > intrvlUI) && led_LOCK){
    led_LOCK = false;
    Serial.println("lock LED - OFF");
    if(currentShownState != tmMode) {
      // go to next mode
      tmMode = currentShownState;
      // reset clock
      clockUp = 0;
    }
  }


  if(tmMode == MODE_A)
  {
    turingMachine(); 
  } 
  else if(tmMode == MODE_B)
  {
    dualBernoulliGate();
  } 
  else if (tmMode == MODE_C)
  {
    //TODO
  } 
  else if (tmMode == MODE_D)
  {
    //TODO
  }
}
