/*
	Name:       EuroshieldGranular.ino
	Created:	24.08.2018 14:06:30
	Author:     Triscus

	Granular Synth Effect for Euroshield by 1010Music

	https://github.com/Triscus/EuroshieldGranular

	Input 1:	Audio Input
	Input 2:	CV Input
	Output 1:	Output from Granual Effect
	Output 2:	Dry Output

	MIDI In:	Gate IN
	MIDI Out:	n/a
	Upper Pot:	Grain Size (20-290ms)
	Lower Pot:
	LED 1:		active when Gate is high

*/

#include "Audio.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <elapsedMillis.h>
#include <Bounce.h>

//uncomment here if you want the Serial Out or TFT
#define SERIAL_OUT
//#define TFT

//choose between modified Audio Library (64KB Sample Length) and Vanilla Audio Library (32K Sample Length)
#define GRANULAR_SIZE_32K
//#define GRANULAR_SIZE_64K

#ifdef TFT
#include "ILI9341_t3.h"
//---------------DISPLAY-------------------
#define DC 10
#define CS 15
#define RST 255
#define MOSI 7
#define SCLK 14
#define MISO 12

ILI9341_t3 tft = ILI9341_t3(CS, DC, RST, MOSI, SCLK, MISO);
elapsedMillis timeElapsed;
elapsedMillis serialMillis;
uint8_t displayRefreshInterval = 25;

#endif // TFT

//------------AUDIO------------------

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=401,243
AudioEffectEnvelope      envelope1;      //xy=561,110
AudioEffectGranular      granular1;      //xy=723,110
AudioAnalyzePeak         cvInput;          //xy=761,318
AudioMixer4              mixer1;         //xy=889,128
AudioOutputI2S           i2s2; //xy=1051,222
AudioConnection          patchCord1(i2s1, 0, i2s2, 1);
AudioConnection          patchCord2(i2s1, 0, envelope1, 0);
AudioConnection          patchCord3(i2s1, 1, cvInput, 0);
AudioConnection          patchCord4(envelope1, granular1);
AudioConnection          patchCord5(granular1, 0, mixer1, 0);
AudioConnection          patchCord6(mixer1, 0, i2s2, 0);
AudioControlSGTL5000     audioShield;     //xy=446,367
										 // GUItool: end automatically generated code

// GUItool: end automatically generated code

//---------------POTS-------------------

#define POT_MIN_VALUE 1
#define POT_MAX_VALUE 1023
#define POT_MAX_VALUE_F 1023.0

int upperPotInput = 20;
int lowerPotInput = 21;
float SHRT_MAX_AS_FLT = 32767.0;
float POT_MAX_AS_FLT = 1023.0;
int potInput[2] = { upperPotInput, lowerPotInput };
float potValue[2] = { 0.0,0.0 };
float potValueLast[2] = { 0.0,0.0 };
float smoothedPotValue[2] = { 0.0,0.0 };
bool potTracking[2] = { false,false };
float potValueFloat[2];

//----------BUTTON-----------
Bounce bouncer1 = Bounce(2, 100);
int buttonInput = 2;
int lastButtonState = -1;
long lastButtonMS = 0;
int  mode = 0;
int  lastmode = 0;
#define debounceMS    100

//----------LEDS---------------
#define ledPinCount   4
int     ledPins[ledPinCount] = { 3, 4, 5, 6 };
uint8_t ledState = 1;

//---------------CV INPUT--------------

double peakCV = 0.0;
double peakCVLastRawValue = 0.0;
double maxPeakCV;
#define GATE_IN0 0
#define GATE_IN1 1
bool gateInState[2];
bool gateInState_old[2];
bool gateInStateChanged[2];

//----GRANULAR EFFECT--------

#ifdef GRANULAR_SIZE_64K

#define GRANULAR_MEMORY_SIZE 58000  // Sample Length 1315 ms at 44.1 kHz
int32_t  granularMemory[GRANULAR_MEMORY_SIZE];

#endif 

#ifdef GRANULAR_SIZE_32K

#define GRANULAR_MEMORY_SIZE 32000  //  Sample Length 725 ms at 44.1 kHz
int16_t  granularMemory[GRANULAR_MEMORY_SIZE];

#endif 

#define MAX_SAMPLE_LENGTH round(GRANULAR_MEMORY_SIZE /44.1)
elapsedMillis envelopeMillis;
elapsedMillis granularMillis;
bool granularActive;
#define ATTACK_TIME 5
#define RELEASE_TIME 5

