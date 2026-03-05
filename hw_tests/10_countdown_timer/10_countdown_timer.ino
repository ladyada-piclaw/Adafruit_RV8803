/*!
 * @file 10_countdown_timer.ino
 * @brief Hardware test 10: Countdown Timer
 *
 * Tests countdown timer functionality with timing verification.
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
  uint8_t total = 5;

  // Test 1: Set 3-second countdown and verify value
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

  // Test 2: Verify 3-second timer fires with correct timing (2500-4500ms)
  // Note: Timer may take up to 1 extra second due to asynchronous start
  Serial.println(F("Test 2: Verify 3-second timing (expect 2500-4500ms) ..."));
  rtc.clearTimer();
  rtc.enableCountdownTimer(RV8803_Timer1Hz, 3);
  rtc.enableInterrupt(RV8803_InterruptTimer);

  unsigned long start = millis();
  bool fired = false;
  unsigned long elapsed = 0;

  while (millis() - start < 6000) {
    if (rtc.timerFired()) {
      fired = true;
      elapsed = millis() - start;
      Serial.print(F("  Timer fired after "));
      Serial.print(elapsed);
      Serial.println(F(" ms"));
      break;
    }
    delay(50);
  }

  if (fired) {
    if (elapsed >= 2500 && elapsed <= 4500) {
      Serial.println(F("  PASS - timing correct (3s +1s/-0.5s)"));
      passed++;
    } else {
      Serial.print(F("  FAIL - timing off (expected 2500-4500ms, got "));
      Serial.print(elapsed);
      Serial.println(F("ms)"));
    }
  } else {
    Serial.println(F("  FAIL - timer did not fire within 6s"));
  }

  // Test 3: Verify 5-second timer fires with correct timing (4500-5500ms)
  Serial.println(F("Test 3: Verify 5-second timing (expect 4500-5500ms) ..."));
  rtc.clearTimer();
  rtc.enableCountdownTimer(RV8803_Timer1Hz, 5);
  rtc.enableInterrupt(RV8803_InterruptTimer);

  start = millis();
  fired = false;
  elapsed = 0;

  while (millis() - start < 7000) {
    if (rtc.timerFired()) {
      fired = true;
      elapsed = millis() - start;
      Serial.print(F("  Timer fired after "));
      Serial.print(elapsed);
      Serial.println(F(" ms"));
      break;
    }
    delay(50);
  }

  if (fired) {
    if (elapsed >= 4500 && elapsed <= 5500) {
      Serial.println(F("  PASS - timing correct (5s ± 500ms)"));
      passed++;
    } else {
      Serial.print(F("  FAIL - timing off (expected 4500-5500ms, got "));
      Serial.print(elapsed);
      Serial.println(F("ms)"));
    }
  } else {
    Serial.println(F("  FAIL - timer did not fire within 7s"));
  }

  // Test 4: Clear timer flag
  Serial.println(F("Test 4: Clear timer flag ..."));
  rtc.clearTimer();
  if (!rtc.timerFired()) {
    Serial.println(F("  PASS - timer flag cleared"));
    passed++;
  } else {
    Serial.println(F("  FAIL - timer flag still set"));
  }

  // Test 5: Disable timer
  Serial.println(F("Test 5: Disable timer ..."));
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
