/* 
 *  
 *
 This is a random sequencer inspired by Music Thing's Turing Machine. 
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

enum { MODE_A, MODE_B, MODE_C, MODE_D };

#define buttonPin     2
#define upperPotInput 20
#define lowerPotInput 21
#define kMaxPotInput  1024
#define ledPinCount   4
#define debounceMS    100

int     ledPins[ledPinCount] = { 3, 4, 5, 6 };
int     clockUp = 0;
int     clockUp2 = 0;
float   seqSteps[32];
float   chance = 0;
int     tmMode = MODE_B;
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

byte b[4] = { random(16), random(16), random(16), random(16) } ; // change with appropriate function
uint32_t register_32b = (b[3] << 24) | (b[2] << 16) | ( b[1] << 8 ) | (b[0]); //register initial status (random)
uint16_t voct = register_32b >> 16;
byte last = 0;
//byte flip = 0; //DEBUG only
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

void printBin(unsigned n) 
{ 
    unsigned i; 
    for (i = 1 << 31; i > 0; i = i / 2) 
        (n & i)? Serial.print("1"): Serial.print("0"); 
} 

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

  //Serial.begin(38400);

  //Serial.println(register_32b);
  //Serial.println(voct);
  
}


/*TODO:
 * 
[DONE]  FIRST STEP: GENERATE JUST RANDOM DATA AT EVERY CLOCK
[DONE]  SECOND STEP: read the music thing documentation https://musicthing.co.uk/collateral/TuringRev1Docs.pdf
              try emulate the functioning of that
*
*/
void loop()
{

  // Check for button presses here...
  int buttonState = digitalRead(buttonPin);
  
  if (buttonState != lastButtonState) {
      long ms = millis();
      if (ms - lastButtonMS > debounceMS) {
        if (buttonState == LOW) {
          tmMode = (tmMode + 1) % 4;
          //digitalWrite(ledPins[2], (tmMode & 1) > 0? HIGH : LOW);
          //digitalWrite(ledPins[3], (tmMode & 2) > 0? HIGH : LOW);
          clockUp = 0;
        }
      }
      lastButtonMS = ms;
      lastButtonState = buttonState;
  }


  if(tmMode == MODE_A){
    /* 
     *  input_1 = CLK, 
     *  input_2 = offset/sequence, 
     *  out1 = sequence, 
     *  out2 = pulse1 (first bit of register)
    */
    if (input_1.available()) {
        last_input_1 = input_1.read();
        //Serial.println(last_input_1); // TODO: SOMEHOW THE INPUT from MN 0-coast "CLK" IS NOT PERFECTLY SQARE AND AFFECTS THE NOTE PITCH (1 V/OCT) WITH A SORT OF BENDING OF THE NOTE. 
    }
  
    if (input_2.available()) {
        last_input_2 = input_2.read();
        //Serial.println(last_input_2);
        //in_offset = ((last_input_2/1.0)-.5);
        in_offset = last_input_2;
    }
    
    trig1 = last_input_1;    
    if (trig1 > 0.5) {
      if (clockUp == 0) { // we are starting a new beat
        clockUp = 1;
        digitalWrite(ledPins[0], HIGH);
        chance = (random(1024)/1023.0); // NOTE random(m) function returns an integer in the interval [0,m-1]
        
        //flip = 0; // debug only 
        last = register_32b & 1;
        //  compute probability. determine if change is going to occur
        probKnob = analogRead(upperPotInput)/1023.0; // 0-1023 
        if (chance > probKnob) { //  if change must occur, flip the last value
          //flip = 1;
          last = ~last & 1;
        }
        
        register_32b = register_32b >> 1; // right shift, 1 bit
        register_32b = (last << 31) | register_32b;
        
        
        /*
        // PRINT register bits 
        printBin(register_32b);
        //if (flip) Serial.print(" -- Flip");
        if ((register_32b >> 31)) Serial.print(" -- pulse");
        Serial.println();
        */
        
        if ((register_32b >> 31)) {
          isPulse = true;
          pulse_last = millis();
        }
  
        
      }
       
    }else {
      // else trig is down 
      if (clockUp > 0) {  
              digitalWrite(ledPins[0], LOW);
              clockUp = 0;       
          }
    }
  
    if((millis() - pulse_last) >= pulse_duration)
          isPulse = false;


    
    float lowerPotVal = analogRead(lowerPotInput)/1024.0; // [0..1]
    voct = register_32b >> 16; 
    float write_val = (((float) voct) / 65535.0)*lowerPotVal; // [0..1] from 16 bit variable
    dc1.amplitude(write_val);
    
    if (isPulse){
      dc2.amplitude(1.0);
    }else{
      dc2.amplitude(0.0);
    }

    //Serial.println(dc2.read());
    //dc1.amplitude((write_val + in_offset)/2.0); // [0..1] from 16 bit variable
    // TODO not really what I want  ^^^    --->   if we remove the offset, the max value would be 0.5. 
    // Find another solution that permits also normal turing [0..1 values]  when nothing is connected to input_2
      
  }else if(tmMode == MODE_B){

    if (input_1.available()) {
      last_input_1 = input_1.read();
    }
  
    if (input_2.available()) {
      last_input_2 = input_2.read();
    }

    /***** BERNOULLI GATE 1 *******/
    trig1 = last_input_1;
    
    if (trig1 > 0.5) {
      
      // we are starting a new beat
      if (clockUp == 0) { 
        digitalWrite(ledPins[0], HIGH);
        clockUp = 1;
        chance = (random(1024)/1023.0); // NOTE random(m) function returns an integer in the interval [0,m-1]
        probKnob = analogRead(upperPotInput)/1023.0; // 0-1023 
        
        if (chance < probKnob) { //  if change must occur, flip the last value
          isPulse = true;
          pulse_last = millis();
        }
      }

    }else{
      // else trig is down
      if (clockUp > 0) {   
        digitalWrite(ledPins[0], LOW);
        clockUp = 0;       
      }
    }

    if((millis() - pulse_last) >= pulse_duration)
      isPulse = false;

    if (isPulse){
      dc1.amplitude(1.0);
    }else{
      dc1.amplitude(0.0);
    }

    /****** BERNOULLI GATE 2 ********/
    trig2 = last_input_2;
    
    if (trig2 > 0.5) {
      
      // we are starting a new beat
      if (clockUp2 == 0) { 
        digitalWrite(ledPins[1], HIGH);
        clockUp2 = 1;
        chance = (random(1024)/1023.0); // NOTE random(m) function returns an integer in the interval [0,m-1]
        probKnob2 = analogRead(lowerPotInput)/1023.0; // 0-1023 

        if (chance < probKnob2) { //  if change must occur, flip the last value
          isPulse2 = true;
          pulse_last2 = millis();
        }
      }

    }else{
      // else trig is down
      if (clockUp2 > 0) {   
        digitalWrite(ledPins[1], LOW);
        clockUp2 = 0;       
      }
    }

    if((millis() - pulse_last2) >= pulse_duration)
      isPulse2 = false;

    if (isPulse2){
      //Serial.println(1);
      dc2.amplitude(1.0);
    }else{
      //Serial.println(0);
      dc2.amplitude(0.0);
    }

    //Serial.println(input_2.read());

    
  } else if (tmMode == MODE_C){
  } else if (tmMode == MODE_D){
  }
}