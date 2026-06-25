/*
  Becoming Ripples - runtime reset + D3 bus test
  Board: Arduino Nano R3 / Uno compatible

  Temporary diagnostic sketch.

  Purpose:
  - Test the D3 active-low bus between two Presence Group Controllers.
  - Keep the short per-device runtime reset scheduler from the runtime reset
    test sketch.
  - Add a simple A/B row setting at the top so the two rows can use different
    first-reset delays while sharing the same code.

  Wiring between two controller Arduinos:
  - Controller A D3  -> Controller B D3
  - Controller A GND -> Controller B GND

  Bus behavior:
  - If this controller has local RUN request, it pulls D3 LOW.
  - If this controller does not have local RUN request, it releases D3 with
    INPUT_PULLUP and reads whether the other controller is pulling it LOW.
  - Remote bus RUN can start RUN relays, just like local presence.

  Short test timing:
  - D2 HIGH for 500ms starts local RUN.
  - Local or remote bus RUN keeps relays 1/2/3 ON.
  - Each device becomes reset-due after 12 seconds of accumulated RUN time.
  - Row A waits 5 seconds in IDLE before the first reset pulse.
  - Row B waits 8 seconds in IDLE before the first reset pulse.
  - Later reset pulses are separated by 5 seconds.
*/

const char ROW_ID = 'B'; // Use 'A' for one row, 'B' for the other row.

const int sensorPin = 2;
const int busPin = 3;
const int deviceCount = 3;
const int runRelayPins[deviceCount] = {5, 6, 7};
const int resetRelayPins[deviceCount] = {8, 9, 10};
const int statusLedPin = 13;

const bool SENSOR_ACTIVE_HIGH = true;
const bool RELAY_ACTIVE_LOW = false;

const unsigned long presenceDebounceMs = 500UL;
const unsigned long holdTimeMs = 2000UL;
const unsigned long runTimeBeforeResetMs = 12000UL;
const unsigned long rowAResetIdleDelayMs = 5000UL;
const unsigned long rowBResetIdleDelayMs = 8000UL;
const unsigned long resetPulseMs = 2000UL;
const unsigned long resetBetweenDevicesMs = 5000UL;
const unsigned long loopDelayMs = 20UL;

const unsigned long resetIdleDelayMs = ROW_ID == 'B' ? rowBResetIdleDelayMs : rowAResetIdleDelayMs;

unsigned long lastLocalPresenceTime = 0;
unsigned long localPresenceStartedTime = 0;
unsigned long lastRunAccountingTime = 0;
unsigned long runTimeSinceDeviceReset[deviceCount] = {0, 0, 0};

bool roomRunActive = false;
bool previousRoomRunActive = false;
bool hasSeenLocalPresence = false;
bool rawLocalPresenceWasActive = false;

bool idleResetTimerStarted = false;
unsigned long nextResetAllowedTime = 0;
unsigned long resetPulseStartedTime = 0;
int activeResetDeviceIndex = -1;

bool currentResetDue[deviceCount] = {false, false, false};
bool nextResetDue[deviceCount] = {false, false, false};

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

bool anyDue(const bool dueList[]) {
  for (int i = 0; i < deviceCount; i++) {
    if (dueList[i]) {
      return true;
    }
  }
  return false;
}

bool resetCycleActive() {
  return activeResetDeviceIndex >= 0 || anyDue(currentResetDue);
}

int nextDueDeviceIndex() {
  for (int i = 0; i < deviceCount; i++) {
    if (currentResetDue[i]) {
      return i;
    }
  }
  return -1;
}

void promoteNextDueToCurrent() {
  for (int i = 0; i < deviceCount; i++) {
    currentResetDue[i] = nextResetDue[i];
    nextResetDue[i] = false;
  }
}

void queueDueDevices() {
  bool queueForNextPass = resetCycleActive();

  for (int i = 0; i < deviceCount; i++) {
    if (runTimeSinceDeviceReset[i] < runTimeBeforeResetMs) {
      continue;
    }

    if (currentResetDue[i] || nextResetDue[i]) {
      continue;
    }

    if (queueForNextPass) {
      nextResetDue[i] = true;
    } else {
      currentResetDue[i] = true;
    }
  }
}

void pauseResetScheduler() {
  setResetRelays(false);
  idleResetTimerStarted = false;
}

void startResetPulse(int deviceIndex, unsigned long now) {
  activeResetDeviceIndex = deviceIndex;
  resetPulseStartedTime = now;
  setResetRelay(activeResetDeviceIndex, true);
}

void finishResetPulse(unsigned long now) {
  setResetRelay(activeResetDeviceIndex, false);
  runTimeSinceDeviceReset[activeResetDeviceIndex] = 0;
  currentResetDue[activeResetDeviceIndex] = false;
  activeResetDeviceIndex = -1;

  if (anyDue(currentResetDue)) {
    nextResetAllowedTime = now + resetBetweenDevicesMs;
    idleResetTimerStarted = true;
    return;
  }

  if (anyDue(nextResetDue)) {
    promoteNextDueToCurrent();
    nextResetAllowedTime = now + resetBetweenDevicesMs;
    idleResetTimerStarted = true;
    return;
  }

  idleResetTimerStarted = false;
}

void accountRunTime(unsigned long now) {
  if (roomRunActive) {
    if (!previousRoomRunActive) {
      lastRunAccountingTime = now;
    } else {
      unsigned long elapsedMs = now - lastRunAccountingTime;
      for (int i = 0; i < deviceCount; i++) {
        runTimeSinceDeviceReset[i] += elapsedMs;
      }
      lastRunAccountingTime = now;
    }

    queueDueDevices();
  }

  previousRoomRunActive = roomRunActive;
}

void updateResetScheduler(bool runActive, unsigned long now) {
  if (activeResetDeviceIndex >= 0) {
    if (now - resetPulseStartedTime >= resetPulseMs) {
      finishResetPulse(now);
    }
    return;
  }

  if (runActive) {
    pauseResetScheduler();
    return;
  }

  if (!anyDue(currentResetDue)) {
    if (anyDue(nextResetDue)) {
      promoteNextDueToCurrent();
    } else {
      return;
    }
  }

  if (!idleResetTimerStarted) {
    idleResetTimerStarted = true;
    nextResetAllowedTime = now + resetIdleDelayMs;
  }

  if (timeReached(now, nextResetAllowedTime)) {
    int deviceIndex = nextDueDeviceIndex();
    if (deviceIndex >= 0) {
      startResetPulse(deviceIndex, now);
    }
  }
}

void setup() {
  pinMode(sensorPin, INPUT);
  releasePresenceBus();

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
  bool remoteRunRequest = false;

  if (localRunRequest) {
    assertPresenceBus();
  } else {
    remoteRunRequest = readRemotePresence();
  }

  bool requestedRunActive = localRunRequest || remoteRunRequest;
  bool finishResetBeforeRun = activeResetDeviceIndex >= 0;
  roomRunActive = requestedRunActive && !finishResetBeforeRun;

  accountRunTime(now);
  setRunRelays(roomRunActive);
  updateResetScheduler(roomRunActive, now);
  digitalWrite(statusLedPin, roomRunActive ? HIGH : LOW);

  delay(loopDelayMs);
}
