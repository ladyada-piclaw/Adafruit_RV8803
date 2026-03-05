/*!
 * @file Adafruit_RV8803.cpp
 *
 * @mainpage Adafruit RV-8803-C7 Real-Time Clock Library
 *
 * @section intro_sec Introduction
 *
 * This is a library for the RV-8803-C7 Real-Time Clock from Micro Crystal.
 * The RV8803 is a high-accuracy (±3ppm) RTC with I2C interface, alarm,
 * countdown timer, external event timestamp, CLKOUT, and offset calibration.
 *
 * @section dependencies Dependencies
 *
 * This library depends on:
 * - Adafruit RTClib
 * - Adafruit BusIO
 *
 * @section author Author
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 *
 * @section license License
 *
 * MIT license, all text above must be included in any redistribution
 */

#include "Adafruit_RV8803.h"

/**
 * @brief Initialize the RV8803 RTC
 * @param wire Pointer to TwoWire instance (default &Wire)
 * @return true if device found, false on NACK
 */
bool Adafruit_RV8803::begin(TwoWire* wire) {
  if (!i2c_dev) {
    i2c_dev = new Adafruit_I2CDevice(RV8803_I2C_ADDRESS, wire);
  }
  if (!i2c_dev->begin()) {
    return false;
  }
  _alarmMode = RV8803_A_MinHourDay;
  return true;
}

/**
 * @brief Read current date/time from RTC
 * @return DateTime object with current time
 * @note Burst-reads registers 0x00-0x06 for consistency
 */
DateTime Adafruit_RV8803::now() {
  uint8_t buffer[7];
  // Burst read seconds through year
  buffer[0] = RV8803_REG_SECONDS;
  i2c_dev->write(buffer, 1);
  i2c_dev->read(buffer, 7);

  uint8_t ss = bcd2bin(buffer[0] & 0x7F);
  uint8_t mm = bcd2bin(buffer[1] & 0x7F);
  uint8_t hh = bcd2bin(buffer[2] & 0x3F);
  // Weekday in buffer[3] is one-hot, but DateTime computes dayOfTheWeek()
  uint8_t d = bcd2bin(buffer[4] & 0x3F);
  uint8_t m = bcd2bin(buffer[5] & 0x1F);
  uint16_t y = bcd2bin(buffer[6]) + 2000;

  return DateTime(y, m, d, hh, mm, ss);
}

/**
 * @brief Set the RTC date/time
 * @param dt DateTime object with desired time
 * @note Burst-writes registers 0x00-0x06. Clears V1F and V2F flags.
 */
void Adafruit_RV8803::adjust(const DateTime& dt) {
  uint8_t buffer[8];
  buffer[0] = RV8803_REG_SECONDS;
  buffer[1] = bin2bcd(dt.second());
  buffer[2] = bin2bcd(dt.minute());
  buffer[3] = bin2bcd(dt.hour());
  buffer[4] = weekday2onehot(dt.dayOfTheWeek());
  buffer[5] = bin2bcd(dt.day());
  buffer[6] = bin2bcd(dt.month());
  buffer[7] = bin2bcd(dt.year() - 2000);
  i2c_dev->write(buffer, 8);

  // Clear power flags
  clearPowerFlags();
}

/**
 * @brief Check if RTC lost power and time is invalid
 * @return true if V2F flag is set (time data invalid, must call adjust())
 */
bool Adafruit_RV8803::lostPower() {
  return (readFlagRegister() & RV8803_FLAG_V2F) != 0;
}

/**
 * @brief Check if RTC oscillator is running (has not lost power)
 * @return true if V2F is NOT set (oscillator OK)
 */
bool Adafruit_RV8803::isrunning() {
  return !lostPower();
}

/**
 * @brief Read hundredths of a second (0-99)
 * @return Hundredths value from register 0x10
 * @note This register is read-only and cleared when writing seconds
 */
uint8_t Adafruit_RV8803::getHundredths() {
  return bcd2bin(read_register(RV8803_REG_HUNDREDTHS));
}

/**
 * @brief Read seconds (0-59)
 * @return Seconds value
 */
uint8_t Adafruit_RV8803::getSeconds() {
  return bcd2bin(read_register(RV8803_REG_SECONDS) & 0x7F);
}

/**
 * @brief Read minutes (0-59)
 * @return Minutes value
 */
uint8_t Adafruit_RV8803::getMinutes() {
  return bcd2bin(read_register(RV8803_REG_MINUTES) & 0x7F);
}

