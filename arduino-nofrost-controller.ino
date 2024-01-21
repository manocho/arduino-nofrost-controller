/*
No-Frost Controller for Arduino v1.2
It was made and tested using an Arduino UNO

This is the available GPIOs for a custom PCB I made, you can use any GPIO you want!:
D0:  TX Serial
D1:  RX Serial
D11: Relay Trigger
D12: Temperature Sensor (ONEWIRE)
A1: Toggle Mode Button
A2: No-Frost Indicator Led
A3: Not Used
A4: Not Used
*/

#include <ezButton.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pinout & Libraries definitions
const int relayPin = 11;  // Relay PIN
const int sensorPin = 12; // Button PIN
const int buttonPin = A1; // Sensor PIN
const int ledPin = A2; // Led PIN

ezButton button(buttonPin);       // Button PIN
OneWire ourWire(sensorPin);       // Temperature sensor Dallas DS18B20
DallasTemperature sensors(&ourWire); // Dallas sensor definition

// Triggers & Counters contants
const unsigned long normalDuration = 21600000; // Normal operation time: 6 hours (21.600.000 ms)
const unsigned long nofrostDuration = 1500000; // No-Frost operation time: 25 minutes (1.500.000 ms)
const unsigned long serialUpdateInterval = 5000; // Serial console update interval (5 sec)
const int buttonShortPressTime = 1000; // Elapsed time to trigger button (3 sec)
const int buttonLongPressTime = 3000; // Elapsed time to trigger button (3 sec)
//const unsigned long normalDuration = 30000; // Normal operation time: 30 secs for testing
//const unsigned long nofrostDuration = 10000; // No-Frost operation time: 10 secs for testing


// Triggers & Counters variables
unsigned long currentMillis = 0;              // Internal clock elapsed milliseconds
unsigned long relayPreviousMillis = 0;        // Relay previous lap elapsed milliseconds
unsigned long buttonPreviousMillis = 0;       // Button previous lap elapsed milliseconds
unsigned long serialPreviousMillis = 0;       // Serial console previous lap elapsed milliseconds
int temperatureDevicesCount = 0;
int buttonLastState = LOW;  // the previous state from the input pin
int buttonCurrentState = LOW;  // the current reading from the input pin
bool buttonIsPressing = false; // the button is being pressed
bool buttonIsLongDetected = false; // the button is being pressed for long time 


// Time conversion variables
unsigned long modeElapsedMillis = 0;          // Serial console elapsed millis for conversión to hh:mm:ss
int hours = 0;
int minutes = 0;
int seconds = 0;

// Relay variables
bool isNormalState = true; // The relay is in normal state at start (true)

// ezButton variables
unsigned long buttonPressedTime  = 0;
unsigned long buttonReleasedTime = 0;

//// FUNCTIONS ////

// Convert millis to hh:mm:ss
void millisToTime(unsigned long millisInput, int &hours, int &minutes, int &seconds) {
  seconds = millisInput / 1000;
  minutes = seconds / 60;
  hours = minutes / 60;
  seconds %= 60;
  minutes %= 60;
}
// Force No-Frost mode
void forceNoFrost(){ //
    isNormalState = false;
    relayPreviousMillis = currentMillis;
    digitalWrite(relayPin, LOW); // Relay ON (No-Frost State)
    digitalWrite(ledPin, HIGH); // Led ON (No-Frost Indicator)
    Serial.println("No-Frost Mode started by user");
}

// Force Normal mode
void forceNormal(){ //
    isNormalState = true;
    relayPreviousMillis = currentMillis;
    digitalWrite(relayPin, HIGH); // Relay OFF (Normal State)
    digitalWrite(ledPin, LOW); // Led OFF (Normal State)
    Serial.println("Normal Mode started by user");
}

