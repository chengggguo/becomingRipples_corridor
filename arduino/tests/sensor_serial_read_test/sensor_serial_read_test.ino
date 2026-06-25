/*
  Serial read test for LD2410 OUT -> Arduino D2.

  Wiring:
  LD2410 VCC -> Arduino 5V
  LD2410 GND -> Arduino GND
  LD2410 OUT -> Arduino D2

  Open Serial Monitor at 256000 baud.
  It prints the raw digitalRead(D2) value:
  - 1 / HIGH means Arduino sees presence output HIGH.
  - 0 / LOW means Arduino sees presence output LOW.

  D13 behavior:
  - D13 ON means presence detected.
  - D13 OFF means no presence detected.

  Note:
  This sketch reads the LD2410 OUT pin only. It does not read the LD2410
  TX/RX UART data stream. The 256000 baud here is only for the Arduino
  Serial Monitor output, matching the LD2410 documentation for convenience.
*/

const int sensorPin = 2;
const int ledPin = 13;

const bool USE_INTERNAL_PULLUP = false;
const unsigned long printIntervalMs = 250UL;
const unsigned long serialBaud = 256000UL;

unsigned long lastPrintTime = 0;

void setup() {
  if (USE_INTERNAL_PULLUP) {
    pinMode(sensorPin, INPUT_PULLUP);
  } else {
    pinMode(sensorPin, INPUT);
  }

  pinMode(ledPin, OUTPUT);
  Serial.begin(serialBaud);
  Serial.println("LD2410 OUT serial read test started.");
  Serial.println("Serial Monitor baud: 256000");
  Serial.println("Reading Arduino digital D2...");
}

void loop() {
  int sensorValue = digitalRead(sensorPin);
  bool presenceDetected = sensorValue == HIGH;
  digitalWrite(ledPin, presenceDetected ? HIGH : LOW);

  unsigned long now = millis();
  if (now - lastPrintTime >= printIntervalMs) {
    lastPrintTime = now;

    Serial.print("D2=");
    Serial.print(sensorValue);
    Serial.print("  state=");
    Serial.print(sensorValue == HIGH ? "HIGH" : "LOW");
    Serial.print("  presence=");
    Serial.println(presenceDetected ? "YES" : "NO");
  }
}
