// This is the simple time template to be used with my publically shared coding projects.
// Written by Adrian Gilmore autoGilmore 090416
// Instructions:
// Copy all the code snippets blocks into your code for the loop function and the functions.
// Do not use any delays or pauses in your code.
// You can use the time boolean values:
//  timeOneFrame
//  timeHalfSecond
//  timeOneSecond
//  timeOneMinute
//  timeOneHour
//  timeOneDay
//  timeOneWeek
// Do not set these time value boolean as each will be reset on the next cycle.
// Example usage:
// Flash an LED once every second being lit for a half second.
// if (timeOneSecond) {
//     useRedLED = true;
// } else if (timeHalfSecond) {
//     useRedLED = false;
// }

/////////////COPY BELOW////////////////////

// *********************SIMPLE TIME TEMPLATE ****************************
// You may set the frame rate per second if needed
const int FRAMES_PER_SECOND = 16;

// Do not override these values in your program.
boolean timeHalfSecond = false;
boolean timeOneSecond = false;
boolean timeOneMinute = false;
boolean timeOneHour = false;
boolean timeOneDay = false;
boolean timeOneWeek = false;
boolean timeOneFrame = false;
// *********************SIMPLE TIME TEMPLATE ****************************

///////////////COPY ABOVE////////////////////////////

// Setup.
//
void setup() {
  Serial.begin(9600);  // Begin serial communication.
  Serial.println("TESTS STARTING: wait one minute to see results");
}

// Loop.
//
void loop() {
  /////////////COPY BELOW////////////////////
  // Place in the loop() to advance the boolean time values (NOTE: don't use delays in your code)
  time_advanceTimeValues();

  ///////////////COPY ABOVE////////////////////////////
  // If you want to test this code, load this entire file on to your board and view the test results from the serial monitor displayed
  // from this function.
  runUnitTests();
}

// I recommend placing this code at the bottom of your code so that it is out of the way.
/////////////COPY BELOW////////////////////


// *********************SIMPLE TIME TEMPLATE ****************************
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
// *********************SIMPLE TIME TEMPLATE ****************************

///////////////COPY ABOVE////////////////////////////

// No need to include this in your code
/////////////////////////////////// UNIT TESTS //////////////////////////////////

void runUnitTests() {
  // Unit tests
  testMinute();
  testFrameRate();

  // Add simulated program load
  if (timeOneSecond) {
    delay(random(0, 100));
  }
}

boolean run_testMinute = true;
byte secondCount = 0;
unsigned long runningMinTest = 0;
unsigned long lastRunningMinTest = 0;
void testMinute() {
  if (run_testMinute) {
    String testName = "testMinute()";
    // Verifiy time elapsed
    runningMinTest = millis();
    if (timeOneMinute) {
      Serial.println(testName);
      unsigned long nowTestTime = runningMinTest - lastRunningMinTest;
      if (lastRunningMinTest != 0) {
        // Verify one minute milliseconds passed
        getTestResultLongHold(60000, nowTestTime, testName, "one minute ms");
        // Test complete
        run_testMinute = 60000 != nowTestTime; // Run again if failing
      }
      else {
        Serial.println("TEST STILL RUNNING: One Minute elapsed time check");
      }
      lastRunningMinTest = runningMinTest;
    }

    // Verify seconds per minute count
    if (timeOneSecond) {
      secondCount++;
    }
    if (timeOneMinute) {
      getTestResultLongHold(60, secondCount, testName, "One Minute seconds check");
      secondCount = 0;
    }
  }
}

boolean run_testFrameRate = true;
void testFrameRate() {
  if (run_testFrameRate) {
    if (timeOneMinute) {
      String testName = "testFrameRate()";
      Serial.println(testName);
      // Verify frame rate check
      getTestResultLongHold(FRAMES_PER_SECOND, timePrivate_frameCountTest, testName, "frame rate check");
      // Test completed
      run_testFrameRate = FRAMES_PER_SECOND != timePrivate_frameCountTest; // Run again if failing
    }
  }
}

// Helpers to print the test results
void getTestResultLongHold(long expectedValue, long resultValue, String testName, String subTestName) {
  Serial.println("---------------");
  delay(10);
  String message = "";
  if (resultValue == expectedValue) {
    message = "TEST PASS: ";
    message += testName;
    message += ": (";
    message += subTestName;
    message += ") test matched";
  }
  else {
    message = "----TEST FAIL: ";
    message += testName;
    message += ": (";
    message += subTestName;
    message += ") Result: ";
    message += resultValue;
  }
  Serial.println(message);
  delay(10);
}

void getTestResultBooleanHold(boolean expectedValue, boolean resultValue, String testName, String subTestName) {
  Serial.println("---------------");
  String message = "";
  if (resultValue == expectedValue) {
    message = "TEST PASS: ";
    message += testName;
    message += ": (";
    message += subTestName;
    message += ") test matched";
  }
  else {
    message = "----TEST FAIL: ";
    message += testName;
    message += ": (";
    message += subTestName;
    message += ") Result: ";
    if (resultValue) {
      message += "False";
    }
    else {
      message += "False";
    }
  }
  Serial.println(message);
}
