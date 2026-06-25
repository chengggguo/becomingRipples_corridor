/*
  Becoming Ripples - per-device runtime reset scheduler test
  Board: Arduino Nano R3 / Uno compatible

  Temporary diagnostic sketch.

  Purpose:
  - D3 bus is disabled.
  - Relay polarity is set for the on-site relay board:
    RELAY_ACTIVE_LOW = false, so HIGH turns a relay ON.
  - Each device tracks its own accumulated RUN time since that device last
    received a reset pulse.
  - Reset is triggered per device after its own accumulated RUN time reaches
    runTimeBeforeResetMs.
  - If a reset sequence is interrupted by presence, the current 2-second reset
    pulse finishes first. RUN then resumes. Devices that were already queued
    for reset are completed first; devices that become due during the
    interruption are queued for the next pass.

  Short test timing:
  - D2 HIGH for 500ms starts RUN.
  - Each device becomes reset-due after 12 seconds of accumulated RUN time.
  - After RUN stops, wait 5 seconds before starting reset.
  - Pulse one reset relay for 2 seconds.
  - Wait 8 seconds before the next reset relay.

  Example edge case:
  1. Devices 1/2/3 run for 12 seconds. All three become due.
  2. Device 1 resets.
  3. Presence returns and all devices run for another 12 seconds.
  4. On the next IDLE window, devices 2 and 3 reset first, because they were
     already due from the earlier pass.
  5. Device 1 then resets again, because it accumulated another 12 seconds
     after its own previous reset.
*/

const int sensorPin = 2;
const int deviceCount = 3;
const int runRelayPins[deviceCount] = {5, 6, 7};
const int resetRelayPins[deviceCount] = {8, 9, 10};
const int statusLedPin = 13;

const bool SENSOR_ACTIVE_HIGH = true;
const bool RELAY_ACTIVE_LOW = false;

const unsigned long presenceDebounceMs = 500UL;
const unsigned long holdTimeMs = 2000UL;
const unsigned long runTimeBeforeResetMs = 12000UL;
const unsigned long resetIdleDelayMs = 5000UL;
const unsigned long resetPulseMs = 2000UL;
const unsigned long resetBetweenDevicesMs = 8000UL;
const unsigned long loopDelayMs = 20UL;

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
  bool finishResetBeforeRun = activeResetDeviceIndex >= 0;
  roomRunActive = localRunRequest && !finishResetBeforeRun;

  accountRunTime(now);
  setRunRelays(roomRunActive);
  updateResetScheduler(roomRunActive, now);
  digitalWrite(statusLedPin, roomRunActive ? HIGH : LOW);

  delay(loopDelayMs);
}
