/*!
 * @file 06_ram.ino
 * @brief Hardware test 06: RAM Read/Write
 *
 * Tests the 1-byte RAM register.
 */

#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("=== HW Test 06: RAM ==="));
  Serial.println();

  if (!rtc.begin()) {
    Serial.println(F("FAIL: RV8803 not found"));
    return;
  }

  uint8_t passed = 0;
  uint8_t total = 4;

  uint8_t testValues[] = {0xA5, 0x5A, 0x00, 0xFF};

  for (int i = 0; i < 4; i++) {
    Serial.print(F("Test "));
    Serial.print(i + 1);
    Serial.print(F(": Write 0x"));
    Serial.print(testValues[i], HEX);
    Serial.print(F(" ... "));

    rtc.writeRAM(testValues[i]);
    uint8_t readBack = rtc.readRAM();

    Serial.print(F("read 0x"));
    Serial.print(readBack, HEX);

    if (readBack == testValues[i]) {
      Serial.println(F(" PASS"));
      passed++;
    } else {
      Serial.println(F(" FAIL"));
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
