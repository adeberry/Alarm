// TODO: add general notes.



// You can change these values if you need more or less time to get out or enter your code
const int DelayTimeInSeconds   = 10; // 60;   // This is how long you have to get out of the room after you turn on the alarm
const int DisableTimeInSeconds = 10; // 60;   // This is how long you have to turn off the alarm after you enter the room
const int GiveUpTimeInminutes  = 1;  // 10;   // This is how long the siren will go off before giving up. The LED will continue to flash

// Input pins
const int code1pin = 3;         // numeric keypad pin
const int code2pin = 4;         // numeric keypad pin
const int code3pin = 5;         // numeric keypad pin
const int code4pin = 6;         // numeric keypad pin
const int wrongCodePin = 2;     // numeric keypad pin
const int alarmSensorPin = 11;  // alarm door/window sensor pin

// Output pins
const int ledPin = 10;           // the red (alarm) led pin
const int buzzerPin = 9;         // alarm siren pin
const int alarmOffPin = 13;      // pin to turn off alarm power relay

// Alarm State Constants
// These help us keep track of the state of the alarm
const int OnDelay      = 0;  // The alarm system is turned on, but you have some time to leave the room
const int Armed        = 1;  // The alarm is on
const int SensorOpen   = 2;  // You have one minute to enter your code before the trouble starts
const int Deactivated  = 3;  // You entered the code correctly, so the alarm will now turn itself off
const int AlarmOn      = 4;  // Oh no! The lights are flashing and there is a loud noise
const int AlarmTimeout = 5;  // Ok, that's enough noise, but I'll keep flashing the light so you know someone came in
int AlarmState = 0;          // We will start up the alarm in "OnDelay" state

// Timers
long sensorOpenTime;         // This will hold the time in millis when we a sensor was opened
long alarmStartedTime;       // This is the time we started the alarm sound

// Keypad variables
int deactivateState = 0;        // the current state of the deactivation code (0 = no buttons, 1 = first digit entered, 2 = second, etc.)
int code1State = LOW;           // the current reading from code 1 pin
int code2State = LOW;           // the current reading from code 2 pin
int code3State = LOW;           // the current reading from code 3 pin
int code4State = LOW;           // the current reading from code 4 pin
int wrongCodeState = LOW;       // the current reading from wrong code pin
int lastCode1State = LOW;       // the previous reading from code 1 pin
int lastCode2State = LOW;       // the previous reading from code 2 pin
int lastCode3State = LOW;       // the previous reading from code 3 pin
int lastCode4State = LOW;       // the previous reading from code 4 pin
int lastWrongCodeState = LOW;   // the previous reading from wrong code pin
long lastDebounceTime = 0;      // the last time the code pins were toggled
long debounceDelay = 50;        // the debounce time; increase if the output flickers

