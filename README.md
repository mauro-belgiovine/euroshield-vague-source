# Vague Source for 1010Music Euroshield
Vague Source is a mix of pseudo-random techniques and utilities that I often use in my Eurorack explorations, condensed together in a single module. The code is intended for the programmable, Teensy-based 
[Euroshield](https://1010music.com/product/euroshield1) Eurorack module (now discontinued), but potentially could be adapted to other Teensy-based platforms. As for now, the following modes are available:
  * `Mode A` : a random sequencer based on [Music Thing's Turing Machine](https://musicthing.co.uk/collateral/TuringRev1Docs.pdf). 
  * `Mode B`: a dual Bernoulli gate, inspired by [MI Branches](https://mutable-instruments.net/modules/branches/)
  * `Mode C`: Random walk (*Proposed, not yet implemented*)
  * `Mode D`: Random burst clock (*Proposed, not yet implemented*)
  
Note the default mode is `MODE B`. This project is still in infancy and is moved mainly by my curiosity. My goal is to understand and possibly design new functionalities for Eurorack synths based on randomness. The project is open-source and welcome to contributions. If you use this code, have comments or would like to contribute to the project, please drop me a line or consider buying me a beer by donating to my [Paypal](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=AYW2AZK29XNUS&currency_code=USD&source=url) account. 

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
  * *CV Input #2* - Internal sequencer notes are added (scaled) to an input signal (i.e. another sequence that one wants to alter with some randomness from TM). See Appendix for additional details.
  * *CV Output #1*: Sequence step CV out
  * *CV Output #2*: Pulse-1 = for each clock, sends out a trigger when the first bit of register is 1.

* **MODE B**: emulates two independent Bernoulli gates, whose probability are controlled by the upper and lower control knobs, respectively. The input triggers are associated to the 0 and 1 leds of the the Euroshield, respectively, for visual feedback.
  * *Upper knob*: probability for trigger #1
  * *Lower knob*: probability for trigger #2
  * *CV Input #1*: trigger #1
  * *CV Input #2*: trigger #2
  * *CV Output #1*: trigger #1 output after probability 
  * *CV Output #2*: trigger # output after probability
  
------------------

## Appendix
### Turing Machine (Mode A)

Following are some screenshots of module's behaviour when in TM mode and an offset is inserted through `CV Input #2`. The output values for `CV Output #1` are depicted in yellow, while the corresponding value of `CV Input #2` is depicted in blue.

Note also that different clock resolution (from `*CV Input #1`) will lead to a different sampling rate, as the TM's ouptut value is selected in a sample-and-hold (S&H) manner.

![Example 1](img/screen1.png?raw=true "Example 1")
![Example 2](img/screen2.png?raw=true "Example 2")
------------------

# TODOs

- Define the other modes C,D
