# EuroshieldGranular
Granular Sampler for Euroshield Module by 1010Music

## Modified Audio Library
The Sampler can sample up to 1200 ms of Audio when using a modified Teensy Audio library. The variables in effect_granular.h and effect_granular.cpp needs to be changed from type int16_t to int32_t. The sketch then can use up to 64000 bytes sample memory. In my setup I was able to use 58000 bytes with 93% Memory Usage. (Teensy 3.5). The variable for the granular memory needs to be int32_t too.

The modified files are in this repository, make a backup first and copy them to: 

C:\Program Files (x86)\Arduino\hardware\teensy\avr\libraries\Audio

## In/Outs
* Input 1:	  Audio Input
* Input 2:	  CV Input for Playback Ratio
* Output 1: 	Output from Granual Effect
* Output 2:	  Dry Output

* MIDI In:	  Gate IN
* MIDI Out:	  n/a
* Upper Pot:	Grain Size (20-290ms)
* Lower Pot:	None
* LED 1:		  active when Gate is HIGH

*Note: The MIDI IN Port (Teensy Port 0) was used as a digital Input. *

## Function
When the granular Effect begins, it plays back the dry signal for the length of the grain. The Mixer behind the effect opens when the playback of the dry sample stopped (Gate goes HIGH + grainLength) and closes when the Gate closes. So the playback starts after the time of the grain lenght has passed (It needs to 'record' the sample first)

The grain size will update when the Gate goes HIGH. 

To reflect different CV Input Voltages, the maximum Value of the CVIn is saved for mapping the playback ratio. When you change the CV Source, it could be a good idea to reboot the module to reset the value.

*When the Sample starts or ends at a high amplitude a clicking sound can occur. To prevent this, an envelope is used with a 5 ms attack and 5m release, timed at the begin and end of the sample.* (Not really working at the moment)

## Known Issues

*There's still a clicking sound when the playback of a grain stops (I think). It's clearly audible when using waveforms (e.g. from a VCO) as an audio input. Haven't noticed it when using an audio book as the audio source. 

## ToDo

* Add Option to use a Trigger to start Sampling
* Adding SD-Card support
* Add Effects
* Option too choose between Freezing and Pitchshifting or use the two Ouputs for each of them
* Fork / Pull request for the modified Audio Library


