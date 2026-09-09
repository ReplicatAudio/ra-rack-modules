depth mirror?
reflecting pool?


# Mothership
- LFO/VCO module
- 2-way LFO/VCO switch
- glocal freq knob/cv
    - when cv in is provided, the knob acts as an attenuator
- global fm knob/cv
    - when cv in is provided, the knob acts as an attenuator
    - Otherwise knob does nothing
- global phase knob/cv
    - when cv in is provided, the knob acts as an attenuator
- 8 outs
- Each osc has a knob and cv in for positional control 
    - saw up -> square
    - when cv in is provided, the knob acts as an attenuator
- Each osc has a 2-way switch that inverts the output
- each osc has a phase offset knob/cv
    - when cv in is provided, the knob acts as an attenuator
- Each osc has a filer cutoff knob/cv
    - when cv in is provided, the knob acts as an attenuator
- Each osc has a fm knob/cv
    - when cv in is provided, the knob acts as an attenuator
    - Otherwise knob does nothing

add detune to each voice

# Note router
- 8 in 8 out
- auto quant
- screen at the top to show editing channel
- each channel can be note->note or note->trig

# Metal drums
Hat, cymbol and ride are redundant with ra-meteor
This is a superset of all 3


# renames

ra-dseq -> ra-vash

ra-endless -> ra-reflectingpool

ra-seer -> ra-seer-mini
ra-dscope -> ra-seer

ra-freeverb -> ra-freeberd

# lsys
L-system drum/trigger module. 

Similar to ra-vash

Variables/constants are represented by 8 colors:
off/black
red
green
blue
yellow
cyan
magenta
white

Variables/constants are represented by colored LEDs and can be clicked to change their color/value. 

At the top, there is an axiom row which is a row of 8 led buttons that can be used to define the axiom. 

Below that there are 6 rules rows which have 1 target cell and 6 result cells.

To the right there is an 8x8 non-editable matrix that shows the l-system output using the same color conventions. This will truncate the output as nessary. This is also used to show the current sequencer position like ra-vash.   

To the right of that there are 7 outputs for each variable constant. 

This also needs a step button/cv input that steps the sequencer forward. 

# logic gate
- 2 inputs
- mode button
- 
