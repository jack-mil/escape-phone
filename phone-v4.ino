// Pins 2 and 3 are interrupts on Arduino Nano
#define pulse_pin 2
#define dial_pin 3
#define reciever_pin 4
#define relay_pin 8

const String KEY = "3526305";
volatile int pulse_count = 0;
volatile String code = "";
byte reciever_lifted = LOW;
byte last_rec_state = LOW;

// Delay (in Milliseconds) to wait before interrupts are re-triggered
const unsigned long debounce_delay_dial = 90;
const unsigned long debounce_delay_pulse = 20;

void setup() {
	// Setup I/O pins
	// Using internal pullup resistors means pins will be LOW when switch is closed
	// Some digitalReads will be inverted (!) to account for this
	pinMode(pulse_pin, INPUT_PULLUP);
	pinMode(reciever_pin, INPUT_PULLUP);
	pinMode(dial_pin, INPUT_PULLUP);
	
	// The relay controls the output. It is triggered when relay_pin goes HIGH
	pinMode(relay_pin, OUTPUT);
	digitalWrite(relay_pin, LOW);

	// Attach ISRs to pulse pin and dial pin
	// count_pulse() will be called twice per pulse, (rising and falling edge)
	// Increments count to decode number dialed.
	attachInterrupt(digitalPinToInterrupt(pulse_pin), count_pulse, CHANGE);

	// save_digit() will be called when the dial is finished rotating.
	// Appends dialed number to code, and resets pulse_count, ready for the next number
	attachInterrupt(digitalPinToInterrupt(dial_pin), save_digit, FALLING);

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);

	Serial.begin(9600);
}

void loop() {
	// reciever_pin is HIGH when reciever is docked
	reciever_lifted = !digitalRead(reciever_pin);

	// When the reciever is replaced, reset entered code
	if (reciever_lifted != last_rec_state) {
		if(!reciever_lifted) {
			code = "";
			pulse_count = 0;
			digitalWrite(relay_pin, LOW);
		}
	}
	// Update last reciever state
	last_rec_state = reciever_lifted;

	// Check correct sequence was inputted, and light up the LED
	if(code == KEY) {
		digitalWrite(LED_BUILTIN, HIGH);
		digitalWrite(relay_pin, HIGH);
		Serial.println("UNLOCKED");
		code = "";
		pulse_count = 0;
	}
	else {
		digitalWrite(LED_BUILTIN, LOW);
	}
}
void count_pulse() {
	// Interrupts are debounced with timing sequence
	static unsigned long last_interrupt_time = 0;
	unsigned long interrupt_time = millis();
	if (interrupt_time - last_interrupt_time > debounce_delay_pulse)
	{
		// Only increment count when the reciever is lifted
		if (reciever_lifted) {
			pulse_count++;
			Serial.print("Pulse");
			Serial.println(pulse_count);
		}
	}
	last_interrupt_time = interrupt_time;
}
void save_digit() {
	static unsigned long last_interrupt_time = 0;
	unsigned long interrupt_time = millis();
	if (interrupt_time - last_interrupt_time > debounce_delay_dial);
	{
		// only register a number when the reciever is lifted
		if (reciever_lifted) {
			// count is twice the number (rising and falling)
			// mod count by 10 because 10 pulses is "0"
			if (pulse_count > 0) {
				Serial.println("Finished rotating");
				Serial.println(pulse_count / 2 % 10);
				code += String(pulse_count / 2 % 10);
				pulse_count = 0;
			}
		}
	}
	last_interrupt_time = interrupt_time;
}
