/*
  Becoming Ripples - Presence Group Controller
  Board: Arduino Nano R3 / Uno compatible

  One controller is used for each row of three 190cmBar devices.
  Each controller reads one LD2410C digital OUT pin, shares its local
  3-minute run request through a simple active-low bus, and drives
  three relay channels for its local devices.

  Bus behavior:
  - LOW means at least one controller is in its local 3-minute hold window.
  - HIGH means no controller is reporting a local run request.
  - The bus pin is never driven HIGH. It is either INPUT_PULLUP or OUTPUT LOW.
*/

const int sensorPin = 2;
const int busPin = 3;
const int relayPins[3] = {5, 6, 7};
const int statusLedPin = 13;

const bool SENSOR_ACTIVE_HIGH = true;
const bool RELAY_ACTIVE_LOW = true;

const unsigned long holdTimeMs = 180000UL;
const unsigned long loopDelayMs = 20UL;

unsigned long lastLocalPresenceTime = 0;
bool roomRunActive = false;
bool hasSeenLocalPresence = false;

bool readLocalPresence() {
  int sensorValue = digitalRead(sensorPin);
  return SENSOR_ACTIVE_HIGH ? sensorValue == HIGH : sensorValue == LOW;
}

void releasePresenceBus() {
  pinMode(busPin, INPUT_PULLUP);
}

void assertPresenceBus() {
  digitalWrite(busPin, LOW);
  pinMode(busPin, OUTPUT);
}

bool readRemotePresence() {
  releasePresenceBus();
  delayMicroseconds(50);
  return digitalRead(busPin) == LOW;
}

void setRelay(int pin, bool on) {
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(pin, on ? LOW : HIGH);
  } else {
    digitalWrite(pin, on ? HIGH : LOW);
  }
}

void setAllRelays(bool on) {
  for (int i = 0; i < 3; i++) {
    setRelay(relayPins[i], on);
  }
}

void setup() {
  pinMode(sensorPin, INPUT);
  releasePresenceBus();

  for (int i = 0; i < 3; i++) {
    pinMode(relayPins[i], OUTPUT);
    setRelay(relayPins[i], false);
  }

  pinMode(statusLedPin, OUTPUT);
  digitalWrite(statusLedPin, LOW);
}

void loop() {
  bool localPresence = readLocalPresence();

  if (localPresence) {
    hasSeenLocalPresence = true;
    lastLocalPresenceTime = millis();
  }

  bool localRunRequest = hasSeenLocalPresence && millis() - lastLocalPresenceTime < holdTimeMs;
  bool remoteRunRequest = false;

  if (localRunRequest) {
    assertPresenceBus();
  } else {
    remoteRunRequest = readRemotePresence();
  }

  roomRunActive = localRunRequest || remoteRunRequest;

  setAllRelays(roomRunActive);
  digitalWrite(statusLedPin, roomRunActive ? HIGH : LOW);

  delay(loopDelayMs);
}
