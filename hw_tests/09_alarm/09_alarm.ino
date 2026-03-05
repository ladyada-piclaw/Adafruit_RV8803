/*!
 * @file 09_alarm.ino
 * @brief Hardware test 09: Alarm
 *
 * Tests alarm functionality with every-minute mode.
 */

#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("=== HW Test 09: Alarm ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 4;

  // Test 1: Clear alarm flag
  Serial.println(F("Test 1: Clear existing alarm flag ..."));
  rtc.clearAlarm();
  if (!rtc.alarmFired()) {
    Serial.println(F("  PASS - alarm flag cleared"));
    passed++;
  } else {
    Serial.println(F("  FAIL - alarm flag still set"));
  }

  // Test 2: Set every-minute alarm and wait for it
  Serial.println(F("Test 2: Every-minute alarm (wait up to 65s) ..."));
  DateTime now = rtc.now();
  rtc.setAlarm(now, RV8803_A_EveryMinute);
  rtc.enableInterrupt(RV8803_InterruptAlarm);

  unsigned long start = millis();
  bool fired = false;

  while (millis() - start < 65000) {
    if (rtc.alarmFired()) {
      fired = true;
      Serial.print(F("  Alarm fired after "));
      Serial.print((millis() - start) / 1000);
      Serial.println(F(" seconds"));
      break;
    }
    delay(500);
  }

  if (fired) {
    Serial.println(F("  PASS - alarm fired"));
    passed++;
  } else {
    Serial.println(F("  FAIL - alarm did not fire within 65s"));
  }

  // Test 3: Clear alarm and verify
  Serial.println(F("Test 3: Clear alarm flag after firing ..."));
  rtc.clearAlarm();
  if (!rtc.alarmFired()) {
    Serial.println(F("  PASS - alarm flag cleared"));
    passed++;
  } else {
    Serial.println(F("  FAIL - alarm flag still set"));
  }

  // Test 4: setAlarmDate and setAlarmWeekday round-trip
  Serial.println(F("Test 4: setAlarmDate/setAlarmWeekday round-trip ..."));

  // Test date mode
  rtc.setAlarmDate(15);
  DateTime alarmDt = rtc.getAlarm();
  bool datePass = (alarmDt.day() == 15);
  Serial.print(F("  setAlarmDate(15) -> getAlarm().day() = "));
  Serial.print(alarmDt.day());
  Serial.println(datePass ? F(" PASS") : F(" FAIL"));

  // Test weekday mode (weekday mask 0x08 = bit 3 = Wednesday)
  rtc.setAlarmWeekday(0x08);
  alarmDt = rtc.getAlarm();
  // After setAlarmWeekday, WADA=0, so getAlarm returns weekday
  // Weekday mask 0x08 -> one-hot day 3 (Wednesday)
  bool wdPass = (alarmDt.day() == 3);
  Serial.print(F("  setAlarmWeekday(0x08) -> getAlarm().day() = "));
  Serial.print(alarmDt.day());
  Serial.println(wdPass ? F(" PASS") : F(" FAIL"));

  if (datePass && wdPass) {
    passed++;
  }

  // Disable alarm interrupt
  rtc.disableInterrupt(RV8803_InterruptAlarm);

  Serial.println();
  Serial.print(passed);
  Serial.print(F("/"));
  Serial.print(total);
  Serial.println(F(" tests passed"));
}

void loop() {
  // Nothing to do
}
