/*
  Becoming Ripples - Presence Group Controller no-bus relay test
  Board: Arduino Nano R3 / Uno compatible

  Temporary diagnostic sketch.

  Purpose:
  - Disable the D3 cross-controller bus completely.
  - Test one local LD2410 OUT input on D2.
  - Test all six relay channels:
    D5/D6/D7  -> RUN relays for devices 1/2/3
    D8/D9/D10 -> RESET relays for devices 1/2/3

  This variant is tuned for the relay behavior reported on site:
  RELAY_ACTIVE_LOW is set to false, meaning HIGH turns a relay ON.

  Test timeline now matches the production timing, except D3 bus is disabled:
  - Local presence on D2 for 500ms: D13 ON, RUN relays 1/2/3 ON.
  - No presence for holdTimeMs: D13 OFF, RUN relays 1/2/3 OFF.
  - After resetIdleDelayMs in IDLE: pulse relay 4 for resetPulseMs.
  - Then pulse relay 5, then relay 6, separated by resetBetweenDevicesMs.

  If presence appears during an active reset pulse, this version finishes the
  current reset pulse first, then switches to RUN. The device firmware itself
  still decides when it is ready to react to RUN; if it has already entered
  autoHome(), it will complete that blocking reset flow before running.
*/

const int sensorPin = 2;
const int deviceCount = 3;
const int runRelayPins[deviceCount] = {5, 6, 7};
const int resetRelayPins[deviceCount] = {8, 9, 10};
const int statusLedPin = 13;

const bool SENSOR_ACTIVE_HIGH = true;
const bool RELAY_ACTIVE_LOW = false;
const bool FINISH_ACTIVE_RESET_PULSE_BEFORE_RUN = true;

const unsigned long presenceDebounceMs = 500UL;
const unsigned long holdTimeMs = 180000UL;
const unsigned long resetIdleDelayMs = 300000UL;
const unsigned long resetPulseMs = 2000UL;
const unsigned long resetBetweenDevicesMs = 600000UL;
// Row A: 0UL. Row B: 300000UL to start reset 5 minutes later than Row A.
const unsigned long resetStartOffsetMs = 0UL;
const unsigned long loopDelayMs = 20UL;

unsigned long lastLocalPresenceTime = 0;
unsigned long localPresenceStartedTime = 0;
bool roomRunActive = false;
bool hasSeenLocalPresence = false;
bool rawLocalPresenceWasActive = false;

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

bool readDebouncedLocalPresence(unsigned long now) {
  bool rawLocalPresence = readLocalPresence();

  if (!rawLocalPresence) {
    rawLocalPresenceWasActive = false;
    return false;
  }

  if (!rawLocalPresenceWasActive) {
    rawLocalPresenceWasActive = true;
    localPresenceStartedTime = now;
  }

  return now - localPresenceStartedTime >= presenceDebounceMs;
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

  for (int i = 0; i < deviceCount; i++) {
    pinMode(runRelayPins[i], OUTPUT);
    pinMode(resetRelayPins[i], OUTPUT);
    setRelay(runRelayPins[i], false);
    setRelay(resetRelayPins[i], false);
  }

  pinMode(statusLedPin, OUTPUT);
  digitalWrite(statusLedPin, LOW);
}

void loop() {
  unsigned long now = millis();
  bool localPresence = readDebouncedLocalPresence(now);

  if (localPresence) {
    hasSeenLocalPresence = true;
    lastLocalPresenceTime = now;
  }

  bool localRunRequest = hasSeenLocalPresence && now - lastLocalPresenceTime < holdTimeMs;
  bool finishResetBeforeRun = FINISH_ACTIVE_RESET_PULSE_BEFORE_RUN && activeResetDeviceIndex >= 0;
  roomRunActive = localRunRequest && !finishResetBeforeRun;

  setRunRelays(roomRunActive);
  updateResetScheduler(roomRunActive, now);
  digitalWrite(statusLedPin, roomRunActive ? HIGH : LOW);

  delay(loopDelayMs);
}
