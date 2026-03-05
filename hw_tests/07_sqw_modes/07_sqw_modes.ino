/*!
 * @file 07_sqw_modes.ino
 * @brief Hardware test 07: SQW Pin Modes
 *
 * Tests CLKOUT frequency configuration.
 */

#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("=== HW Test 07: SQW Modes ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 3;

  struct {
    rv8803_sqw_mode_t mode;
    const char* name;
  } modes[] = {{RV8803_SquareWave32kHz, "32kHz"},
               {RV8803_SquareWave1kHz, "1kHz"},
               {RV8803_SquareWave1Hz, "1Hz"}};

  for (int i = 0; i < 3; i++) {
    Serial.print(F("Test "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(modes[i].name);
    Serial.print(F(" ... "));

    rtc.writeSqwPinMode(modes[i].mode);
    rv8803_sqw_mode_t readBack = rtc.readSqwPinMode();

    if (readBack == modes[i].mode) {
      Serial.println(F("PASS"));
      passed++;
    } else {
      Serial.print(F("FAIL (got "));
      Serial.print(readBack);
      Serial.println(F(")"));
    }
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
