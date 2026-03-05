/*!
 * @file 08_calibration.ino
 * @brief Hardware test 08: Offset Calibration
 *
 * Tests calibrate() and getCalibration() round-trip.
 */

#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("=== HW Test 08: Calibration ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 5;

  int8_t testValues[] = {0, 1, 31, -1, -32};

  for (int i = 0; i < 5; i++) {
    Serial.print(F("Test "));
    Serial.print(i + 1);
    Serial.print(F(": calibrate("));
    Serial.print(testValues[i]);
    Serial.print(F(") ... "));

    rtc.calibrate(testValues[i]);
    int8_t readBack = rtc.getCalibration();

    Serial.print(F("read "));
    Serial.print(readBack);

    if (readBack == testValues[i]) {
      Serial.println(F(" PASS"));
      passed++;
    } else {
      Serial.println(F(" FAIL"));
    }
  }

  // Reset to 0 when done
  rtc.calibrate(0);

  Serial.println();
  Serial.print(passed);
  Serial.print(F("/"));
  Serial.print(total);
  Serial.println(F(" tests passed"));
}

void loop() {
  // Nothing to do
}
