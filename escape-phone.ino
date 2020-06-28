/* This sketch is the code for a puzzle featured in
 * Race 2 Escape escape room 4.
 * https://www.race2escape.net/
 *
 * An Arduino Nano (ATMega328) is used to detect pulses on an old rotary phone.
 * When the correct number is dialed, a relay connected on pin 5 is activated
 * Audio files are included on an SD card to play a message if the number is
 * incorrect. A track is also played when the receiver left off the hook.
 *
 * The SimpleSDAudio Library is used for audio playback.
 * It can be found here:
 * http://hackerspace-ffm.de/wiki/index.php?title=SimpleSDAudio#Download
 *
 * Written by Jackson Miller
 * Pinouts and more info in the README
 * https://github.com/jack-mil/escape-phone/
 *
 * Copyright (c) 2020 Jackson Miller
 *
 * MIT License:
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


// INCLUDE Library for audio playback
// Mono Speaker must be on D9
#include <SimpleSDAudio.h>

// <<< PIN DEFINITIONS >>>

// Chip select for SD with SPI
#define SD_chip_select_pin 10

// pulse_pin will pulse HIGH to count the digit dialed
// Analog pin only for wiring convenience
#define pulse_pin A5

// receiver_pin will go LOW when the receiver is lifted
#define receiver_pin A4

// relay pin is activated by going HIGH
#define relay_pin 5

// Activate debugging symbols
#define DEBUG

// <<< CONSTANTS >>>

// Secret Number!!
#define LENGTH 7
const char KEY[LENGTH+1] = "3526041";

const unsigned long debounceDelay = 40; // pulse debounce delay (ms)
const unsigned long maxPulseInterval = 350; // time between consecutive digits (ms)

const unsigned long timeoutDelay = 45000; //45 seconds

// <<< GLOBALS >>>
char number[LENGTH+1];
int currentDigit;
int pulseCount;

// State and initial state
typedef enum { ON_HOOK, OFF_HOOK, DIALLING, CONNECTED } stateType;
stateType state = ON_HOOK;

int previousPinReading = HIGH;
unsigned long timePinChanged;
unsigned long now = millis();

// setup() run once when Arduino powers on
void setup() {
	// Setup I/O pins
	// Using internal pullup resistors means pins will be LOW when switch is closed
	// Some digitalReads will be inverted (!) to account for this
	pinMode(pulse_pin, INPUT_PULLUP);
	pinMode(receiver_pin, INPUT_PULLUP);

	// The relay controls the output. It is triggered when relay_pin goes HIGH
	pinMode(relay_pin, OUTPUT);
	digitalWrite(relay_pin, LOW);

	SdPlay.setSDCSPin(SD_chip_select_pin);

	#ifdef DEBUG
		Serial.begin(9600);
		Serial.println("Connection started");
	#endif

	if (!SdPlay.init(SSDA_MODE_FULLRATE | SSDA_MODE_MONO| SSDA_MODE_AUTOWORKER)) {
		#ifdef DEBUG
			Serial.println("SD card not detected.");
		#endif
		return;
	} else {
		#ifdef DEBUG
			Serial.println("SD card initialized.");
		#endif
	}
}

// Repeatably called by Arduino
void loop() {

	// receiver_pin is HIGH when receiver is docked
	bool receiver_lifted = !digitalRead(receiver_pin);

	// Update receiver state machine
	if(receiver_lifted && state == ON_HOOK) {
		#ifdef DEBUG
			Serial.println("receiver lifted");
		#endif

		// Update state
		state = OFF_HOOK;
		// Reset these times so timer counts up from this point
		timePinChanged = now = millis();
	}
	else if(!receiver_lifted && state != ON_HOOK) {

		#ifdef DEBUG
			Serial.println("receiver replaced");
		#endif

		SdPlay.stop();

		// Update state
		state = ON_HOOK;

		// Reset counters
		pulseCount = 0;
		currentDigit = 0;

		// Reset number array to NUL characters
		for (int i = 0; i < LENGTH; i++) {
			number[i] = 0;
		}
	}

	// When the receiver is off hook, or in the middle of dialing,
	// watch the pulse_pin and count number dialed
	if(state == OFF_HOOK || state == DIALLING) {

		// Start or restart the dial tone while the
		// handset is off the hook
		if(SdPlay.isStopped()) {
			SdPlay.setFile("ltone.AFM");
			SdPlay.play();
		}

		now = millis();

		bool pinReading = digitalRead(pulse_pin);

		// When the pin has changed state, debounce, and increase count
		// on pulse rising edge
		if(pinReading != previousPinReading) {

			state = DIALLING;

			// Debounce pulse counting
			if(now - timePinChanged < debounceDelay) {
				return;
			}

			// PulseCount will represent this digit dialed
			if(pinReading == HIGH) { pulseCount++; }

			timePinChanged = now;
			previousPinReading = pinReading;
		}
		else if(now - timePinChanged > timeoutDelay){
			// If receiver has been off hook too long,
			// play message to prompt reset
			SdPlay.stop();
			if(!SdPlay.setFile("dial.AFM")){
				Serial.println("File not found");
			}
			SdPlay.play();
			timePinChanged = now;

			#ifdef DEBUG
				Serial.println("timeOutDelay Elapsed");
			#endif
		}
	}

	// When at least one pulse has been counted, and the time between
	// digits has elapsed, save dialed digit to key
	if(((now - timePinChanged) >= maxPulseInterval) && pulseCount > 0) {

		// If more digits to dial
		if (currentDigit < LENGTH) {

			// Mod by 10 because 10 pulses = "0"
			pulseCount = pulseCount % 10;

			#ifdef DEBUG
				Serial.print("Digit dialled: ");
				Serial.println(pulseCount);
			#endif

			// Save dialed number to character arrray (string)
			// OR by '0' ASCII x30 to convert int to correct char
			number[currentDigit] = pulseCount | '0';

			if(currentDigit == 0 && pulseCount == 0) {
				SdPlay.stop();
				SdPlay.setFile("rick.AFM");
				SdPlay.play();
			}
			currentDigit++;

			// Fill the next index with placeholder NUL char. 
			// Required to match key string ("xxx-xxxx\0")
			number[currentDigit] = 0;
		}

		// When enough numbers have been dialed
		if(currentDigit == LENGTH) {

			#ifdef DEBUG
				Serial.print("Number dialed: ");
				Serial.println(number);
			#endif

			if(strcmp(number, KEY) == 0) {

				// correct number was dialed
				#ifdef DEBUG
					Serial.println("UNLOCKED");
				#endif

				SdPlay.stop();

				// Random Rick-Roll
				if(random(100) < 5) {
					SdPlay.setFile("rick.AFM");
					SdPlay.play();
				}
				
				// Trigger relay to unlock whatever
				digitalWrite(relay_pin, HIGH);
				delay(100);
				digitalWrite(relay_pin, LOW);

				// Idle until receiver is replaced
				while(!digitalRead(receiver_pin)){ delay(1000); }
			}
			else {
				// Incorrect number dialed
				// Play vacant number
				#ifdef DEBUG
					Serial.println("INCORRECT NUMBER");
					Serial.println("Hang up and dial again");
				#endif

				SdPlay.stop();
				if(!SdPlay.setFile("vacant.AFM")) {
					Serial.println("File not fount");
				}
				SdPlay.play();
			}

			// Reset state to initiate timeout if dial not resumed
			state = OFF_HOOK;
		}

		// Reset pulse for next digit
		pulseCount = 0;
	}
}
/* vim: set noexpandtab ts=4 sw=4: */