// Get temperature from sensors (index mode)
void getTemperature(){
  float tempC;
  // Send command to all the sensors for temperature conversion
  sensors.requestTemperatures(); 
  
  // Display temperature from each sensor
  for (int i = 0;  i < temperatureDevicesCount;  i++) {
    Serial.print("Sensor ");
    Serial.print(i+1);
    Serial.print(" : ");
    tempC = sensors.getTempCByIndex(i);
    Serial.print(tempC);
    Serial.println(" °C");
  }
}

//// SETUP ////
void setup() {
  Serial.begin(9600);
  pinMode(relayPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // The relay starts OFF (LOW triggers the relay)
  digitalWrite(ledPin, LOW); // The No-Frost LED starts OFF
  button.setDebounceTime(50); // set debounce time to 50 milliseconds
  sensors.setResolution(9);  // Resolution set to 9 bits (70ms speed)
  sensors.begin();   // Temperature sensors initiated
  
  // locate devices on the bus
  Serial.print("Locating DS18B20 devices...");
  Serial.print("Found ");
  temperatureDevicesCount = sensors.getDeviceCount();
  Serial.print(temperatureDevicesCount, DEC);
  Serial.println(" devices.");
  Serial.println("");

}

//// MAIN LOOP FUNCTION ////
void loop() {
  button.loop(); // ezButton MUST call the loop() function first
  currentMillis = millis();

  // Relay operation cycle (Normal & No-Frost) //
  if (isNormalState && (currentMillis - relayPreviousMillis >= normalDuration)) {
    isNormalState = false;
    relayPreviousMillis = currentMillis;
    digitalWrite(relayPin, LOW); // Relay ON (No-Frost State)
    digitalWrite(ledPin, HIGH); // Led ON (No-Frost Indicator)
    Serial.println("No-Frost Mode started");
  } else if (!isNormalState && (currentMillis - relayPreviousMillis >= nofrostDuration)) {
    isNormalState = true;
    relayPreviousMillis = currentMillis;
    digitalWrite(relayPin, HIGH); // Relay OFF (Normal State)
    digitalWrite(ledPin, LOW); // Led OFF
    Serial.println("Normal Mode started");
  }

  // Console output loop //
  if (currentMillis - serialPreviousMillis >= serialUpdateInterval) {
    // Imprime estado    
    modeElapsedMillis = currentMillis - relayPreviousMillis;
    Serial.print("Total milliseconds elapsed: ");
    Serial.print(currentMillis);
    Serial.println(" ms");
    
    millisToTime(modeElapsedMillis, hours, minutes, seconds);
    Serial.print("Lap Time: ");
    Serial.print(hours);
    Serial.print(":");
    Serial.print(minutes);
    Serial.print(":");
    Serial.print(seconds);
    
    if (!isNormalState) { Serial.println(" of No-Frost mode"); } else { Serial.println(" of Normal mode"); }

    getTemperature();
    
    serialPreviousMillis = currentMillis;
  }
 
 // Button Operation //
  buttonCurrentState = button.getState();

  if ( buttonLastState == HIGH && buttonCurrentState == LOW ) { // button is pressed
    buttonPressedTime = millis();
    buttonIsPressing = true;
    buttonIsLongDetected = false;
  } else if ( buttonLastState == LOW && buttonCurrentState == HIGH ) { // button is released
    buttonIsPressing = false;
    buttonReleasedTime = millis();

    long buttonPressDuration = buttonReleasedTime - buttonPressedTime;

    if ( buttonPressDuration < buttonShortPressTime && buttonPressDuration > 0)
      Serial.println("A short press is detected");
  }

  if ( buttonIsPressing == true && buttonIsLongDetected == false ) {
    long buttonPressDuration = millis() - buttonPressedTime;

    if ( buttonPressDuration > buttonLongPressTime ) {
      Serial.println("A long press is detected");
      buttonIsLongDetected = true;
      if (isNormalState) {
        forceNoFrost();
      } else {
        forceNormal();
      }
    }
  }

  buttonLastState = buttonCurrentState; // save the the button last state

}