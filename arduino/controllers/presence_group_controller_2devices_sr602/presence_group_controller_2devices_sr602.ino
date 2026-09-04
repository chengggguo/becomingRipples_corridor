/*
  Becoming Ripples - Presence Group Controller - SR602
  Board / 开发板: Arduino Nano R3 / Uno compatible / 兼容

  ENGLISH
  One controller serves one row of two 190cmBar devices. It reads one
  SR602 PIR OUT pin, shares its local RUN request through an active-low D3 bus,
  and drives RUN and RESET relay contacts for the two local devices. Each RUN
  cycle starts one randomly selected device first, then the other 10s later.

  Bus: LOW means at least one controller requests RUN. HIGH means neither
  controller requests RUN. D3 is never driven HIGH; it is INPUT_PULLUP or
  OUTPUT LOW only.

  Reset is NOT triggered merely because no target is detected. Each device
  must first accumulate runTimeBeforeResetMs of RUN time after its own last
  RESET. When both controllers are idle, this controller waits the row-specific
  resetIdleDelayMs, then sends a reset pulse. The next local device waits
  resetBetweenDevicesMs before its reset pulse.

  If presence returns during a reset pulse, that active pulse finishes first,
  then RUN resumes. Devices left in the queue are reset later, while idle.

  中文
  每块控制器负责一列中的两台 190cmBar 装置。它读取一个 SR602 PIR 的 OUT
  引脚，通过低电平有效的 D3 总线与另一列联动，并用继电器触点控制本列两台
  装置的 RUN 与 RESET。每轮 RUN 会随机选择一台先启动，另一台延迟 10 秒启动。

  总线：D3 为低电平表示至少有一块控制器请求 RUN；高电平表示两边都没有
  请求 RUN。程序绝不会主动把 D3 输出为高电平，只会设为 INPUT_PULLUP 或
  OUTPUT LOW。

  RESET 并不是“检测不到人一段时间后就一定发生”。每台设备都必须先在自己
  上次 RESET 后累计运行到 runTimeBeforeResetMs；两台控制器都进入空闲后，
  本控制器再等待本列的 resetIdleDelayMs，才开始发送 RESET 脉冲。下一台
  本列设备会再等待 resetBetweenDevicesMs 后复位。

  若 RESET 脉冲期间重新检测到目标，当前这一次 RESET 会先完成，随后恢复
  RUN；尚未复位的设备会保留在队列中，等再次空闲时继续。
*/

const int sensorPin = 2;
const int busPin = 3;
const int deviceCount = 2;
const int runRelayPins[deviceCount] = {5, 7};   // EN: Device A/B RUN. 中文：设备 A/B 的 RUN 继电器。
const int resetRelayPins[deviceCount] = {6, 8}; // EN: Device A/B RESET. 中文：设备 A/B 的 RESET 继电器。
const int statusLedPin = 13;                    // EN: ON while RUN is active. 中文：RUN 时亮起。

const char ROW_ID = 'A'; // EN: Set A or B for this controller. 中文：本控制器所在列，填 A 或 B。

const bool SENSOR_ACTIVE_HIGH = true; // EN: SR602 OUT is HIGH on motion. 中文：SR602 检测到人体移动时 OUT 为高。
const bool RELAY_ACTIVE_LOW = false;  // EN: HIGH turns relay on. 中文：高电平吸合继电器。

// Timing values are milliseconds. / 以下时间单位均为毫秒。
const unsigned long sensorWarmupMs = 4000UL;             // EN: Ignore local PIR for 4s after boot. 中文：开机后忽略本机 PIR 输出 4 秒。
const unsigned long presenceDebounceMs = 6500UL;         // EN: PIR must stay HIGH for 6.5s. 中文：PIR 必须连续保持高电平 6.5 秒。
const unsigned long holdTimeMs = 90000UL;                // EN: Local RUN stays on 90s after last target. 中文：最后一次检测后本机 RUN 保持 90 秒。
const unsigned long secondDeviceRunDelayMs = 10000UL;    // EN: Start the second device 10s later. 中文：第二台装置延迟 10 秒启动。
const unsigned long runTimeBeforeResetMs = 1800000UL;    // EN: Each device resets after 30 min accumulated RUN. 中文：每台设备累计 RUN 30 分钟后才进入复位队列。
const unsigned long rowAResetIdleDelayMs = 300000UL;     // EN: Row A waits 5 min after both sides are idle. 中文：两边都空闲后，A 列等待 5 分钟。
const unsigned long rowBResetIdleDelayMs = 600000UL;     // EN: Row B waits 10 min after both sides are idle. 中文：两边都空闲后，B 列等待 10 分钟。
const unsigned long resetPulseMs = 2000UL;               // EN: RESET contact closes for 2s. 中文：RESET 继电器吸合 2 秒。
const unsigned long resetBetweenDevicesMs = 600000UL;    // EN: Wait 10 min between local devices. 中文：本列两台设备的 RESET 相隔 10 分钟。
const unsigned long loopDelayMs = 20UL;                  // EN: Main loop interval. 中文：主循环间隔。


