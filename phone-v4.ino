// Library and chip select for SD with SPI
#include <SD.h>
#define SD_chip_select_pin 10

// Library and pin for audio playback
#include <TMRpcm.h>
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


// CONSTANTS
#define LENGTH 7
const char KEY[LENGTH+1] = "3526041";
const unsigned long debounceDelay = 40; //ms
const unsigned long maxPulseInterval = 350; //ms

// GLOBALS
TMRpcm player;

char number[LENGTH+1];
int currentDigit;
int pulseCount;

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

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);

	#ifdef DEBUG
		Serial.begin(9600);
		Serial.println("Connection started");
	#endif

	if (!SD.begin(SD_chip_select_pin)) {
		#ifdef DEBUG
			Serial.println("SD card not detected.");
		#endif
		return;
	} else {
		#ifdef DEBUG
			Serial.println("SD card initialized.");
		#endif
	}

	player.play("dial.wav");
}

void loop() {

	// reciever_pin is HIGH when reciever is docked
	bool reciever_lifted = !digitalRead(reciever_pin);

	// If the reciever is lifted, but wasn't before
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

			if(now - timePinChanged < debounceDelay) {
				return;
			}

			if(pinReading == HIGH) {
				pulseCount++;
				digitalWrite(LED_BUILTIN, HIGH);
			}
			else {
				digitalWrite(LED_BUILTIN, LOW);
			}

			timePinChanged = now;
			previousPinReading = pinReading;
		}
	}

	if(((now - timePinChanged) >= maxPulseInterval) && pulseCount > 0) {

		if (currentDigit < LENGTH) {

			pulseCount = pulseCount % 10;

			#ifdef DEBUG
				Serial.print("Digit dialled: ");
				Serial.println(pulseCount);
			#endif

			number[currentDigit] = pulseCount | '0';

			currentDigit++;

			number[currentDigit] = 0;
		}

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

				digitalWrite(relay_pin, HIGH);
				delay(100);
				digitalWrite(relay_pin, LOW);

				while(!digitalRead(reciever_pin)){ delay(1000); }
			}
			else {
				// Incorrect number dialed
				#ifdef DEBUG
					Serial.println("INCORRECT NUMBER");
					Serial.println("Hang up and dial again");
				#endif
			}
		}
	pulseCount = 0;
	}
}
/* vim: set noexpandtab ts=4 sw=4: */
