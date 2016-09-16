
// Scoop time project uses a PIR sensor to track litter box activity.
// When a #1 or #2 has been observed, an indicator LED flashes the number of occurrences.
// Another LED is used to indicate that a #2 has occurred and that it is now time to scoop.
// Two buttons are used for user input to adjust for whether or not scooping was needed.
// Started 080316 by Adrian Gilmore @autoGilmore in honor of our beloved tuxedo Gateway.
// For the Arduino UNO board.

// PINS
const byte RED_LED_PIN = 11;
const byte YELLOW_LED_PIN = 10;
const byte GREEN_LED_PIN = 6;
const byte BLUE_LED_PIN = 7;
const byte SINK_LED_PIN = 9;
const int MOTION_PIN = 2;
const int EARLY_BUTTON_PIN = 4;
const int LATE_BUTTON_PIN = 5;

// User configuration
const int USER_TIME_OUT_SECONDS = 60; // Length of time for ignoring scooping motion.
int maximumNumberTwosAllowed = 1;
int averageMotionSeconds = 30; // Starting point for average time spent in the litter box.
boolean isDebugEnabled = false;

// System motion tracking variables
const int MOTION_MONITOR_TIMEOUT = 90;
int _motionTimeoutTrigger = USER_TIME_OUT_SECONDS;
boolean useGreenLED = false;
boolean useRedLED = false;
boolean useYellowLED = false;
boolean useBlueLED = false;
boolean isTooSoonButtonPressed = false;
boolean isLateButtonPressed = false;
boolean isMotionActive = false;
boolean isWaitState = true;
boolean isMotionState = false;
boolean isCleaningState = false;
boolean isUpdateState = false;
boolean isUserInput = false;
boolean isError = false;
boolean errorFlash = false;
boolean isTooSoon = false;
boolean isLate = false;
boolean isMotionTimeoutTriggered = false;
boolean weAreTesting = false;
boolean motionTimedOut = false;

// *********************SIMPLE TIME****************************
// You may set the frame rate per second if needed
const int FRAMES_PER_SECOND = 32;

// Do not override these values in your program.
boolean timeHalfSecond = false;
boolean timeOneSecond = false;
boolean timeOneMinute = false;
boolean timeOneHour = false;
boolean timeOneDay = false;
boolean timeOneWeek = false;
boolean timeOneFrame = false;
// *********************SIMPLE TIME****************************

