# EuroshieldGranular
Granular Sampler for Euroshield Module by 1010Music

Granular Synth Effect for Euroshield by 1010Music

Input 1:	Audio Input
Input 2:	CV Input for Playback Ratio
Output 1:	Output from Granual Effect
Output 2:	Dry Output

MIDI In:	Gate IN
MIDI Out:	n/a
Upper Pot:	Grain Size (20-290ms)
Lower Pot:	None
LED 1:		active when Gate is high

When the granular Effect begins, it plays back the dry signal for the length of the grain length.
The Mixer behind the effect opens when the playback of the dry sample stopped (Gate goes HIGH + grainLength) and closes when the Gate closes

*When the Sample starts or ends at a high amplitude a clicking sound can occour. To prevent this, an envelope is used with a 5 ms attack and 5m release, timed at the begin and end of the sample.* (Not really working at the moment)
