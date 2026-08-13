/*
  Becoming Ripples - Presence Group Controller
  Board: Arduino Nano R3 / Uno compatible

  One controller is used for each row of two 190cmBar devices.
  Each controller reads one LD2410C digital OUT pin, shares its local
  3-minute run request through a simple active-low bus, and drives
  RUN and reset request relay channels for its local devices.

  Bus behavior:
  - LOW means at least one controller is in its local 3-minute hold window.
  - HIGH means no controller is reporting a local run request.
  - The bus pin is never driven HIGH. It is either INPUT_PULLUP or OUTPUT LOW.

  Reset behavior:
  - Each local device tracks its own accumulated RUN time since that device
    last received a reset pulse.
  - A device is queued for reset only after its own accumulated RUN time
    reaches runTimeBeforeResetMs.
  - Reset pulses are sent only while the room is IDLE.
  - If presence appears during an active reset pulse, the current 2-second
    reset pulse finishes first. RUN then resumes.
  - Devices already queued for reset are completed first; devices that become
    due during an interruption are queued for the next pass.
*/

const int sensorPin = 2;
const int busPin = 3;
const int deviceCount = 2;
const int runRelayPins[deviceCount] = {5, 7};   // Device A, Device B
const int resetRelayPins[deviceCount] = {6, 8}; // Device A, Device B
const int statusLedPin = 13;

const char ROW_ID = 'A'; // Use 'A' for one row, 'B' for the other row.

const bool SENSOR_ACTIVE_HIGH = true;
const bool RELAY_ACTIVE_LOW = false;

const unsigned long presenceDebounceMs = 500UL;
const unsigned long holdTimeMs = 180000UL;
const unsigned long runTimeBeforeResetMs = 900000UL;
const unsigned long rowAResetIdleDelayMs = 300000UL;
const unsigned long rowBResetIdleDelayMs = 600000UL;
const unsigned long resetPulseMs = 2000UL;
const unsigned long resetBetweenDevicesMs = 600000UL;
const unsigned long loopDelayMs = 20UL;

const unsigned long resetIdleDelayMs = ROW_ID == 'B' ? rowBResetIdleDelayMs : rowAResetIdleDelayMs;

unsigned long lastLocalPresenceTime = 0;
unsigned long localPresenceStartedTime = 0;
unsigned long lastRunAccountingTime = 0;
unsigned long runTimeSinceDeviceReset[deviceCount] = {0, 0};

bool roomRunActive = false;
bool previousRoomRunActive = false;
bool hasSeenLocalPresence = false;
bool rawLocalPresenceWasActive = false;

bool idleResetTimerStarted = false;
unsigned long nextResetAllowedTime = 0;
unsigned long resetPulseStartedTime = 0;
int activeResetDeviceIndex = -1;

bool currentResetDue[deviceCount] = {false, false};
bool nextResetDue[deviceCount] = {false, false};

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

