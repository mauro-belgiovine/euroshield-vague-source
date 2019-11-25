/* 
 *  
 *  
       
 This is a liberally inspiredMusic Thing's Turing Machine-like sequencer. 
 It will be a personal attempt to a quasi random sequencer 
 
 The code used as a Base is by Jim Bumgardner (8/11/2018) and all its code rights are reserved to him 
 (now there is only some commented code, that will be used as my code base).


 Mauro Belgiovine, Sunday 24/11/2019

Following, the original code comments for reference:

  *
  ********
  ********
  *

 An implementation of a (Make Noise) Turing-Machine style random-looping sequencer for the 1010music Euroshield.  
   Good for Steve Reichian patterns that slowly mutate.
   I don't own one of these, but used the manual for the simpler 2hp TM module as a reference.
 
   -- Jim Bumgardner 8/11/2018

BUTTON toggles modes A B C D  (feedback on LEDS 3+4 is supposed to indicate mode)
       Mode A   Knob 2 Controls Amplitude,    CV-IN-2 controls amplitude.
       Mode B   Knob 2 controls step-length,  CV-IN-2 controls amplitude
       Mode C   Knob 2 controls amplitude,    CV-IN-2 controls step-length.
       Mode D   Knob 2 controls step length., CV-IN-2 controls step length.

     LED 3 indicates mode of KNOB 2
     LED 4 indicates mode of CV-IN-2


CV-IN 1
     Clock/Trigger - used to control our speed.
     NOTE: In my first test, I was accidentally using input-2 for everything, which was kind of interesting... It
     might have resulted in the nice polyrhythmic effect I was getting..

CV-IN 2
     Controls Amplitude or step-length, depending on mode (store last value otherwise). (may want to play with probabilty as well).
     LED 4 indicates mode of CV-IN-2

CV-OUT 1
     Step-wise output

CV-OUT 2
     Smooth output (kinda wonky at the moment, I may change how this works in the future...).

KNOB 1. 
     Probability
        Goes from Full Random to Locked Sequence
          (so likelihood that each step of sequence is a new value from 0->100%)
          I'm getting a very Steve Reichian effect with the knob at around 80% and the output going into a scale quantizer.

KNOB 2.
   Controls amplitude (interval size) or step-length.  When combined with CV (modes A,D), it provides an offset which the CV adds to.
   LED 3 indicates mode of KNOB 2
   
At the moment, I'm only seeing LEDs 1 and 2, so I suspect my button detection needs work...

LED 1. On during beat-1 of sequence.
LED 2. On 1st half every beat (square duty).  Probably need to shorten this.
LED 3. Mode indicator for KNOB 2
LED 4. Mode indicator for CV-IN-2

TODO: Work out how MIDI works. And use MIDI-IN for clock and MIDI-OUT for note trigering or CC.
 
Rev 1  First try, leds weren't working and I was using CV-IN-2 for both inputs.
Rev 2  Fixed the LEDs, button press not working..
Rev 3  Fixed the button and debounced it.

 *********************************************/
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
float   seqSteps[32];
float   out1 = 0;
int     tmMode = 0;
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

  Serial.begin(38400);
}


/*TODO:
 * 
[DONE]  FIRST STEP: GENERATE JUST RANDOM DATA AT EVERY CLOCK
[ -- ]  SECOND STEP: read the music thing documentation https://musicthing.co.uk/collateral/TuringRev1Docs.pdf
              try emulate the functioning of that
*
*/
void loop()
{
  
 if (input_1.available()) {
      last_input_1 = input_1.read();
      //Serial.println(last_input_1); // TODO: SOMEHOW THE INPUT from MN 0-coast "CLK" IS NOT PERFECTLY SQARE AND AFFECTS THE NOTE PITCH (1 V/OCT) WITH A SORT OF BENDING OF THE NOTE. 
  }

  if (input_2.available()) {
      last_input_2 = input_2.read();
  }
  
  float trig1 = last_input_1;
  
  if (trig1 > 0.5) {


    if (clockUp == 0) { // we are starting a new beat
      clockUp = 1;
      digitalWrite(ledPins[1], HIGH);
      out1 = (random(1024)/1024.0);

      
    }
  }else {
    if (clockUp > 0) { // else trig is down  
            digitalWrite(ledPins[1], LOW);
            clockUp = 0;       
        }
  }
  
  
  dc1.amplitude(out1);  
  //Serial.println(dc1.read());
  
}