void setup()
{
#ifdef TFT
	//-----DISPLAY-----
	tft.begin();
	tft.setRotation(2);
	tft.fillScreen(ILI9341_BLACK);
	tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
	tft.setTextSize(1);
	tft.println("Initializing...");
#endif

	// the Granular effect requires memory to operate
	granular1.begin(granularMemory, GRANULAR_MEMORY_SIZE);

	Serial.begin(15200);

	//-----AUDIO-----
	AudioMemory(10);

	audioShield.enable();
	audioShield.inputSelect(AUDIO_INPUT_LINEIN);
	audioShield.volume(1.0);
	audioShield.adcHighPassFilterDisable();
	audioShield.lineInLevel(0, 0);

	//Envelope Configuration (to avoid clicking)
	envelope1.attack(ATTACK_TIME);
	envelope1.decay(0);
	envelope1.sustain(1);
	envelope1.release(RELEASE_TIME);
	//	envelope1.releaseNoteOn(10);

		//pin configuration
	pinMode(buttonInput, INPUT);

	//using MIDI IN/OUT as Digital Inputs
	pinMode(GATE_IN0, INPUT_PULLUP);
	pinMode(GATE_IN1, INPUT_PULLUP);

	//LEDS
	for (int i = 0; i < ledPinCount; i++)
		pinMode(ledPins[i], OUTPUT);

#ifdef TFT
	tft.fillScreen(ILI9341_BLACK);
	tft.setCursor(0, 0);
#endif
}

void loop()
{
	for (int i = 0; i < 2; i++) {
		//Reading the PotValues and smooth the values a bit
		int bothValues = potValueLast[i] + analogRead(potInput[i]);
		potValue[i] = bothValues / 2;
		potValue[i] = analogRead(potInput[i]);
		potValueLast[i] = potValue[i];

		//Convert PotValues to float (0.0-1.0)
		potValueFloat[i] = potValue[i] / POT_MAX_VALUE_F;

		// Read Digital Inputs (MIDI In/Out Jacks)
		gateInState[i] = digitalRead(i);

		// Detect a change of the Gate
		if (gateInState[i] != gateInState_old[i])

		{
			gateInStateChanged[i] = true;
			gateInState_old[i] = gateInState[i];
		}
		else

		{
			gateInStateChanged[i] = false;
		}
	}

	//read the CV Inputs / Trigger
	peakCV = peakCVLastRawValue;
	if (cvInput.available()) {
		peakCV = cvInput.read();

		if (maxPeakCV < peakCV)

		{
			maxPeakCV = peakCV;
		}
	}
	peakCVLastRawValue = peakCV;

	//Update Button State
	//bouncer1.update();

	// Continuously adjust the speed, based on the Lower pot
	float ratio = mapfloat(peakCV, 0.0, maxPeakCV, 0.125, 3.0);
	granular1.setSpeed(ratio);

	// Adjust the Grain Length (20-290 ms)
	float msec = 20.0 + (potValueFloat[0] * MAX_SAMPLE_LENGTH);

	// Hold Phase of the envelope (Grain Lenght - Attack - Release)
	envelope1.hold(msec - ATTACK_TIME - RELEASE_TIME);

	// If gate has change and is HIGH (State is reversed, HIGH is false)
	if (gateInStateChanged[0] == true && gateInState[0] == false)

	{
		envelope1.noteOn();				//Start enevlope
		granular1.beginFreeze(msec);	//Start sampling
	//	granular1.beginPitchShift(msec);
		granularActive = true;
		granularMillis = 0;				//reset the Timer of the Effect duration
		digitalWrite(ledPins[0], HIGH); // Light up LED 1
	}

	// If gate has change and is LOW (LOW is true)
	if (gateInStateChanged[0] == true && gateInState[0] == true)

	{
		mixer1.gain(0, 0.0);	// close Mixer
		granular1.stop();		// stop effect
		granularActive = false;
		digitalWrite(ledPins[0], LOW);	// dim LED 1
	}

	// Open the mixer if the effect is active longer than the first sample
	if (granularActive == true && granularMillis > msec)
	{
		mixer1.gain(0, 1.0);
	}

	if (envelope1.isActive() && (envelopeMillis > msec * 2 - RELEASE_TIME))
	{
		//envelope1.noteOff();
	}

#ifdef TFT
	tft.setCursor(0, 0);
	if (timeElapsed > displayRefreshInterval)

	{
		tft.print("msec:   ");
		tft.print(msec);
		tft.println("  ");
		tft.print("ratio:  ");
		tft.print(ratio);
		tft.println("  ");
		tft.println(peakCV);
		tft.println(maxPeakCV);
		//tft.println(potValueFloat[0]);
		//tft.println(potValue[0]);
		//tft.println(potValueFloat[1]);
		//tft.println(potValue[1]);
	}
#endif

#ifdef SERIAL_OUT
	if (serialMillis > 500)
	{
		Serial.print("Grain Length: ");
		Serial.print(msec);
		Serial.print("---");
		Serial.print("Ratio:");
		Serial.println(ratio);
		serialMillis = 0;
	}
#endif
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) //function for mapping floats
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