// EN: Uses A or B delay above. 中文：根据 ROW_ID 自动采用 A 或 B 列的空闲等待时间。
const unsigned long resetIdleDelayMs = ROW_ID == 'B' ? rowBResetIdleDelayMs : rowAResetIdleDelayMs;

unsigned long lastLocalPresenceTime = 0;
unsigned long localPresenceStartedTime = 0;
unsigned long lastDeviceRunAccountingTime[deviceCount] = {0, 0};
unsigned long runTimeSinceDeviceReset[deviceCount] = {0, 0};

bool roomRunActive = false;
bool deviceRunActive[deviceCount] = {false, false};
bool previousDeviceRunActive[deviceCount] = {false, false};
bool runSequenceActive = false;
unsigned long runSequenceStartedTime = 0;
int firstRunDeviceIndex = -1;
int secondRunDeviceIndex = -1;
bool hasSeenLocalPresence = false;
bool rawLocalPresenceWasActive = false;
bool sensorWarmupComplete = false;
bool printedPresenceState = false;
bool previousPrintedPresenceState = false;
bool localSensorStateInitialized = false;
bool previousLocalSensorPresence = false;

bool idleResetTimerStarted = false;
unsigned long nextResetAllowedTime = 0;
unsigned long resetPulseStartedTime = 0;
int activeResetDeviceIndex = -1;

bool currentResetDue[deviceCount] = {false, false};
bool nextResetDue[deviceCount] = {false, false};

void printDeviceName(int deviceIndex) {
  Serial.print(F("设备"));
  Serial.print((char)('A' + deviceIndex));
}

void printDueList(const bool dueList[]) {
  bool printedOne = false;

  for (int i = 0; i < deviceCount; i++) {
    if (!dueList[i]) {
      continue;
    }

    if (printedOne) {
      Serial.print(F("、"));
    }
    printDeviceName(i);
    printedOne = true;
  }

  if (!printedOne) {
    Serial.print(F("无"));
  }
}

int updateLocalSensorState(bool sensorReady, bool localPresence) {
  if (!sensorReady) {
    return 0;
  }

  if (!localSensorStateInitialized) {
    previousLocalSensorPresence = localPresence;
    localSensorStateInitialized = true;
    return 0;
  }

  if (localPresence == previousLocalSensorPresence) {
    return 0;
  }

  previousLocalSensorPresence = localPresence;
  return localPresence ? 1 : -1;
}

void printPresenceState(unsigned long now, bool sensorReady,
                        bool presenceActive, bool localRunRequest,
                        bool remoteRunRequest,
                        bool remoteRunRequestWasRead,
                        int localSensorTransition,
                        int runSequenceEvent) {
  if (!sensorReady && !presenceActive) {
    return;
  }

  bool roomStateChanged = !printedPresenceState ||
                          presenceActive != previousPrintedPresenceState;
  if (localSensorTransition == 0 && runSequenceEvent == 0 &&
      !roomStateChanged) {
    return;
  }

  Serial.println();
  Serial.println(F("================================"));
  if (localSensorTransition > 0) {
    Serial.println(F(">>> 本机传感器：检测到人体移动 <<<"));
  } else if (localSensorTransition < 0) {
    Serial.println(F(">>> 本机传感器输出变为 LOW（本次运动脉冲结束） <<<"));
  } else if (runSequenceEvent == 2) {
    Serial.println(F(">>> 第二台装置等待结束，RUN 已启动 <<<"));
  } else if (presenceActive) {
    Serial.println(F(">>> 检测到有人，RUN 已开启 <<<"));
  } else if (printedPresenceState) {
    Serial.println(F(">>> 连续 90 秒没有有效移动：确认无人，RUN 已关闭 <<<"));
  } else {
    Serial.println(F(">>> 初始化完成：当前没有有效移动，RUN 已关闭 <<<"));
  }
  Serial.println(F("================================"));

  if (localSensorTransition > 0) {
    Serial.print(F("RUN 保持计时已开始或重新计时："));
    Serial.print(holdTimeMs / 1000UL);
    Serial.println(F(" 秒"));
  } else if (localSensorTransition < 0) {
    Serial.print(F("本次 D2 HIGH 实际持续："));
    Serial.print(now - localPresenceStartedTime);
    Serial.println(F(" 毫秒"));
    Serial.print(F("RUN 保持中；若无新的有效移动，将在 "));
    Serial.print(holdTimeMs / 1000UL);
    Serial.println(F(" 秒后关闭"));
  }

  if (runSequenceEvent == 1) {
    Serial.print(F("随机启动顺序："));
    printDeviceName(firstRunDeviceIndex);
    Serial.print(F(" 先启动；"));
    printDeviceName(secondRunDeviceIndex);
    Serial.print(F(" 将在 "));
    Serial.print(secondDeviceRunDelayMs / 1000UL);
    Serial.println(F(" 秒后启动"));
  } else if (runSequenceEvent == 2) {
    printDeviceName(secondRunDeviceIndex);
    Serial.println(F(" 的 RUN 继电器现已开启"));
  }

  Serial.print(F("系统时间："));
  Serial.print(now / 1000UL);
  Serial.println(F(" 秒"));

  Serial.print(F("本机 RUN 请求："));
  Serial.println(localRunRequest ? F("开启") : F("关闭"));

  Serial.print(F("另一组 RUN 请求："));
  if (remoteRunRequestWasRead) {
    Serial.println(remoteRunRequest ? F("开启") : F("关闭"));
  } else {
    Serial.println(F("未读取（本机已经请求 RUN）"));
  }

  Serial.print(F("RUN 继电器："));
  for (int i = 0; i < deviceCount; i++) {
    if (i > 0) {
      Serial.print(F("；"));
    }
    printDeviceName(i);
    Serial.print(deviceRunActive[i] ? F(" 开启") : F(" 关闭"));
  }
  Serial.println();

  Serial.print(F("当前 RESET："));
  if (activeResetDeviceIndex >= 0) {
    printDeviceName(activeResetDeviceIndex);
    Serial.println(F(" 正在复位"));
  } else {
    Serial.println(F("无"));
  }

  Serial.print(F("设备累计运行："));
  for (int i = 0; i < deviceCount; i++) {
    if (i > 0) {
      Serial.print(F("；"));
    }
    printDeviceName(i);
    Serial.print(F(" "));
    Serial.print(runTimeSinceDeviceReset[i] / 1000UL);
    Serial.print(F(" 秒"));
  }
  Serial.println();

  Serial.print(F("本轮待复位："));
  printDueList(currentResetDue);
  Serial.println();

  Serial.print(F("下一轮待复位："));
  printDueList(nextResetDue);
  Serial.println();
  Serial.println(F("================================"));

  previousPrintedPresenceState = presenceActive;
  printedPresenceState = true;
}
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

