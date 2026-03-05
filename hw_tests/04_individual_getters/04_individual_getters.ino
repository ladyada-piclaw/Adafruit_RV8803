/*!
 * @file 04_individual_getters.ino
 * @brief Hardware test 04: Individual Getters
 *
 * Tests that individual getter methods match now() result.
 */

#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("=== HW Test 04: Individual Getters ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 7;

  // Set a known time first
  DateTime setTime(2026, 3, 4, 22, 30, 45);
  rtc.adjust(setTime);
  delay(100);

  // Get values from now() for reference
  DateTime now = rtc.now();

  // Test each getter
  Serial.println(F("Comparing individual getters to now():"));

  // Test: getSeconds()
  uint8_t sec = rtc.getSeconds();
  Serial.print(F("  getSeconds(): "));
  Serial.print(sec);
  Serial.print(F(" vs now(): "));
  Serial.print(now.second());
  // Allow 1 second variance due to timing
  if (sec == now.second() || sec == now.second() + 1 ||
      (now.second() == 59 && sec == 0)) {
    Serial.println(F(" PASS"));
    passed++;
  } else {
    Serial.println(F(" FAIL"));
  }

  // Test: getMinutes()
  uint8_t min = rtc.getMinutes();
  Serial.print(F("  getMinutes(): "));
  Serial.print(min);
  Serial.print(F(" vs now(): "));
  Serial.print(now.minute());
  if (min == now.minute()) {
    Serial.println(F(" PASS"));
    passed++;
  } else {
    Serial.println(F(" FAIL"));
  }

  // Test: getHours()
  uint8_t hr = rtc.getHours();
  Serial.print(F("  getHours(): "));
  Serial.print(hr);
  Serial.print(F(" vs now(): "));
  Serial.print(now.hour());
  if (hr == now.hour()) {
    Serial.println(F(" PASS"));
    passed++;
  } else {
    Serial.println(F(" FAIL"));
  }

  // Test: getDate()
  uint8_t day = rtc.getDate();
  Serial.print(F("  getDate(): "));
  Serial.print(day);
  Serial.print(F(" vs now(): "));
  Serial.print(now.day());
  if (day == now.day()) {
    Serial.println(F(" PASS"));
    passed++;
  } else {
    Serial.println(F(" FAIL"));
  }

  // Test: getMonth()
  uint8_t mon = rtc.getMonth();
  Serial.print(F("  getMonth(): "));
  Serial.print(mon);
  Serial.print(F(" vs now(): "));
  Serial.print(now.month());
  if (mon == now.month()) {
    Serial.println(F(" PASS"));
    passed++;
  } else {
    Serial.println(F(" FAIL"));
  }

  // Test: getYear()
  uint16_t yr = rtc.getYear();
  Serial.print(F("  getYear(): "));
  Serial.print(yr);
  Serial.print(F(" vs now(): "));
  Serial.print(now.year());
  if (yr == now.year()) {
    Serial.println(F(" PASS"));
    passed++;
  } else {
    Serial.println(F(" FAIL"));
  }

  // Test: getWeekday() - Tuesday = 2
  uint8_t wd = rtc.getWeekday();
  Serial.print(F("  getWeekday(): "));
  Serial.print(wd);
  Serial.print(F(" vs now(): "));
  Serial.print(now.dayOfTheWeek());
  if (wd == now.dayOfTheWeek()) {
    Serial.println(F(" PASS"));
    passed++;
  } else {
    Serial.println(F(" FAIL"));
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