/**
 * @brief Read hours (0-23)
 * @return Hours value (24-hour format)
 */
uint8_t Adafruit_RV8803::getHours() {
  return bcd2bin(read_register(RV8803_REG_HOURS) & 0x3F);
}

/**
 * @brief Read weekday (0-6, 0=Sunday)
 * @return Weekday value converted from one-hot encoding
 */
uint8_t Adafruit_RV8803::getWeekday() {
  return onehot2weekday(read_register(RV8803_REG_WEEKDAY) & 0x7F);
}

/**
 * @brief Read day of month (1-31)
 * @return Day value
 */
uint8_t Adafruit_RV8803::getDate() {
  return bcd2bin(read_register(RV8803_REG_DATE) & 0x3F);
}

/**
 * @brief Read month (1-12)
 * @return Month value
 */
uint8_t Adafruit_RV8803::getMonth() {
  return bcd2bin(read_register(RV8803_REG_MONTH) & 0x1F);
}

/**
 * @brief Read year (2000-2099)
 * @return Full year value
 */
uint16_t Adafruit_RV8803::getYear() {
  return bcd2bin(read_register(RV8803_REG_YEAR)) + 2000;
}

/**
 * @brief Set the alarm time and mode
 * @param dt DateTime with alarm time (minute, hour, day fields used)
 * @param mode Alarm mode determining which fields participate
 * @return true on success, false on I2C error
 * @note AE bits are inverted: 0=enabled, 1=disabled
 */
bool Adafruit_RV8803::setAlarm(const DateTime& dt, rv8803_alarm_mode_t mode) {
  _alarmMode = mode;

  // Determine AE bit values based on mode
  // Mode bits: [AE_WD][AE_H][AE_M] where 0=enabled, 1=disabled
  uint8_t ae_m = (mode & 0x01) ? RV8803_ALARM_AE_M : 0;
  uint8_t ae_h = (mode & 0x02) ? RV8803_ALARM_AE_H : 0;
  uint8_t ae_wd = (mode & 0x04) ? RV8803_ALARM_AE_WD : 0;

  // Read current values to preserve GP0 and GP1 bits
  uint8_t hours_reg = read_register(RV8803_REG_HOURS_ALARM);
  uint8_t wd_reg = read_register(RV8803_REG_WEEKDAY_DATE_ALARM);
  uint8_t ext_reg = readExtensionRegister();

  // Minutes alarm
  uint8_t minutes_alarm = bin2bcd(dt.minute()) | ae_m;
  write_register(RV8803_REG_MINUTES_ALARM, minutes_alarm);

  // Hours alarm (preserve GP0 bit)
  uint8_t hours_alarm = (hours_reg & 0x40) | bin2bcd(dt.hour()) | ae_h;
  write_register(RV8803_REG_HOURS_ALARM, hours_alarm);

  // Weekday/Date alarm depends on WADA bit
  if (ext_reg & RV8803_EXT_WADA) {
    // Date mode (preserve GP1 bit)
    uint8_t date_alarm = (wd_reg & 0x40) | bin2bcd(dt.day()) | ae_wd;
    write_register(RV8803_REG_WEEKDAY_DATE_ALARM, date_alarm);
  } else {
    // Weekday mode (one-hot)
    uint8_t weekday_alarm = weekday2onehot(dt.dayOfTheWeek()) | ae_wd;
    write_register(RV8803_REG_WEEKDAY_DATE_ALARM, weekday_alarm);
  }

  return true;
}

/**
 * @brief Read the current alarm settings
 * @return DateTime with alarm minute, hour, and day fields
 */
DateTime Adafruit_RV8803::getAlarm() {
  uint8_t minutes = bcd2bin(read_register(RV8803_REG_MINUTES_ALARM) & 0x7F);
  uint8_t hours = bcd2bin(read_register(RV8803_REG_HOURS_ALARM) & 0x3F);

  uint8_t ext_reg = readExtensionRegister();
  uint8_t wd_reg = read_register(RV8803_REG_WEEKDAY_DATE_ALARM);
  uint8_t day;

  if (ext_reg & RV8803_EXT_WADA) {
    // Date mode
    day = bcd2bin(wd_reg & 0x3F);
  } else {
    // Weekday mode - convert one-hot to day number
    day = onehot2weekday(wd_reg & 0x7F);
  }

  // Return DateTime with alarm fields (year/month set to minimum)
  return DateTime(2000, 1, day, hours, minutes, 0);
}