bool anyRunRelayActive() {
  for (int i = 0; i < deviceCount; i++) {
    if (deviceRunActive[i]) {
      return true;
    }
  }
  return false;
}

void stopRunSequence() {
  for (int i = 0; i < deviceCount; i++) {
    deviceRunActive[i] = false;
  }
  runSequenceActive = false;
  firstRunDeviceIndex = -1;
  secondRunDeviceIndex = -1;
}

int updateRunSequence(bool allowRun, unsigned long now) {
  if (!allowRun) {
    stopRunSequence();
    return 0;
  }

  if (!runSequenceActive) {
    firstRunDeviceIndex = (int)random(deviceCount);
    secondRunDeviceIndex = (firstRunDeviceIndex + 1) % deviceCount;
    runSequenceStartedTime = now;
    runSequenceActive = true;

    for (int i = 0; i < deviceCount; i++) {
      deviceRunActive[i] = false;
    }
    deviceRunActive[firstRunDeviceIndex] = true;
    return 1;
  }

  if (!deviceRunActive[secondRunDeviceIndex] &&
      now - runSequenceStartedTime >= secondDeviceRunDelayMs) {
    deviceRunActive[secondRunDeviceIndex] = true;
    return 2;
  }

  return 0;
}

void applyRunRelays() {
  for (int i = 0; i < deviceCount; i++) {
    setRelay(runRelayPins[i], deviceRunActive[i]);
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
  bool accumulatedRunTime = false;

  for (int i = 0; i < deviceCount; i++) {
    if (previousDeviceRunActive[i]) {
      runTimeSinceDeviceReset[i] +=
          now - lastDeviceRunAccountingTime[i];
      accumulatedRunTime = true;
    }

    if (deviceRunActive[i]) {
      lastDeviceRunAccountingTime[i] = now;
    }
    previousDeviceRunActive[i] = deviceRunActive[i];
  }

  if (accumulatedRunTime) {
    queueDueDevices();
  }
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
  Serial.begin(115200);

  pinMode(sensorPin, INPUT);
  releasePresenceBus();
  randomSeed(((unsigned long)analogRead(A0) << 16) ^ micros());

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

  if (!sensorWarmupComplete && now >= sensorWarmupMs) {
    sensorWarmupComplete = true;
  }

  bool localPresence = false;
  if (sensorWarmupComplete) {
    localPresence = readDebouncedLocalPresence(now);
  } else {
    rawLocalPresenceWasActive = false;
  }

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
  int runSequenceEvent =
      updateRunSequence(requestedRunActive && !finishResetBeforeRun, now);
  roomRunActive = anyRunRelayActive();

  int localSensorTransition = updateLocalSensorState(sensorWarmupComplete,
                                                     localPresence);
  printPresenceState(now, sensorWarmupComplete, requestedRunActive,
                     localRunRequest, remoteRunRequest, !localRunRequest,
                     localSensorTransition, runSequenceEvent);

  accountRunTime(now);
  applyRunRelays();
  updateResetScheduler(roomRunActive, now);
  digitalWrite(statusLedPin, roomRunActive ? HIGH : LOW);

  delay(loopDelayMs);
}
