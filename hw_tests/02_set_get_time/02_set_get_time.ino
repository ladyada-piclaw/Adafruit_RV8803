/*!
 * @file 02_set_get_time.ino
 * @brief Hardware test 02: Set and Get Time
 *
 * Tests setting and reading time from the RV8803 RTC.
 */

#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("=== HW Test 02: Set/Get Time ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 2;

  // Test 1: Set time and read back immediately
  Serial.println(F("Test 1: Set 2026-03-04 22:30:45 (Tuesday) ..."));
  DateTime setTime(2026, 3, 4, 22, 30, 45);
  rtc.adjust(setTime);
  delay(100); // Small delay for registers to settle

  DateTime now = rtc.now();
  Serial.print(F("  Read back: "));
  Serial.print(now.year());
  Serial.print(F("-"));
  Serial.print(now.month());
  Serial.print(F("-"));
  Serial.print(now.day());
  Serial.print(F(" "));
  Serial.print(now.hour());
  Serial.print(F(":"));
  Serial.print(now.minute());
  Serial.print(F(":"));
  Serial.println(now.second());

  bool test1Pass = (now.year() == 2026) && (now.month() == 3) &&
                   (now.day() == 4) && (now.hour() == 22) &&
                   (now.minute() == 30) && (now.second() >= 45);
  if (test1Pass) {
    Serial.println(F("  PASS"));
    passed++;
  } else {
    Serial.println(F("  FAIL - values don't match"));
  }

  // Test 2: Wait 2 seconds, verify time advances
  Serial.println(F("Test 2: Wait 2s, verify seconds advance ..."));
  uint8_t sec1 = rtc.now().second();
  delay(2000);
  uint8_t sec2 = rtc.now().second();

  // Handle rollover at 60
  int diff = sec2 - sec1;
  if (diff < 0)
    diff += 60;

  Serial.print(F("  Before: "));
  Serial.print(sec1);
  Serial.print(F("s, After: "));
  Serial.print(sec2);
  Serial.print(F("s, Diff: "));
  Serial.println(diff);

  if (diff >= 1 && diff <= 3) {
    Serial.println(F("  PASS"));
    passed++;
  } else {
    Serial.println(F("  FAIL - seconds didn't advance correctly"));
  }

  Serial.println();
  Serial.print(passed);
  Serial.print(F("/"));
  Serial.print(total);
  Serial.println(F(" tests passed"));
}

void loop() {
  // Nothing to do
}
