// Library and chip select for SD with SPI
#include <SD.h>
#define SD_chip_select_pin 10

// Library and pin for audio playback
#include <SimpleSDAudio.h>
#define speaker_pin 9

// PIN DEFINITIONS
// pulse_pin will pulse HIGH count the digit dialed
#define pulse_pin 2

// reciever_pin will go LOW when the reciever is lifted
#define reciever_pin 4

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

const unsigned long timeoutDelay = 30000; //30 seconds

// GLOBALS
char number[LENGTH+1];
int currentDigit;
int pulseCount;

// State and initial state
typedef enum { ON_HOOK, OFF_HOOK, DIALLING, CONNECTED } stateType;
stateType state = ON_HOOK;

int previousPinReading = HIGH;
unsigned long timePinChanged;
unsigned long now = millis();

void setup() {
	// Setup I/O pins
	// Using internal pullup resistors means pins will be LOW when switch is closed
	// Some digitalReads will be inverted (!) to account for this
	pinMode(pulse_pin, INPUT_PULLUP);
	pinMode(reciever_pin, INPUT_PULLUP);
	//pinMode(dial_pin, INPUT_PULLUP);

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

void loop() {

	// reciever_pin is HIGH when reciever is docked
	bool reciever_lifted = !digitalRead(reciever_pin);

	// Update reciever state machine
	if(reciever_lifted && state == ON_HOOK) {
		#ifdef DEBUG
			Serial.println("Reciever lifted");
		#endif

		// Update state
		state = OFF_HOOK;
	}
	else if(!reciever_lifted && state != ON_HOOK) {

		#ifdef DEBUG
			Serial.println("Reciever replaced");
		#endif

		// Update state
		state = ON_HOOK;

		// Reset dialed number
		pulseCount = 0;
		currentDigit = 0;

		for (int i = 0; i < LENGTH; i++) {
			number[i] = "\0";
		}
	}

	if(state == OFF_HOOK || state == DIALLING) {

		now = millis();

		bool pinReading = digitalRead(pulse_pin);

		if(pinReading != previousPinReading) {

			state = DIALLING;

			// Debounce pulse counting
			if(now - timePinChanged < debounceDelay) {
				return;
			}

			// Increase increase pulseCount on state change
			// PulseCount will represent the digit dialed
			if(pinReading == HIGH) { pulseCount++; }

			timePinChanged = now;
			previousPinReading = pinReading;
		}
		else if(now - timePinChanged > timeoutDelay){
			// If reciever has been off hook too long,
			// play message to prompt reset
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

		// Number is not finished dialing
		if (currentDigit < LENGTH) {

			// Mod by 10 becuase 10 pulses = "0"
			pulseCount = pulseCount % 10;

			#ifdef DEBUG
				Serial.print("Digit dialled: ");
				Serial.println(pulseCount);
			#endif

			// Save dialed number to character arrray (string)
			number[currentDigit] = pulseCount | '0';

			currentDigit++;

			number[currentDigit] = 0;
		}

		// Enough numbers have been dialed
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

				// Trigger relay to unlock whatever
				digitalWrite(relay_pin, HIGH);
				delay(100);
				digitalWrite(relay_pin, LOW);

				// Idle until reciever is replaced
				while(!digitalRead(reciever_pin)){ delay(1000); }
			}
			else {
				// Incorrect number dialed
				#ifdef DEBUG
					Serial.println("INCORRECT NUMBER");
					Serial.println("Hang up and dial again");
				#endif

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
