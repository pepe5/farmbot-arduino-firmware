// Do not remove the include below
#include "farmbot_arduino_controller.h"
#include "pins.h"
#include "Config.h"
#include "StepperControl.h"
#include "ServoControl.h"
#include "PinGuard.h"
#include "TimerOne.h"

static char commandEndChar = 0x0A;
static GCodeProcessor* gCodeProcessor = new GCodeProcessor();

unsigned long lastAction;
unsigned long currentTime;

// Blink led routine used for testing
bool blink = false;
void blinkLed() {
	blink = !blink;
	digitalWrite(LED_PIN,blink);
}

// Interrupt handling for:
//   - movement
//   - encoders
//   - pin guard
//
bool interruptBusy = false;
int interruptSecondTimer = 0;
void interrupt(void) {
	interruptSecondTimer++;

        if (interruptBusy == false) {

                interruptBusy = true;
                StepperControl::getInstance()->handleMovementInterrupt();

                // Check the actions triggered once per second
		if (interruptSecondTimer >= 1000000 / MOVEMENT_INTERRUPT_SPEED) {
			interruptSecondTimer = 0;
			PinGuard::getInstance()->checkPins();
			//blinkLed();
		}

                interruptBusy = false;
        }
}


//The setup function is called once at startup of the sketch
void setup() {

	// Setup pin input/output settings
	pinMode(X_STEP_PIN  , OUTPUT);
	pinMode(X_DIR_PIN   , OUTPUT);
	pinMode(X_ENABLE_PIN, OUTPUT);
	pinMode(X_MIN_PIN   , INPUT );
	pinMode(X_MAX_PIN   , INPUT );

	pinMode(Y_STEP_PIN  , OUTPUT);
	pinMode(Y_DIR_PIN   , OUTPUT);
	pinMode(Y_ENABLE_PIN, OUTPUT);
	pinMode(Y_MIN_PIN   , INPUT );
	pinMode(Y_MAX_PIN   , INPUT );

	pinMode(Z_STEP_PIN  , OUTPUT);
	pinMode(Z_DIR_PIN   , OUTPUT);
	pinMode(Z_ENABLE_PIN, OUTPUT);
	pinMode(Z_MIN_PIN   , INPUT );
	pinMode(Z_MAX_PIN   , INPUT );

	pinMode(HEATER_0_PIN, OUTPUT);
	pinMode(HEATER_1_PIN, OUTPUT);
	pinMode(FAN_PIN     , OUTPUT);
	pinMode(LED_PIN     , OUTPUT);

	//pinMode(SERVO_0_PIN , OUTPUT);
	//pinMode(SERVO_1_PIN , OUTPUT);

	digitalWrite(X_ENABLE_PIN, HIGH);
	digitalWrite(Y_ENABLE_PIN, HIGH);
	digitalWrite(Z_ENABLE_PIN, HIGH);

	Serial.begin(115200);

	// Start the motor handling
	ServoControl::getInstance()->attach();

	// Dump all values to the serial interface
	ParameterList::getInstance()->readAllValues();

	// Get the settings for the pin guard
	PinGuard::getInstance()->loadConfig();

	// Start the interrupt used for moviing
	// Interrupt management code library written by Paul Stoffregen
	// The default time 100 micro seconds
        Timer1.attachInterrupt(interrupt);
        Timer1.initialize(MOVEMENT_INTERRUPT_SPEED);
	Timer1.start();

	// Initialize the inactivity check
	lastAction = millis();
}

// The loop function is called in an endless loop
void loop() {

	if (Serial.available()) {

		String commandString = Serial.readStringUntil(commandEndChar);
		if (commandString && commandString.length() > 0) {

			lastAction = millis();

			Command* command = new Command(commandString);
			if(LOGGING) {
				command->print();
			}
			gCodeProcessor->execute(command);
			free(command);
		}
	}
	delay(10);

	// Test
	/**///StepperControl::getInstance()->test();

	currentTime = millis();
	if (currentTime < lastAction) {
		// If the device timer overruns, reset the idle counter
		lastAction = millis();
	}
	else {

		if ((currentTime - lastAction) > 5000) {
			// After an idle time, send the idle message
			Serial.print("R00\r\n");
			CurrentState::getInstance()->printPosition();
			CurrentState::getInstance()->printEndStops();
			lastAction = millis();
		}
	}
}