/**
 * @brief Set weekday alarm with multi-day mask (WADA=0)
 * @param weekday_mask One-hot mask for days (bit 0=day 1, bit 6=day 7)
 * @return true on success
 * @note Sets WADA=0 for weekday mode
 */
bool Adafruit_RV8803::setAlarmWeekday(uint8_t weekday_mask) {
  // Set WADA=0 for weekday mode
  uint8_t ext_reg = readExtensionRegister();
  ext_reg &= ~RV8803_EXT_WADA;
  writeExtensionRegister(ext_reg);

  // Read current AE bit and write weekday mask
  uint8_t ae_bit = read_register(RV8803_REG_WEEKDAY_DATE_ALARM) & 0x80;
  write_register(RV8803_REG_WEEKDAY_DATE_ALARM, ae_bit | (weekday_mask & 0x7F));
  return true;
}

/**
 * @brief Set date alarm (WADA=1)
 * @param date Day of month (1-31)
 * @return true on success
 * @note Sets WADA=1 for date mode
 */
bool Adafruit_RV8803::setAlarmDate(uint8_t date) {
  // Set WADA=1 for date mode
  uint8_t ext_reg = readExtensionRegister();
  ext_reg |= RV8803_EXT_WADA;
  writeExtensionRegister(ext_reg);

  // Read current AE bit and GP1 bit, write date
  uint8_t wd_reg = read_register(RV8803_REG_WEEKDAY_DATE_ALARM);
  uint8_t preserved = wd_reg & 0xC0; // AE_WD and GP1
  write_register(RV8803_REG_WEEKDAY_DATE_ALARM, preserved | bin2bcd(date));
  return true;
}

/**
 * @brief Get the current alarm mode
 * @return rv8803_alarm_mode_t enum value
 */
rv8803_alarm_mode_t Adafruit_RV8803::getAlarmMode() {
  return _alarmMode;
}

/**
 * @brief Check if alarm has fired
 * @return true if AF flag is set
 */
bool Adafruit_RV8803::alarmFired() {
  return (readFlagRegister() & RV8803_FLAG_AF) != 0;
}

/**
 * @brief Clear the alarm flag
 * @return true on success
 */
bool Adafruit_RV8803::clearAlarm() {
  uint8_t flags = readFlagRegister();
  flags &= ~RV8803_FLAG_AF; // Clear AF by writing 0
  return writeFlagRegister(flags);
}

/**
 * @brief Enable the countdown timer
 * @param clock Timer clock source (frequency)
 * @param value 12-bit countdown value (0-4095)
 * @return true on success
 * @note Preserves GP2-GP5 bits in Timer Counter 1 register
 */
bool Adafruit_RV8803::enableCountdownTimer(rv8803_timer_clock_t clock,
                                           uint16_t value) {
  // Limit to 12 bits
  if (value > 4095) {
    value = 4095;
  }

  // Write lower 8 bits
  write_register(RV8803_REG_TIMER_COUNTER0, value & 0xFF);

  // Read Timer Counter 1 to preserve GP2-GP5 bits
  uint8_t tc1 = read_register(RV8803_REG_TIMER_COUNTER1);
  tc1 = (tc1 & 0xF0) | ((value >> 8) & 0x0F);
  write_register(RV8803_REG_TIMER_COUNTER1, tc1);

  // Set TD bits and enable timer (TE=1)
  uint8_t ext_reg = readExtensionRegister();
  ext_reg &= ~(RV8803_EXT_TD_MASK | RV8803_EXT_TE);
  ext_reg |= (clock & RV8803_EXT_TD_MASK) | RV8803_EXT_TE;
  return writeExtensionRegister(ext_reg);
}

/**
 * @brief Disable the countdown timer
 * @return true on success
 */
bool Adafruit_RV8803::disableCountdownTimer() {
  uint8_t ext_reg = readExtensionRegister();
  ext_reg &= ~RV8803_EXT_TE;
  return writeExtensionRegister(ext_reg);
}

/**
 * @brief Read the countdown timer preset value
 * @return 12-bit timer value (not live countdown)
 */
uint16_t Adafruit_RV8803::getCountdownTimer() {
  uint8_t tc0 = read_register(RV8803_REG_TIMER_COUNTER0);
  uint8_t tc1 = read_register(RV8803_REG_TIMER_COUNTER1);
  return tc0 | ((uint16_t)(tc1 & 0x0F) << 8);
}

