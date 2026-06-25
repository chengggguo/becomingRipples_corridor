/*
  Safe sensor pin test for the Presence Group Controller.

  This sketch checks whether Arduino digital D2 can be read correctly.
  Do not connect 5V to D2 for this test.

  Test steps:
  1. Upload this sketch.
  2. Leave D2 disconnected: the onboard D13 LED should be ON.
  3. Connect D2 to GND: the onboard D13 LED should turn OFF.

  If D2-to-GND makes the board power off, the wire is probably on the
  wrong pin or there is a short on the board/wiring.
*/

const int sensorPin = 2;
const int ledPin = 13;

void setup() {
  pinMode(sensorPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  digitalWrite(ledPin, digitalRead(sensorPin));
}
