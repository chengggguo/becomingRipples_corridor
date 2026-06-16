/*
  Becoming Ripples - Presence Group Controller
  Board: Arduino Nano R3 / Uno compatible

  One controller is used for each row of three 190cmBar devices.
  Each controller reads one LD2410C digital OUT pin, shares its local
  3-minute run request through a simple active-low bus, and drives
  RUN and reset request relay channels for its local devices.

  Bus behavior:
  - LOW means at least one controller is in its local 3-minute hold window.
  - HIGH means no controller is reporting a local run request.
  - The bus pin is never driven HIGH. It is either INPUT_PULLUP or OUTPUT LOW.
*/

const int sensorPin = 2;
const int busPin = 3;
const int deviceCount = 3;
const int runRelayPins[deviceCount] = {5, 6, 7};
const int resetRelayPins[deviceCount] = {8, 9, 10};
const int statusLedPin = 13;

const bool SENSOR_ACTIVE_HIGH = true;
const bool RELAY_ACTIVE_LOW = true;

const unsigned long holdTimeMs = 180000UL;
const unsigned long resetIdleDelayMs = 300000UL;
const unsigned long resetPulseMs = 2000UL;
const unsigned long resetBetweenDevicesMs = 60000UL;
const unsigned long resetStartOffsetMs = 0UL;
const unsigned long loopDelayMs = 20UL;

unsigned long lastLocalPresenceTime = 0;
bool roomRunActive = false;
bool hasSeenLocalPresence = false;

bool idleResetTimerStarted = false;
bool resetSweepComplete = false;
unsigned long nextResetAllowedTime = 0;
unsigned long resetPulseStartedTime = 0;
int nextResetDeviceIndex = 0;
int activeResetDeviceIndex = -1;
int resetPulsesThisIdle = 0;

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

bool timeReached(unsigned long now, unsigned long targetTime) {
  return (long)(now - targetTime) >= 0;
}

void setRunRelays(bool on) {
  for (int i = 0; i < deviceCount; i++) {
    setRelay(runRelayPins[i], on);
  }
}

void setResetRelay(int deviceIndex, bool on) {
  setRelay(resetRelayPins[deviceIndex], on);
}

void setResetRelays(bool on) {
  for (int i = 0; i < deviceCount; i++) {
    setResetRelay(i, on);
  }
}

void cancelResetScheduler() {
  setResetRelays(false);
  activeResetDeviceIndex = -1;
  idleResetTimerStarted = false;
  resetSweepComplete = false;
  resetPulsesThisIdle = 0;
}

void startResetPulse(int deviceIndex, unsigned long now) {
  activeResetDeviceIndex = deviceIndex;
  resetPulseStartedTime = now;
  setResetRelay(activeResetDeviceIndex, true);
}

void updateResetScheduler(bool runActive, unsigned long now) {
  if (runActive) {
    cancelResetScheduler();
    return;
  }

  if (resetSweepComplete) {
    return;
  }

  if (!idleResetTimerStarted) {
    idleResetTimerStarted = true;
    nextResetAllowedTime = now + resetIdleDelayMs + resetStartOffsetMs;
  }

  if (activeResetDeviceIndex >= 0) {
    if (now - resetPulseStartedTime >= resetPulseMs) {
      setResetRelay(activeResetDeviceIndex, false);
      activeResetDeviceIndex = -1;
      nextResetDeviceIndex = (nextResetDeviceIndex + 1) % deviceCount;
      resetPulsesThisIdle++;

      if (resetPulsesThisIdle >= deviceCount) {
        resetSweepComplete = true;
      } else {
        nextResetAllowedTime = now + resetBetweenDevicesMs;
      }
    }
    return;
  }

  if (timeReached(now, nextResetAllowedTime)) {
    startResetPulse(nextResetDeviceIndex, now);
  }
}

void setup() {
  pinMode(sensorPin, INPUT);
  releasePresenceBus();

  for (int i = 0; i < deviceCount; i++) {
    setRelay(runRelayPins[i], false);
    setRelay(resetRelayPins[i], false);
    pinMode(runRelayPins[i], OUTPUT);
    pinMode(resetRelayPins[i], OUTPUT);
  }

  pinMode(statusLedPin, OUTPUT);
  digitalWrite(statusLedPin, LOW);
}

void loop() {
  unsigned long now = millis();
  bool localPresence = readLocalPresence();

  if (localPresence) {
    hasSeenLocalPresence = true;
    lastLocalPresenceTime = now;
  }

  bool localRunRequest = hasSeenLocalPresence && now - lastLocalPresenceTime < holdTimeMs;
  bool remoteRunRequest = false;

  if (localRunRequest) {
    assertPresenceBus();
  } else {
    remoteRunRequest = readRemotePresence();
  }

  roomRunActive = localRunRequest || remoteRunRequest;

  setRunRelays(roomRunActive);
  updateResetScheduler(roomRunActive, now);
  digitalWrite(statusLedPin, roomRunActive ? HIGH : LOW);

  delay(loopDelayMs);
}