/**
 * @brief Check if timer has fired
 * @return true if TF flag is set
 */
bool Adafruit_RV8803::timerFired() {
  return (readFlagRegister() & RV8803_FLAG_TF) != 0;
}

/**
 * @brief Clear the timer flag
 * @return true on success
 */
bool Adafruit_RV8803::clearTimer() {
  uint8_t flags = readFlagRegister();
  flags &= ~RV8803_FLAG_TF;
  return writeFlagRegister(flags);
}

/**
 * @brief Set periodic update mode (second or minute)
 * @param mode RV8803_UpdateSecond or RV8803_UpdateMinute
 * @return true on success
 */
bool Adafruit_RV8803::setUpdateMode(rv8803_update_mode_t mode) {
  uint8_t ext_reg = readExtensionRegister();
  if (mode == RV8803_UpdateMinute) {
    ext_reg |= RV8803_EXT_USEL;
  } else {
    ext_reg &= ~RV8803_EXT_USEL;
  }
  return writeExtensionRegister(ext_reg);
}

/**
 * @brief Check if periodic update flag is set
 * @return true if UF flag is set
 */
bool Adafruit_RV8803::updateFired() {
  return (readFlagRegister() & RV8803_FLAG_UF) != 0;
}

/**
 * @brief Clear the periodic update flag
 * @return true on success
 */
bool Adafruit_RV8803::clearUpdate() {
  uint8_t flags = readFlagRegister();
  flags &= ~RV8803_FLAG_UF;
  return writeFlagRegister(flags);
}

/**
 * @brief Configure external event detection
 * @param rising_edge true for rising edge, false for falling edge
 * @param filter Event filter time
 * @return true on success
 */
bool Adafruit_RV8803::configureEvent(bool rising_edge,
                                     rv8803_event_filter_t filter) {
  uint8_t evctrl = readEventControl();
  evctrl &= ~(RV8803_EVCTRL_EHL | RV8803_EVCTRL_ET_MASK);
  if (rising_edge) {
    evctrl |= RV8803_EVCTRL_EHL;
  }
  evctrl |= ((filter << RV8803_EVCTRL_ET_SHIFT) & RV8803_EVCTRL_ET_MASK);
  return writeEventControl(evctrl);
}

/**
 * @brief Enable or disable event timestamp capture
 * @param enable true to enable capture
 * @return true on success
 */
bool Adafruit_RV8803::enableEventCapture(bool enable) {
  uint8_t evctrl = readEventControl();
  if (enable) {
    evctrl |= RV8803_EVCTRL_ECP;
  } else {
    evctrl &= ~RV8803_EVCTRL_ECP;
  }
  return writeEventControl(evctrl);
}

/**
 * @brief Enable or disable auto-reset of hundredths on event
 * @param enable true to enable auto-reset
 * @return true on success
 */
bool Adafruit_RV8803::enableEventReset(bool enable) {
  uint8_t evctrl = readEventControl();
  if (enable) {
    evctrl |= RV8803_EVCTRL_ERST;
  } else {
    evctrl &= ~RV8803_EVCTRL_ERST;
  }
  return writeEventControl(evctrl);
}

/**
 * @brief Read captured hundredths from external event
 * @return Hundredths value (0-99)
 */
uint8_t Adafruit_RV8803::getEventHundredths() {
  return bcd2bin(read_register(RV8803_REG_HUNDREDTHS_CP));
}

/**
 * @brief Read captured seconds from external event
 * @return Seconds value (0-59)
 */
uint8_t Adafruit_RV8803::getEventSeconds() {
  return bcd2bin(read_register(RV8803_REG_SECONDS_CP) & 0x7F);
}

/**
 * @brief Check if external event flag is set
 * @return true if EVF flag is set
 */
bool Adafruit_RV8803::eventFired() {
  return (readFlagRegister() & RV8803_FLAG_EVF) != 0;
}

/**
 * @brief Clear the external event flag
 * @return true on success
 */
bool Adafruit_RV8803::clearEvent() {
  uint8_t flags = readFlagRegister();
  flags &= ~RV8803_FLAG_EVF;
  return writeFlagRegister(flags);
}

/**
 * @brief Enable an interrupt source
 * @param source Interrupt source to enable
 * @return true on success
 */
