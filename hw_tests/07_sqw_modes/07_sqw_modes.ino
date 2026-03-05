/*!
 * @file 07_sqw_modes.ino
 * @brief Hardware test 07: SQW Pin Modes
 *
 * Tests CLKOUT frequency configuration and verifies read-modify-write
 * logic preserves other Extension register bits.
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
  uint8_t total = 6;

  // Test 1-3: Basic mode round-trip
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
    Serial.print(F(" round-trip ... "));

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

  // Test 4: Read-modify-write preserves TE bit
  Serial.println(F("Test 4: SQW change preserves TE bit ..."));

  // First, set up extension register with TE=1 (timer enable)
  uint8_t extReg = rtc.readExtensionRegister();
  extReg |= 0x10; // Set TE bit
  rtc.writeExtensionRegister(extReg);

  // Verify TE is set
  uint8_t before = rtc.readExtensionRegister();
  bool teBefore = (before & 0x10) != 0;
  Serial.print(F("  TE before: "));
  Serial.println(teBefore ? F("1") : F("0"));

  // Change SQW mode
  rtc.writeSqwPinMode(RV8803_SquareWave1Hz);

  // Verify TE is still set
  uint8_t after = rtc.readExtensionRegister();
  bool teAfter = (after & 0x10) != 0;
  Serial.print(F("  TE after SQW change: "));
  Serial.println(teAfter ? F("1") : F("0"));

  if (teBefore && teAfter) {
    Serial.println(F("  PASS - TE preserved"));
    passed++;
  } else {
    Serial.println(F("  FAIL - TE was clobbered"));
  }

  // Clean up - disable TE
  extReg = rtc.readExtensionRegister();
  extReg &= ~0x10;
  rtc.writeExtensionRegister(extReg);

  // Test 5: Read-modify-write preserves WADA and USEL bits
  Serial.println(F("Test 5: SQW change preserves WADA/USEL bits ..."));

  // Set WADA=1 (0x40) and USEL=1 (0x20)
  extReg = rtc.readExtensionRegister();
  extReg |= 0x60; // Set WADA and USEL
  rtc.writeExtensionRegister(extReg);

  before = rtc.readExtensionRegister();
  uint8_t wadaUselBefore = before & 0x60;
  Serial.print(F("  WADA|USEL before: 0x"));
  Serial.println(wadaUselBefore, HEX);

  // Change SQW mode
  rtc.writeSqwPinMode(RV8803_SquareWave32kHz);

  after = rtc.readExtensionRegister();
  uint8_t wadaUselAfter = after & 0x60;
  Serial.print(F("  WADA|USEL after: 0x"));
  Serial.println(wadaUselAfter, HEX);

  if (wadaUselBefore == wadaUselAfter && wadaUselAfter == 0x60) {
    Serial.println(F("  PASS - WADA/USEL preserved"));
    passed++;
  } else {
    Serial.println(F("  FAIL - WADA/USEL clobbered"));
  }

  // Test 6: Verify TD bits not affected by FD changes
  Serial.println(F("Test 6: SQW change preserves TD bits ..."));

  // Set TD to 0x02 (1Hz timer clock)
  extReg = rtc.readExtensionRegister();
  extReg = (extReg & 0xFC) | 0x02; // Set TD bits to 0x02
  rtc.writeExtensionRegister(extReg);

  before = rtc.readExtensionRegister();
  uint8_t tdBefore = before & 0x03;
  Serial.print(F("  TD before: 0x"));
  Serial.println(tdBefore, HEX);

  // Change SQW mode through all values
  rtc.writeSqwPinMode(RV8803_SquareWave1Hz);
  rtc.writeSqwPinMode(RV8803_SquareWave1kHz);
  rtc.writeSqwPinMode(RV8803_SquareWave32kHz);

  after = rtc.readExtensionRegister();
  uint8_t tdAfter = after & 0x03;
  Serial.print(F("  TD after: 0x"));
  Serial.println(tdAfter, HEX);

  if (tdBefore == tdAfter && tdAfter == 0x02) {
    Serial.println(F("  PASS - TD preserved"));
    passed++;
  } else {
    Serial.println(F("  FAIL - TD clobbered"));
  }

  // Clean up - reset extension register to default
  rtc.writeExtensionRegister(0x00);

  Serial.println();
  Serial.print(passed);
  Serial.print(F("/"));
  Serial.print(total);
  Serial.println(F(" tests passed"));
}

void loop() {
  // Nothing to do
}
