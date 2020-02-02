# Random Sequencer for 1010Music Euroshield
This is a Random Sequencer inspired by Music Thing's Turing Machine (https://musicthing.co.uk/collateral/TuringRev1Docs.pdf). 

# How to load the code
1. Load the code on Arudino IDE, setup IDE's ports accordingly to your Teensy board version.
2. Connect the USB to your computer
3. Compile and Load the code

# How to use it
- *Upper knob* -> probabilty of change
- *Lower knob* -> scale
- *CV Input #1* - Clock in
- *CV Output #1* - Sequence step CV out
- *CV Output #2* - Pulse 

------------------

# TODOs
- (to be refined) *CV Input #2* - Offset/Sequence is added to the internal sequencer note. Current CV Output #1 is computed as `(input_1 + input_2)/2`. 