bool Adafruit_RV8803::enableInterrupt(rv8803_interrupt_t source) {
  uint8_t ctrl = readControlRegister();
  ctrl |= source;
  return writeControlRegister(ctrl);
}

/**
 * @brief Disable an interrupt source
 * @param source Interrupt source to disable
 * @return true on success
 */
bool Adafruit_RV8803::disableInterrupt(rv8803_interrupt_t source) {
  uint8_t ctrl = readControlRegister();
  ctrl &= ~source;
  return writeControlRegister(ctrl);
}

/**
 * @brief Set the CLKOUT frequency
 * @param mode Square wave frequency mode
 * @return true on success
 * @note CLKOE pin must be tied HIGH for CLKOUT to work
 */
bool Adafruit_RV8803::writeSqwPinMode(rv8803_sqw_mode_t mode) {
  uint8_t ext_reg = readExtensionRegister();
  ext_reg &= ~RV8803_EXT_FD_MASK;
  ext_reg |= ((mode << RV8803_EXT_FD_SHIFT) & RV8803_EXT_FD_MASK);
  return writeExtensionRegister(ext_reg);
}

/**
 * @brief Read the current CLKOUT frequency setting
 * @return Square wave frequency mode
 */
rv8803_sqw_mode_t Adafruit_RV8803::readSqwPinMode() {
  uint8_t ext_reg = readExtensionRegister();
  return (rv8803_sqw_mode_t)((ext_reg & RV8803_EXT_FD_MASK) >>
                             RV8803_EXT_FD_SHIFT);
}

/**
 * @brief Set the offset calibration value
 * @param offset 6-bit two's complement (-32 to +31), 0.2384 ppm/step
 * @return true on success
 */
bool Adafruit_RV8803::calibrate(int8_t offset) {
  // Clamp to 6-bit signed range
  if (offset > 31)
    offset = 31;
  if (offset < -32)
    offset = -32;

  // Convert to 6-bit representation
  uint8_t regval = offset & 0x3F;
  write_register(RV8803_REG_OFFSET, regval);
  return true;
}

/**
 * @brief Read the current calibration offset
 * @return Signed offset value (-32 to +31)
 */
int8_t Adafruit_RV8803::getCalibration() {
  uint8_t regval = read_register(RV8803_REG_OFFSET) & 0x3F;
  // Sign-extend from 6 bits
  if (regval & 0x20) {
    return (int8_t)(regval | 0xC0);
  }
  return (int8_t)regval;
}

/**
 * @brief Check if temperature compensation was interrupted
 * @return true if V1F flag is set
 */
bool Adafruit_RV8803::tempCompStopped() {
  return (readFlagRegister() & RV8803_FLAG_V1F) != 0;
}

/**
 * @brief Clear both power flags (V1F and V2F)
 * @return true on success
 */
bool Adafruit_RV8803::clearPowerFlags() {
  uint8_t flags = readFlagRegister();
  flags &= ~(RV8803_FLAG_V1F | RV8803_FLAG_V2F);
  return writeFlagRegister(flags);
}

/**
 * @brief Write to the 1-byte RAM register
 * @param value Byte to store
 * @return true on success
 */
bool Adafruit_RV8803::writeRAM(uint8_t value) {
  write_register(RV8803_REG_RAM, value);
  return true;
}

/**
 * @brief Read from the 1-byte RAM register
 * @return Stored byte value
 */
uint8_t Adafruit_RV8803::readRAM() {
  return read_register(RV8803_REG_RAM);
}

/**
 * @brief Write to general purpose bits (GP0-GP5)
 * @param bits 6-bit value (bit 0=GP0, bit 5=GP5)
 * @return true on success
 * @note GP bits are scattered across multiple registers
 */
bool Adafruit_RV8803::writeGP(uint8_t bits) {
  // GP0 is in Hours Alarm [6]
  uint8_t hours_alarm = read_register(RV8803_REG_HOURS_ALARM);
  hours_alarm = (hours_alarm & ~0x40) | ((bits & 0x01) << 6);
  write_register(RV8803_REG_HOURS_ALARM, hours_alarm);

  // GP1 is in Weekday/Date Alarm [6] (only when WADA=1, but we write anyway)
  uint8_t wd_alarm = read_register(RV8803_REG_WEEKDAY_DATE_ALARM);
  wd_alarm = (wd_alarm & ~0x40) | ((bits & 0x02) << 5);
  write_register(RV8803_REG_WEEKDAY_DATE_ALARM, wd_alarm);

  // GP2-GP5 are in Timer Counter 1 [7:4]
  uint8_t tc1 = read_register(RV8803_REG_TIMER_COUNTER1);
  tc1 = (tc1 & 0x0F) | ((bits & 0x3C) << 2);
  write_register(RV8803_REG_TIMER_COUNTER1, tc1);

  return true;
}