void setup() {
  if (isDebugEnabled)  {
    Serial.begin(9600);
    Serial.println(F("Serial started..."));
  }

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(SINK_LED_PIN, OUTPUT);
  digitalWrite(SINK_LED_PIN, HIGH);

  pinMode(MOTION_PIN, INPUT_PULLUP);
  pinMode(EARLY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LATE_BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  time_advanceTimeValues();

  readSensorInput();
  handleUserInput();

  runCurrentState();
  verifyActiveStateCount();
  updateLEDStates();
  updateLEDPins();

  motionTimeoutUpdater();
}

// PIR motion timeout
int motionStateTimeoutCnt = _motionTimeoutTrigger;
boolean isMotionTimeout() {
  if (timeOneSecond && motionStateTimeoutCnt < _motionTimeoutTrigger) {
    motionStateTimeoutCnt++;
  }
  if (isMotionActive) {
    motionStateTimeoutCnt = 0;
  }
  return motionStateTimeoutCnt >= _motionTimeoutTrigger;
}

// User button input timeout
int userInputTimeoutCnt = 0;
boolean isUserInputTimeout() {
  if (timeOneSecond && userInputTimeoutCnt < USER_TIME_OUT_SECONDS) {
    userInputTimeoutCnt++;
  }
  if (isTooSoonButtonPressed || isLateButtonPressed) {
    userInputTimeoutCnt = 0;
    isTooSoon = isTooSoonButtonPressed;
    isLate = isLateButtonPressed;
    if (isTooSoon) isLate = false;
    if (isLate) isTooSoon = false;
  }
  return userInputTimeoutCnt >= USER_TIME_OUT_SECONDS;
}

// Determine the current state
int inMotionStateSecondCnt = 0;
int lastFoundMotionSecondCnt = 0;
boolean isWaitPause = true;
void runCurrentState() {
  if (isWaitState) {
    // Stop an immediate bounce to the motion state
    if (timeOneSecond) isWaitPause = false;

    // Motion detected, go to Motion state
    if (isWaitPause == false && isMotionActive) {
      isMotionState = true;
      isWaitState = !isMotionState;
      isWaitPause = true;
      inMotionStateSecondCnt = 0;
      lastFoundMotionSecondCnt = 0;
    }
  } else if (isMotionState) {
    if (timeOneSecond) inMotionStateSecondCnt++;
    if (isMotionActive) {
      // Store the last active motion second count
      lastFoundMotionSecondCnt = inMotionStateSecondCnt;
    }
    if (isUserInput) {
      // If input is received, roll back this current active count
      lastFoundMotionSecondCnt = 0;
      // Go to the cleaning state
      isCleaningState = true;
      isMotionState = !isCleaningState;
    } else {
      // If no input received, wait for motion stop and timeout
      if (isMotionTimeout()) {
        // Go to the update state
        isUpdateState = true;
        isMotionState = !isUpdateState;
      }
    }
  } else if (isCleaningState) {
    if (isMotionTimeout() && weAreTesting == false) {
      // Determine if scooping was needed or not
      if (isTooSoon) {
        isTooSoon = false;
        if (isTimeToScoop()) {
          // Huh? Guess it was too soon. Next time I should wait longer
          //maximumNumberTwosAllowed++; // Allow for more #2s if you dare
          averageMotionSeconds++;
        } else {
          // Thought so.
        }
      } else if (isLate) {
        isLate = false;
        if (isTimeToScoop()) {
          // Thought so.
        } else {
          // Huh? I didn't detect that... next time I should not wait as long
          //if (maximumNumberTwosAllowed - 1 > 0)  maximumNumberTwosAllowed--; // Allow for fewer #2s
          if (averageMotionSeconds - 1 > 0) averageMotionSeconds--;
        }
      }
      // Reset and return to wait state
      cleanLitterReset();
      isWaitState = true;
      isCleaningState = !isWaitState;
    }
  } else if (isUpdateState) {
    if (isMotionActive) {
      // Go back to motion state
      isMotionState = true;
      isUpdateState = !isMotionState;
    } else if (weAreTesting == false) {
      // Count the litter #1 or #2
      updateNumberOneOrTwo();

      // Go to wait state
      isWaitState = true;
      isUpdateState = !isWaitState;
    }
  } else {
    isError = true;
    Serial.println(F("ERROR: State not set"));
    // Return to wait state
    isWaitState = true;
  }
}

// LED indicators:
// One flash blue = time to scoop indicator before showing other counts
// Flashing red = #2 count since last cleaning
// Flashing yellow = #1 count since last cleaning
// Solid red = button pressed confirming it was time to scoop
// Solid yellow = button pressed showing scooping was not yet needed
// Solid green = PIR motion detected
// Flashing green = Motion stopped but waiting for additional motion before a timeout
// Flashing red continuously = error

// Update the LEDs based on the current state
int showOneCnt = 0;
int showTwoCnt = 0;
int showScoopCnt = 0;
boolean alternateShow = true;
void updateLEDStates() {
  if (isWaitState) {
    useGreenLED = false;

    // Show the current litter box counts
    if (timeHalfSecond) {
      alternateShow = !alternateShow;
      if (alternateShow) {
        if (showScoopCnt > 0) {
          useBlueLED = isTimeToScoop();
          showScoopCnt--;
        } else if (showTwoCnt > 0) {
          showTwoCnt--;
          useRedLED = true;
        } else if (showOneCnt > 0) {
          showOneCnt--;
          useYellowLED = true;
        } else {
          showTwoCnt = getNumberTwoCount();
          showOneCnt = getNumberOneCount();
          showScoopCnt = 1; // Just one flash if scooping is needed
        }
      }
    } else if (timeOneFrame) {
      useRedLED = false;
      useYellowLED = false;
      useBlueLED = false;
    }
  } else if (isMotionState) {
    useBlueLED = false;
    useRedLED = false;
    useYellowLED = false;
    if (isMotionActive) {
      useGreenLED = true;
    } else {
      if (timeOneFrame) useGreenLED = !useGreenLED;
    }
  } else if (isCleaningState) {
    useBlueLED = false;
    useGreenLED = isMotionActive;
    useRedLED = isLate;
    useYellowLED = isTooSoon;
  } else if (isUpdateState) {
    useBlueLED = false;
    useRedLED = false;
    useYellowLED = false;
    if (timeOneSecond) useGreenLED = true;
    if (timeOneFrame)    useGreenLED = false;
  }

  if (isError) {
    errorFlash = !errorFlash;
    if (timeOneSecond) useRedLED = !errorFlash;
  }

  // During any state, if a button is pressed show the corresponding button light
  if (isLateButtonPressed) useRedLED = true;
  if (isTooSoonButtonPressed) useYellowLED = true;
}

// Inputs
void readSensorInput() {
  isTooSoonButtonPressed = digitalRead(EARLY_BUTTON_PIN) == LOW;
  isLateButtonPressed = digitalRead(LATE_BUTTON_PIN) == LOW;
  isMotionActive = digitalRead(MOTION_PIN) == LOW; // NOTE: Sensor is LOW when active
}

// User button input
void handleUserInput() {
  if (isUserInput == false && (isTooSoonButtonPressed || isLateButtonPressed)) {
    isUserInput = true;
  } else if (isUserInput && isUserInputTimeout()) {
    isUserInput = false;
  }
}

// Reset litter counts
int _numberOneCount = 0;
int _numberTwoCount = 0;
void cleanLitterReset() {
  _numberOneCount = 0;
  _numberTwoCount = 0;
}

// Add a #1 or #2 based on the litter motion time
void updateNumberOneOrTwo() {
  // #1 or #2
  if (lastFoundMotionSecondCnt > averageMotionSeconds) {
    addNumberTwoCount();
    averageMotionSeconds++;
  } else {
    addNumberOneCount();
    if (averageMotionSeconds > 1) {
      averageMotionSeconds--;
    }
  }
  lastFoundMotionSecondCnt = 0;
}

void addNumberOneCount() {
  _numberOneCount++;
}
void addNumberTwoCount() {
  _numberTwoCount++;
}

int getNumberOneCount() {
  return _numberOneCount;
}
int getNumberTwoCount() {
  return _numberTwoCount;
}

boolean isTimeToScoop() {
  return getNumberTwoCount() >= maximumNumberTwosAllowed;
}

void updateLEDPins() {
  digitalWrite(GREEN_LED_PIN, !useGreenLED);
  digitalWrite(RED_LED_PIN, !useRedLED);
  digitalWrite(YELLOW_LED_PIN, !useYellowLED);
  digitalWrite(BLUE_LED_PIN, !useBlueLED);
}

void verifyActiveStateCount() {
  int activeStateCnt = 0;
  if (isWaitState)activeStateCnt++;
  if (isMotionState)activeStateCnt++;
  if (isCleaningState)activeStateCnt++;
  if (isUpdateState)activeStateCnt++;
  if (activeStateCnt != 1) {
    isError = true;
  }
}

// Adjust the time that the motion timeout waits
int motionMonitorInactiveCnt = 0;
void motionTimeoutUpdater() {
  if (weAreTesting == false && isMotionActive) {
    weAreTesting = true;
    motionMonitorInactiveCnt = 0;
    motionTimedOut = false;
  } else if (weAreTesting) {
    if (motionTimedOut == false && isMotionTimeout()) {
      motionTimedOut = true;
    } else if (motionTimedOut) {
      if (timeOneSecond) motionMonitorInactiveCnt++;
      if (motionMonitorInactiveCnt < MOTION_MONITOR_TIMEOUT) {
        // Uh-oh, motion was detected
        if (isMotionTimeout() == false) {
          // Increase the timeout
          _motionTimeoutTrigger += 5;
          weAreTesting = false;
        }
      } else {
        // We can shorten it
        if (_motionTimeoutTrigger - 1 > 0) _motionTimeoutTrigger--;
        weAreTesting = false;
      }
    }
  }
}

// *********************SIMPLE TIME****************************
// Private values used to calculate and trigger the boolean time values.
byte timePrivate_halfSeconds = 0;
byte timePrivate_seconds = 0;
byte timePrivate_minutes = 0;
byte timePrivate_hours = 0;
byte timePrivate_days = 1;
unsigned long timePrivate_runningTime;
unsigned long timePrivate_lastHalfSecondRunTime;
long timePrivate_cyclesPerSecond = 0;
long timePrivate_avgCyclesPerSecond = FRAMES_PER_SECOND;
long timePrivate_cycleFrameCount = 0;
int timePrivate_avgCyclesPerFrame = 100;
int timePrivate_frameCountCheck = 0;
int timePrivate_frameCountTest = FRAMES_PER_SECOND;
int timePrivate_halfSecondMsTrigger = 500; // New half second value with an offset from the original 500 ms value.
const int HALF_SECOND_MILLISECONDS = 500;

// Sets boolean time values to represent the time.
//
long timePrivate_nowRunTimeRemainder = 0;
int hsTrigger = HALF_SECOND_MILLISECONDS;
void time_advanceTimeValues() {
  timePrivate_runningTime = millis();
  unsigned long nowRunTime = timePrivate_runningTime - timePrivate_lastHalfSecondRunTime;
  timePrivate_lastHalfSecondRunTime = timePrivate_runningTime;

  // Reset time booleans
  timeOneFrame = false;
  timeHalfSecond = false;
  timeOneSecond = false;
  timeOneMinute = false;
  timeOneHour = false;
  timeOneDay = false;
  timeOneWeek = false;

  // Add back the carry-over remainder for syncing.
  nowRunTime += timePrivate_nowRunTimeRemainder;
  timePrivate_nowRunTimeRemainder = 0; // Reset.

  int addHalfSeconds = 0;
  while (nowRunTime >= hsTrigger) {
    addHalfSeconds++;
    nowRunTime -= hsTrigger;
  }

  // Carry-over the remainder to next time for syncing.
  if (nowRunTime > 0) {
    timePrivate_nowRunTimeRemainder = nowRunTime;
  }

  // Increment and set the time booleans
  if (addHalfSeconds > 0) {
    for (int hs = 0; hs < addHalfSeconds; hs++) {
      timeHalfSecond = true;
      timePrivate_halfSeconds++;
      if (timePrivate_halfSeconds >= 2) {
        timeOneSecond = true;
        timePrivate_halfSeconds = 0;
        timePrivate_seconds++;
        if (timePrivate_seconds >= 60) {
          timeOneMinute = true;
          timePrivate_seconds = 0;
          timePrivate_minutes++;
          if (timePrivate_minutes >= 60) {
            timeOneHour = true;
            timePrivate_minutes = 0;
            timePrivate_hours++;
            if (timePrivate_hours >= 24) {
              timeOneDay = true;
              timePrivate_hours = 0;
              timePrivate_days++;
              if (timePrivate_days >= 8) {
                timeOneWeek = true;
                timePrivate_days = 1;
              }
            }
          }
        }
      }
    }
  }

  // Toggle ms offset.
  if (timeOneMinute) {
    hsTrigger = timePrivate_halfSecondMsTrigger;
  }
  else if (timeOneSecond) {
    hsTrigger = HALF_SECOND_MILLISECONDS;
  }

  // Processing cycle information
  timePrivate_cyclesPerSecond++;
  if (timeOneSecond) {
    timePrivate_avgCyclesPerSecond = ((timePrivate_avgCyclesPerSecond * 29) + timePrivate_cyclesPerSecond) / 30;

    // Update frame rate average.
    timePrivate_avgCyclesPerFrame = timePrivate_avgCyclesPerSecond / FRAMES_PER_SECOND;

    // For unit testing.
    timePrivate_frameCountTest = timePrivate_frameCountCheck;

    // Reset.
    timePrivate_cyclesPerSecond = 0;
    timePrivate_frameCountCheck = 0;
  }

  // Frames per half second. (NOTE: approximate)
  timePrivate_cycleFrameCount++;
  if (timePrivate_cycleFrameCount >= timePrivate_avgCyclesPerFrame) {
    timeOneFrame = true;
    timePrivate_cycleFrameCount = 0;
    timePrivate_frameCountCheck++;
  }
}
// *********************SIMPLE TIME****************************
