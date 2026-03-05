/*!
 * @file 10_countdown_timer.ino
 * @brief Hardware test 10: Countdown Timer
 *
 * Tests countdown timer functionality.
 */

#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("=== HW Test 10: Countdown Timer ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 4;

  // Test 1: Set 3-second countdown
  Serial.println(F("Test 1: Enable 3-second countdown ..."));
  rtc.clearTimer();
  rtc.enableCountdownTimer(RV8803_Timer1Hz, 3);
  rtc.enableInterrupt(RV8803_InterruptTimer);

  uint16_t timerVal = rtc.getCountdownTimer();
  Serial.print(F("  getCountdownTimer() = "));
  Serial.println(timerVal);

  if (timerVal == 3) {
    Serial.println(F("  PASS - timer value set correctly"));
    passed++;
  } else {
    Serial.println(F("  FAIL - timer value incorrect"));
  }

  // Test 2: Wait for timer to fire
  Serial.println(F("Test 2: Wait for timer (up to 5s) ..."));
  unsigned long start = millis();
  bool fired = false;

  while (millis() - start < 5000) {
    if (rtc.timerFired()) {
      fired = true;
      Serial.print(F("  Timer fired after "));
      Serial.print((millis() - start));
      Serial.println(F(" ms"));
      break;
    }
    delay(100);
  }

  if (fired) {
    Serial.println(F("  PASS - timer fired"));
    passed++;
  } else {
    Serial.println(F("  FAIL - timer did not fire within 5s"));
  }

  // Test 3: Clear timer flag
  Serial.println(F("Test 3: Clear timer flag ..."));
  rtc.clearTimer();
  if (!rtc.timerFired()) {
    Serial.println(F("  PASS - timer flag cleared"));
    passed++;
  } else {
    Serial.println(F("  FAIL - timer flag still set"));
  }

  // Test 4: Disable timer
  Serial.println(F("Test 4: Disable timer ..."));
  rtc.disableCountdownTimer();
  rtc.disableInterrupt(RV8803_InterruptTimer);
  Serial.println(F("  PASS - timer disabled"));
  passed++;

  Serial.println();
  Serial.print(passed);
  Serial.print(F("/"));
  Serial.print(total);
  Serial.println(F(" tests passed"));
}

void loop() {
  // Nothing to do
}