/**
 * @brief Read general purpose bits (GP0-GP5)
 * @return 6-bit value (bit 0=GP0, bit 5=GP5)
 */
uint8_t Adafruit_RV8803::readGP() {
  uint8_t result = 0;

  // GP0 is in Hours Alarm [6]
  uint8_t hours_alarm = read_register(RV8803_REG_HOURS_ALARM);
  result |= (hours_alarm >> 6) & 0x01;

  // GP1 is in Weekday/Date Alarm [6]
  uint8_t wd_alarm = read_register(RV8803_REG_WEEKDAY_DATE_ALARM);
  result |= (wd_alarm >> 5) & 0x02;

  // GP2-GP5 are in Timer Counter 1 [7:4]
  uint8_t tc1 = read_register(RV8803_REG_TIMER_COUNTER1);
  result |= (tc1 >> 2) & 0x3C;

  return result;
}

/**
 * @brief Read the Extension Register (0x0D)
 * @return Register value
 */
uint8_t Adafruit_RV8803::readExtensionRegister() {
  return read_register(RV8803_REG_EXTENSION);
}

/**
 * @brief Write to the Extension Register (0x0D)
 * @param value Value to write (TEST bit should be 0)
 * @return true on success
 */
bool Adafruit_RV8803::writeExtensionRegister(uint8_t value) {
  value &= ~RV8803_EXT_TEST; // Ensure TEST bit is always 0
  write_register(RV8803_REG_EXTENSION, value);
  return true;
}

/**
 * @brief Read the Flag Register (0x0E)
 * @return Register value
 */
uint8_t Adafruit_RV8803::readFlagRegister() {
  return read_register(RV8803_REG_FLAG);
}

/**
 * @brief Write to the Flag Register (0x0E)
 * @param value Value to write (write 0 to clear flags)
 * @return true on success
 */
bool Adafruit_RV8803::writeFlagRegister(uint8_t value) {
  write_register(RV8803_REG_FLAG, value);
  return true;
}

/**
 * @brief Read the Control Register (0x0F)
 * @return Register value
 */
uint8_t Adafruit_RV8803::readControlRegister() {
  return read_register(RV8803_REG_CONTROL);
}

/**
 * @brief Write to the Control Register (0x0F)
 * @param value Value to write
 * @return true on success
 */
bool Adafruit_RV8803::writeControlRegister(uint8_t value) {
  write_register(RV8803_REG_CONTROL, value);
  return true;
}

/**
 * @brief Read the Event Control Register (0x2F)
 * @return Register value
 */
uint8_t Adafruit_RV8803::readEventControl() {
  return read_register(RV8803_REG_EVENT_CONTROL);
}

/**
 * @brief Write to the Event Control Register (0x2F)
 * @param value Value to write
 * @return true on success
 */
bool Adafruit_RV8803::writeEventControl(uint8_t value) {
  write_register(RV8803_REG_EVENT_CONTROL, value);
  return true;
}

/**
 * @brief Perform software reset
 * @return true on success
 * @note Resets prescaler and all registers except time/calendar
 */
bool Adafruit_RV8803::reset() {
  uint8_t ctrl = readControlRegister();
  ctrl |= RV8803_CTRL_RESET;
  writeControlRegister(ctrl);
  // RESET bit auto-clears
  return true;
}

/**
 * @brief Convert weekday (0-6) to one-hot encoding
 * @param day Weekday number (0=Sunday, 6=Saturday)
 * @return One-hot encoded byte (bit 0 = Sunday)
 */
uint8_t Adafruit_RV8803::weekday2onehot(uint8_t day) {
  if (day > 6)
    day = 0;
  return 1 << day;
}

/**
 * @brief Convert one-hot encoding to weekday (0-6)
 * @param bits One-hot encoded byte
 * @return Weekday number (0=Sunday, 6=Saturday)
 */
uint8_t Adafruit_RV8803::onehot2weekday(uint8_t bits) {
  for (uint8_t i = 0; i < 7; i++) {
    if (bits & (1 << i)) {
      return i;
    }
  }
  return 0; // Default to Sunday if no bit set
}
