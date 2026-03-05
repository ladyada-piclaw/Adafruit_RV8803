/*!
 * @file simpletest.ino
 *
 * Simple example for the Adafruit RV8803 RTC library.
 * Reads and prints the time every second.
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 */

#include <Adafruit_RV8803.h>

Adafruit_RV8803 rtc;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10); // Wait for serial on native USB boards

  Serial.println(F("Adafruit RV8803 Simple Test"));

  if (!rtc.begin()) {
    Serial.println(F("Couldn't find RV8803"));
    while (1)
      delay(10);
  }
  Serial.println(F("RV8803 found!"));

  // Check if RTC lost power and needs time set
  if (rtc.lostPower()) {
    Serial.println(F("RTC lost power, setting time to compile time"));
    // Set to compile time
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Print initial status
  Serial.print(F("V1F (temp comp stopped): "));
  Serial.println(rtc.tempCompStopped() ? F("Yes") : F("No"));

  // Clear any power flags
  rtc.clearPowerFlags();
}

void loop() {
  DateTime now = rtc.now();

  // Print date
  Serial.print(now.year(), DEC);
  Serial.print(F("/"));
  if (now.month() < 10)
    Serial.print(F("0"));
  Serial.print(now.month(), DEC);
  Serial.print(F("/"));
  if (now.day() < 10)
    Serial.print(F("0"));
  Serial.print(now.day(), DEC);

  Serial.print(F(" "));

  // Print time
  if (now.hour() < 10)
    Serial.print(F("0"));
  Serial.print(now.hour(), DEC);
  Serial.print(F(":"));
  if (now.minute() < 10)
    Serial.print(F("0"));
  Serial.print(now.minute(), DEC);
  Serial.print(F(":"));
  if (now.second() < 10)
    Serial.print(F("0"));
  Serial.print(now.second(), DEC);

  // Print hundredths
  Serial.print(F("."));
  uint8_t hundredths = rtc.getHundredths();
  if (hundredths < 10)
    Serial.print(F("0"));
  Serial.print(hundredths, DEC);

  // Print day of week
  const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  Serial.print(F(" ("));
  Serial.print(days[now.dayOfTheWeek()]);
  Serial.println(F(")"));

  delay(1000);
}
