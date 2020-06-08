# Vague Source for 1010Music Euroshield
Vague Source is a mix of pseudo-random techniques and utilities that I often use in my Eurorack explorations, condensed together in a single module. I have always been in interested in understanding and be able to program these kind of functialities in a module. The code is intended for the programmable 
[Euroshield](https://1010music.com/product/euroshield1) Eurorack module (now discontinued), but potentially could be adapted to other Teensy-based platforms. The project is open-source and welcome to contributions. As for now, the following modes are available:
  * `Mode A` : a random sequencer based on [Music Thing's Turing Machine](https://musicthing.co.uk/collateral/TuringRev1Docs.pdf). 
  * `Mode B`: a dual Bernoulli gate, inspired by [MI Branches](https://mutable-instruments.net/modules/branches/)
  * `Mode C`: TBD
  * `Mode D`: TBD
  
Note the default mode is `MODE B`. If you use this code, have comments or would like to contribute to the project, please drop me a line or consider buying me a beer by donating to my [Paypal](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=AYW2AZK29XNUS&currency_code=USD&source=url) account. 

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=AYW2AZK29XNUS&currency_code=USD&source=url)
 

# How to load the code
0. Install <a href="https://www.pjrc.com/teensy/tutorial.html">Teensy software</a> components
1. Load the code on Teensydino IDE, setup IDE's ports accordingly to your Teensy board version.
2. Connect the USB to your computer
3. Compile and Load the code

# How to use it
There are four Modes available on the module and it is possible to switch to the next mode by pressing the button on the Euroshield. Based on the active Mode , the module operates in this way:
* **MODE A**: Emulates the original Turing Machine, sending out sequences and pulses based on the clock input
  * *Upper knob*: probabilty of change
  * *Lower knob*: scale
  * *CV Input #1*: Clock in
  * *CV Output #1*: Sequence step CV out
  * *CV Output #2*: Pulse-1 = for each clock, sends out a trigger when the first bit of register is 1.

* **MODE B**: emulates two independent Bernoulli gates, whose probability is controlled by the upper and lower control knobs respectively. The input triggers are associated to the 0 and 1 leds of the the Euroshield, respectively, for visual feedback.
  * *Upper knob*: probability for trigger #1
  * *Lower knob*: probability for trigger #2
  * *CV Input #1*: trigger #1
  * *CV Input #2*: trigger #2
  * *CV Output #1*: trigger #1 output after probability 
  * *CV Output #2*: trigger # output after probability
  
------------------

# TODOs
- MODE A - *CV Input #2* - Offset/Sequence is added to the internal sequencer note. Current CV Output #1 is computed as `(input_1 + input_2)/2`. It would be better to use another approach (i.e. normalization). NOTE: this is disabled at the moment.
- Define the other modes C,D