/*
float getAmplitude()
{
    switch (tmMode) {
    case MODE_A: // knob only
       lastAmplitude = (analogRead(lowerPotInput)-512) / 512.0;
       break;
    case MODE_B: // cv
       lastAmplitude = last_input_2;
       break;
    case MODE_C: // both knob and cv
       lastAmplitude = last_input_2 + (analogRead(lowerPotInput)-512) / 512.0;
       break;
    case MODE_D: // no change
       break;
    }
    return lastAmplitude;
}

int getSeqLength() { // 1->32
    switch (tmMode) {
    case MODE_A:  // NO CHANGE
        //Serial.println("A");
        break;
    case MODE_B:  // KNOB
        //Serial.println("B");
        lastSeqLength = 1 + analogRead(lowerPotInput)/32;
        break;
    case MODE_C:  // CV
        //Serial.println("C");
        lastSeqLength = 1 + (int) (last_input_2*32.0);
        break;
    case MODE_D:  // CV, KNOB
        //Serial.println("D");
        lastSeqLength = 1 + analogRead(lowerPotInput)/32 + (int) (last_input_2*32);
    }
    lastSeqLength = constrain(lastSeqLength, 1, 32);
    return lastSeqLength;
}

void loop()
{
    // Check for button presses here...
    int buttonState = digitalRead(buttonPin);
    
    if (buttonState != lastButtonState) { // Probably needs debouncing, can't tell yet cause LEDs didn't work when I tested...
        long ms = millis();
        if (ms - lastButtonMS > debounceMS) {
          if (buttonState == LOW) {
            tmMode = (tmMode + 1) % 4;
            digitalWrite(ledPins[2], (tmMode & 1) > 0? HIGH : LOW);
            digitalWrite(ledPins[3], (tmMode & 2) > 0? HIGH : LOW);
          }
        }
        lastButtonMS = ms;
        lastButtonState = buttonState;
    }

    if (input_1.available()) {
        last_input_1 = input_1.read();
        //Serial.println(last_input_1); // TODO: SOMEHOW THE INPUT IS NOT PERFECTLY SQARE AND AFFECTS THE NOTE PITCH (1 V/OCT) WITH A SORT OF BENDING OF THE NOTE. 
    }

    if (input_2.available()) {
        last_input_2 = input_2.read();
    }

    float trig = last_input_1;
    // check if we are starting a new beat
    if (trig > 0.5) {

        if (clockUp == 0) { // we are starting a new beat
            clockUp = 1;
            long tm = millis();
            clockRate = tm - lastBeatMS;  // ms since last beat
            
            lastBeatMS = tm; 

            //  compute probability. determine if change is going to occur
            unsigned int probKnob = analogRead(upperPotInput); // 0-1023 
            
            if (random(1024) > probKnob) { //  if change must occur, change the step level
                float amp_k2 = getAmplitude(); // get amplitude from #2 knob
                out1 = 0.5 + (amp_k2 * (random(1024)-512)/512.0)*0.5;
                out1 = constrain(out1, 0.0, 1.0);     
            } else {//  else (no change)
                out1 = seqSteps[stepNumber];
            }

            
            dc1.amplitude(out1);            // stepped version...
            
            //dc2.amplitude(out1, clockRate); // linear interpolate version (sounds wonky at the moment...)
            seqSteps[stepNumber] = out1;    // save current step for re-use
            digitalWrite(ledPins[0], stepNumber == 0? HIGH : LOW);
            digitalWrite(ledPins[1], HIGH);
                


            stepNumber += 1;
            
            // determine next stepNumber
            int seqLength = getSeqLength();
            if (stepNumber >= seqLength) {
                stepNumber = 0;
            }
        }
    } 
    else { // else trig is down
        if (clockUp > 0) {
            clockUp = 0;
            digitalWrite(ledPins[1], LOW);
        }
    }

    Serial.println(dc1.read());
}
*/