void setup() {
  // put your setup code here, to run once:
  pinMode(code1pin, INPUT);
  pinMode(code2pin, INPUT);
  pinMode(code3pin, INPUT);
  pinMode(code4pin, INPUT);
  pinMode(wrongCodePin, INPUT);
  pinMode(alarmSensorPin, INPUT);

  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(alarmOffPin, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  switch (AlarmState) {
    case OnDelay:
      // So... just wait around until they leave the room
      analogWrite(ledPin, HIGH);
      for (int i = 0; i < DelayTimeInSeconds; i++) {
        delay(2);
      }
      analogWrite(ledPin, LOW);
      AlarmState = Armed;
      break;
    case Armed:
      // Keep reading the switch pin until someone opens a door or window
      if (digitalRead(alarmSensorPin) == LOW) {
        // Oh no! There was a breech!
        AlarmState = SensorOpen;
        sensorOpenTime = millis();
      }
      break;
    case SensorOpen:
      // Turn on the Alarm LED
      analogWrite(ledPin, HIGH);

      // Make sure we haven't run out of time to enter the code
      if ( (sensorOpenTime + (DisableTimeInSeconds * 1000)) < millis() ) {
        AlarmState = AlarmOn;
        alarmStartedTime = millis();
        break;
      }

      // Check for the unlock code
      CheckForCode();
      break;
    case Deactivated:
      // For now, just turn the alarm system off
      // Maybe we should also play a happy tone in the future?
      digitalWrite(alarmOffPin, HIGH);
      break;
    case AlarmOn:
      // Flash the lights and the buzzer

      // Check to see if we need to give up on the siren
      if ( (alarmStartedTime + (GiveUpTimeInminutes * 60000)) < millis() ) {
        AlarmState = AlarmTimeout;
        break;
      }

      // fade in LED/Siren for 1/2 second
      for (int fadeValue = 0 ; fadeValue <= 250; fadeValue += 5) {
        analogWrite(ledPin, fadeValue);
        analogWrite(buzzerPin, fadeValue);
        // wait for 2 milliseconds to see the dimming effect
        delay(2);
      }
      // fade out LED/Siren for 1/2 second
      for (int fadeValue = 250 ; fadeValue >= 0; fadeValue -= 5) {
        // sets the value (range from 0 to 255):
        analogWrite(ledPin, fadeValue);
        analogWrite(buzzerPin, fadeValue);
        // wait for 2 milliseconds to see the dimming effect
        delay(2);
      }

      // Check for the unlock code
      CheckForCode();

      // Alternate siren code...
      //      //Flash LED and buzzer on and off.
      //      for (i = 1; i <= 10; i++)
      //      {
      //
      //        analogWrite(ledPin, 255);
      //        analogWrite(buzzerPin, 200);
      //        delay(100);
      //        analogWrite(ledPin, 0);
      //        analogWrite(buzzerPin, 25);
      //        delay(100);
      //      }
      //
      break;
    case AlarmTimeout:
      // Turn off the buzzer, but keep flashing the light

      // fade in LED for 1/2 second
      for (int fadeValue = 0 ; fadeValue <= 250; fadeValue += 5) {
        analogWrite(ledPin, fadeValue);
        // wait for 2 milliseconds to see the dimming effect
        delay(2);
      }
      // fade out LED for 1/2 second
      for (int fadeValue = 250 ; fadeValue >= 0; fadeValue -= 5) {
        // sets the value (range from 0 to 255):
        analogWrite(ledPin, fadeValue);
        // wait for 2 milliseconds to see the dimming effect
        delay(2);
      }

      // Check for the unlock code
      CheckForCode();
      break;
  }
}

void CheckForCode() {
  // Read the keypad code...
  int code1reading = digitalRead(code1pin);
  int code2reading = digitalRead(code2pin);
  int code3reading = digitalRead(code3pin);
  int code4reading = digitalRead(code4pin);
  int wrongCodeReading = digitalRead(wrongCodePin);

  // check to see if you just pressed the button
  // (i.e. the input changed), and you've waited
  // long enough since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (code1reading != lastCode1State 
   || code2reading != lastCode2State 
   || code3reading != lastCode3State 
   || code4reading != lastCode4State 
   || wrongCodeReading != lastWrongCodeState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
  if (code1reading != code1State 
   || code2reading != code2State 
   || code3reading != code3State 
   || code4reading != code4State 
   || wrongCodeReading != wrongCodeState) {

      if (wrongCodeReading == HIGH) {
        // Wrong button, reset the deactivate state
        deactivateState = 0;
      } else if (code1reading == HIGH) {
        // First digit has been entered
        if (deactivateState == 0) {
          // Go back to 0 if they are pressing the wrong button
          deactivateState = 1;
        } else {
          // Go back to 0 if they are pressing the wrong button
          deactivateState = 0;
        }
      } else if (code2reading == HIGH) {
        // Second digit has been entered
        if (deactivateState == 1) {
          deactivateState = 2;
        } else {
          // Go back to 0 if they are pressing the wrong button
          deactivateState = 0;
        }
      } else if (code3reading == HIGH) {
        // Third digit has been entered
        if (deactivateState == 2) {
          deactivateState = 3;
        } else {
          // Go back to 0 if they are pressing the wrong button
          deactivateState = 0;
        }
      } else if (code4reading == HIGH) {
        // Fourth digit has been entered
        if (deactivateState == 3) {
          // Last digit has been entered. Turn off the alarm
          AlarmState = Deactivated;
        } else {
          // Go back to 0 if they are pressing the wrong button
          deactivateState = 0;
        }
      }

      // Update the current keypadState
      code1State = code1reading;
      code2State = code2reading;
      code3State = code3reading;
      code4State = code4reading;
      wrongCodeState = wrongCodeReading;
    }
  }

  // Save the reading.  Next time through the loop, it'll be the lastKeypadState:
  lastCode1State = code1reading;
  lastCode2State = code2reading;
  lastCode3State = code3reading;
  lastCode4State = code4reading;
  lastWrongCodeState = wrongCodeReading;
}

