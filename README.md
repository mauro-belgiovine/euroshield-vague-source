# Random Sequencer for 1010Music Euroshield
This is a Random Sequencer inspired by Music Thing's Turing Machine (https://musicthing.co.uk/collateral/TuringRev1Docs.pdf). 

# How to load the code
0. Install <a href="https://www.pjrc.com/teensy/tutorial.html">Teensy software</a> components
1. Load the code on Teensydino IDE, setup IDE's ports accordingly to your Teensy board version.
2. Connect the USB to your computer
3. Compile and Load the code

# How to use it
There are four Modes available on the module and it is possible to switch to the next mode by pressing the button on the Euroshield. Based on the active Mode (indicated by leds), the module operates in this way:
- **Mode A**: Emulates the original Turing Machine, sending out sequences and pulses based on the clock input
  - *Upper knob* -> probabilty of change
  - *Lower knob* -> scale
  - *CV Input #1* - Clock in
  - *CV Output #1* - Sequence step CV out
  - *CV Output #2* - Pulse-1 = for each clock, sends out a trigger when the first bit of register is 1.


------------------

# TODOs
- MODE A - *CV Input #2* - Offset/Sequence is added to the internal sequencer note. Current CV Output #1 is computed as `(input_1 + input_2)/2`. It would be better to use another approach (i.e. normalization). NOTE: this is disabled at the moment.
- Define the other 3 modes B,C,D
