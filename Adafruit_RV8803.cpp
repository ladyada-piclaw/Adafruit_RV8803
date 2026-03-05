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

#include <Adafruit_BusIO_Register.h>

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
  uint8_t reg = RV8803_REG_SECONDS;
  uint8_t buffer[7];
  // Burst read seconds through year
  i2c_dev->write_then_read(&reg, 1, buffer, 7);

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
  Adafruit_BusIO_Register flag_reg(i2c_dev, RV8803_REG_FLAG, 1);
  Adafruit_BusIO_RegisterBits v2f(&flag_reg, 1, 1); // V2F is bit 1
  return v2f.read();
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
  Adafruit_BusIO_Register hundredths_reg(i2c_dev, RV8803_REG_HUNDREDTHS, 1);
  return bcd2bin(hundredths_reg.read());
}

/**
 * @brief Read seconds (0-59)
 * @return Seconds value
 */
uint8_t Adafruit_RV8803::getSeconds() {
  Adafruit_BusIO_Register seconds_reg(i2c_dev, RV8803_REG_SECONDS, 1);
  return bcd2bin(seconds_reg.read() & 0x7F);
}

/**
 * @brief Read minutes (0-59)
 * @return Minutes value
 */
uint8_t Adafruit_RV8803::getMinutes() {
  Adafruit_BusIO_Register minutes_reg(i2c_dev, RV8803_REG_MINUTES, 1);
  return bcd2bin(minutes_reg.read() & 0x7F);
}

/**
 * @brief Read hours (0-23)
 * @return Hours value (24-hour format)
 */
uint8_t Adafruit_RV8803::getHours() {
  Adafruit_BusIO_Register hours_reg(i2c_dev, RV8803_REG_HOURS, 1);
  return bcd2bin(hours_reg.read() & 0x3F);
}

/**
 * @brief Read weekday (0-6, 0=Sunday)
 * @return Weekday value converted from one-hot encoding
 */
uint8_t Adafruit_RV8803::getWeekday() {
  Adafruit_BusIO_Register weekday_reg(i2c_dev, RV8803_REG_WEEKDAY, 1);
  return onehot2weekday(weekday_reg.read() & 0x7F);
}

/**
 * @brief Read day of month (1-31)
 * @return Day value
 */
uint8_t Adafruit_RV8803::getDate() {
  Adafruit_BusIO_Register date_reg(i2c_dev, RV8803_REG_DATE, 1);
  return bcd2bin(date_reg.read() & 0x3F);
}

/**
 * @brief Read month (1-12)
 * @return Month value
 */
uint8_t Adafruit_RV8803::getMonth() {
  Adafruit_BusIO_Register month_reg(i2c_dev, RV8803_REG_MONTH, 1);
  return bcd2bin(month_reg.read() & 0x1F);
}

/**
 * @brief Read year (2000-2099)
 * @return Full year value
 */
uint16_t Adafruit_RV8803::getYear() {
  Adafruit_BusIO_Register year_reg(i2c_dev, RV8803_REG_YEAR, 1);
  return bcd2bin(year_reg.read()) + 2000;
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
  uint8_t ae_m = (mode & 0x01) ? 1 : 0;
  uint8_t ae_h = (mode & 0x02) ? 1 : 0;
  uint8_t ae_wd = (mode & 0x04) ? 1 : 0;

  // Minutes alarm register
  Adafruit_BusIO_Register min_alarm_reg(i2c_dev, RV8803_REG_MINUTES_ALARM, 1);
  Adafruit_BusIO_RegisterBits ae_m_bit(&min_alarm_reg, 1, 7);
  uint8_t minutes_alarm = bin2bcd(dt.minute());
  min_alarm_reg.write(minutes_alarm);
  ae_m_bit.write(ae_m);

  // Hours alarm register (preserve GP0 bit)
  Adafruit_BusIO_Register hours_alarm_reg(i2c_dev, RV8803_REG_HOURS_ALARM, 1);
  Adafruit_BusIO_RegisterBits ae_h_bit(&hours_alarm_reg, 1, 7);
  Adafruit_BusIO_RegisterBits gp0_bit(&hours_alarm_reg, 1, 6);
  uint8_t gp0_val = gp0_bit.read();
  hours_alarm_reg.write(bin2bcd(dt.hour()) | (gp0_val << 6));
  ae_h_bit.write(ae_h);

  // Weekday/Date alarm register
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits wada(&ext_reg, 1, 6);

  Adafruit_BusIO_Register wd_alarm_reg(i2c_dev, RV8803_REG_WEEKDAY_DATE_ALARM,
                                       1);
  Adafruit_BusIO_RegisterBits ae_wd_bit(&wd_alarm_reg, 1, 7);
  Adafruit_BusIO_RegisterBits gp1_bit(&wd_alarm_reg, 1, 6);

  if (wada.read()) {
    // Date mode (preserve GP1 bit)
    uint8_t gp1_val = gp1_bit.read();
    wd_alarm_reg.write(bin2bcd(dt.day()) | (gp1_val << 6));
  } else {
    // Weekday mode (one-hot)
    wd_alarm_reg.write(weekday2onehot(dt.dayOfTheWeek()));
  }
  ae_wd_bit.write(ae_wd);

  return true;
}

/**
 * @brief Read the current alarm settings
 * @return DateTime with alarm minute, hour, and day fields
 */
DateTime Adafruit_RV8803::getAlarm() {
  Adafruit_BusIO_Register min_alarm_reg(i2c_dev, RV8803_REG_MINUTES_ALARM, 1);
  Adafruit_BusIO_Register hours_alarm_reg(i2c_dev, RV8803_REG_HOURS_ALARM, 1);
  Adafruit_BusIO_Register wd_alarm_reg(i2c_dev, RV8803_REG_WEEKDAY_DATE_ALARM,
                                       1);
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits wada(&ext_reg, 1, 6);

  uint8_t minutes = bcd2bin(min_alarm_reg.read() & 0x7F);
  uint8_t hours = bcd2bin(hours_alarm_reg.read() & 0x3F);

  uint8_t wd_val = wd_alarm_reg.read();
  uint8_t day;

  if (wada.read()) {
    // Date mode
    day = bcd2bin(wd_val & 0x3F);
  } else {
    // Weekday mode - convert one-hot to day number
    day = onehot2weekday(wd_val & 0x7F);
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
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits wada(&ext_reg, 1, 6);
  wada.write(0);

  // Read current AE bit and write weekday mask
  Adafruit_BusIO_Register wd_alarm_reg(i2c_dev, RV8803_REG_WEEKDAY_DATE_ALARM,
                                       1);
  Adafruit_BusIO_RegisterBits ae_wd_bit(&wd_alarm_reg, 1, 7);
  uint8_t ae_val = ae_wd_bit.read();
  wd_alarm_reg.write((ae_val << 7) | (weekday_mask & 0x7F));
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
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits wada(&ext_reg, 1, 6);
  wada.write(1);

  // Read current AE bit and GP1 bit, write date
  Adafruit_BusIO_Register wd_alarm_reg(i2c_dev, RV8803_REG_WEEKDAY_DATE_ALARM,
                                       1);
  Adafruit_BusIO_RegisterBits ae_wd_bit(&wd_alarm_reg, 1, 7);
  Adafruit_BusIO_RegisterBits gp1_bit(&wd_alarm_reg, 1, 6);
  uint8_t ae_val = ae_wd_bit.read();
  uint8_t gp1_val = gp1_bit.read();
  wd_alarm_reg.write((ae_val << 7) | (gp1_val << 6) | bin2bcd(date));
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
  Adafruit_BusIO_Register flag_reg(i2c_dev, RV8803_REG_FLAG, 1);
  Adafruit_BusIO_RegisterBits af(&flag_reg, 1, 3); // AF is bit 3
  return af.read();
}

/**
 * @brief Clear the alarm flag
 * @return true on success
 */
bool Adafruit_RV8803::clearAlarm() {
  Adafruit_BusIO_Register flag_reg(i2c_dev, RV8803_REG_FLAG, 1);
  Adafruit_BusIO_RegisterBits af(&flag_reg, 1, 3); // AF is bit 3
  return af.write(0);
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
  Adafruit_BusIO_Register tc0_reg(i2c_dev, RV8803_REG_TIMER_COUNTER0, 1);
  tc0_reg.write(value & 0xFF);

  // Write upper 4 bits, preserving GP2-GP5
  Adafruit_BusIO_Register tc1_reg(i2c_dev, RV8803_REG_TIMER_COUNTER1, 1);
  Adafruit_BusIO_RegisterBits timer_upper(&tc1_reg, 4, 0);
  timer_upper.write((value >> 8) & 0x0F);

  // Set TD bits and enable timer (TE=1)
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits td(&ext_reg, 2, 0); // TD is bits 0-1
  Adafruit_BusIO_RegisterBits te(&ext_reg, 1, 4); // TE is bit 4
  td.write(clock & 0x03);
  return te.write(1);
}

/**
 * @brief Disable the countdown timer
 * @return true on success
 */
bool Adafruit_RV8803::disableCountdownTimer() {
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits te(&ext_reg, 1, 4); // TE is bit 4
  return te.write(0);
}

/**
 * @brief Read the countdown timer preset value
 * @return 12-bit timer value (not live countdown)
 */
uint16_t Adafruit_RV8803::getCountdownTimer() {
  Adafruit_BusIO_Register tc0_reg(i2c_dev, RV8803_REG_TIMER_COUNTER0, 1);
  Adafruit_BusIO_Register tc1_reg(i2c_dev, RV8803_REG_TIMER_COUNTER1, 1);
  Adafruit_BusIO_RegisterBits timer_upper(&tc1_reg, 4, 0);
  return tc0_reg.read() | ((uint16_t)timer_upper.read() << 8);
}

/**
 * @brief Check if timer has fired
 * @return true if TF flag is set
 */
bool Adafruit_RV8803::timerFired() {
  Adafruit_BusIO_Register flag_reg(i2c_dev, RV8803_REG_FLAG, 1);
  Adafruit_BusIO_RegisterBits tf(&flag_reg, 1, 4); // TF is bit 4
  return tf.read();
}

/**
 * @brief Clear the timer flag
 * @return true on success
 */
bool Adafruit_RV8803::clearTimer() {
  Adafruit_BusIO_Register flag_reg(i2c_dev, RV8803_REG_FLAG, 1);
  Adafruit_BusIO_RegisterBits tf(&flag_reg, 1, 4); // TF is bit 4
  return tf.write(0);
}

/**
 * @brief Set periodic update mode (second or minute)
 * @param mode RV8803_UpdateSecond or RV8803_UpdateMinute
 * @return true on success
 */
bool Adafruit_RV8803::setUpdateMode(rv8803_update_mode_t mode) {
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits usel(&ext_reg, 1, 5); // USEL is bit 5
  return usel.write(mode == RV8803_UpdateMinute ? 1 : 0);
}

/**
 * @brief Check if periodic update flag is set
 * @return true if UF flag is set
 */
bool Adafruit_RV8803::updateFired() {
  Adafruit_BusIO_Register flag_reg(i2c_dev, RV8803_REG_FLAG, 1);
  Adafruit_BusIO_RegisterBits uf(&flag_reg, 1, 5); // UF is bit 5
  return uf.read();
}

/**
 * @brief Clear the periodic update flag
 * @return true on success
 */
bool Adafruit_RV8803::clearUpdate() {
  Adafruit_BusIO_Register flag_reg(i2c_dev, RV8803_REG_FLAG, 1);
  Adafruit_BusIO_RegisterBits uf(&flag_reg, 1, 5); // UF is bit 5
  return uf.write(0);
}

/**
 * @brief Configure external event detection
 * @param rising_edge true for rising edge, false for falling edge
 * @param filter Event filter time
 * @return true on success
 */
bool Adafruit_RV8803::configureEvent(bool rising_edge,
                                     rv8803_event_filter_t filter) {
  Adafruit_BusIO_Register evctrl_reg(i2c_dev, RV8803_REG_EVENT_CONTROL, 1);
  Adafruit_BusIO_RegisterBits ehl(&evctrl_reg, 1, 6); // EHL is bit 6
  Adafruit_BusIO_RegisterBits et(&evctrl_reg, 2, 4);  // ET is bits 4-5
  ehl.write(rising_edge ? 1 : 0);
  return et.write(filter & 0x03);
}

/**
 * @brief Enable or disable event timestamp capture
 * @param enable true to enable capture
 * @return true on success
 */
bool Adafruit_RV8803::enableEventCapture(bool enable) {
  Adafruit_BusIO_Register evctrl_reg(i2c_dev, RV8803_REG_EVENT_CONTROL, 1);
  Adafruit_BusIO_RegisterBits ecp(&evctrl_reg, 1, 7); // ECP is bit 7
  return ecp.write(enable ? 1 : 0);
}

/**
 * @brief Enable or disable auto-reset of hundredths on event
 * @param enable true to enable auto-reset
 * @return true on success
 */
bool Adafruit_RV8803::enableEventReset(bool enable) {
  Adafruit_BusIO_Register evctrl_reg(i2c_dev, RV8803_REG_EVENT_CONTROL, 1);
  Adafruit_BusIO_RegisterBits erst(&evctrl_reg, 1, 0); // ERST is bit 0
  return erst.write(enable ? 1 : 0);
}

/**
 * @brief Read captured hundredths from external event
 * @return Hundredths value (0-99)
 */
uint8_t Adafruit_RV8803::getEventHundredths() {
  Adafruit_BusIO_Register hundredths_cp_reg(i2c_dev, RV8803_REG_HUNDREDTHS_CP,
                                            1);
  return bcd2bin(hundredths_cp_reg.read());
}

/**
 * @brief Read captured seconds from external event
 * @return Seconds value (0-59)
 */
uint8_t Adafruit_RV8803::getEventSeconds() {
  Adafruit_BusIO_Register seconds_cp_reg(i2c_dev, RV8803_REG_SECONDS_CP, 1);
  return bcd2bin(seconds_cp_reg.read() & 0x7F);
}

/**
 * @brief Check if external event flag is set
 * @return true if EVF flag is set
 */
bool Adafruit_RV8803::eventFired() {
  Adafruit_BusIO_Register flag_reg(i2c_dev, RV8803_REG_FLAG, 1);
  Adafruit_BusIO_RegisterBits evf(&flag_reg, 1, 2); // EVF is bit 2
  return evf.read();
}

/**
 * @brief Clear the external event flag
 * @return true on success
 */
bool Adafruit_RV8803::clearEvent() {
  Adafruit_BusIO_Register flag_reg(i2c_dev, RV8803_REG_FLAG, 1);
  Adafruit_BusIO_RegisterBits evf(&flag_reg, 1, 2); // EVF is bit 2
  return evf.write(0);
}

/**
 * @brief Enable an interrupt source
 * @param source Interrupt source to enable
 * @return true on success
 */
bool Adafruit_RV8803::enableInterrupt(rv8803_interrupt_t source) {
  Adafruit_BusIO_Register ctrl_reg(i2c_dev, RV8803_REG_CONTROL, 1);
  uint8_t ctrl = ctrl_reg.read();
  ctrl |= source;
  return ctrl_reg.write(ctrl);
}

/**
 * @brief Disable an interrupt source
 * @param source Interrupt source to disable
 * @return true on success
 */
bool Adafruit_RV8803::disableInterrupt(rv8803_interrupt_t source) {
  Adafruit_BusIO_Register ctrl_reg(i2c_dev, RV8803_REG_CONTROL, 1);
  uint8_t ctrl = ctrl_reg.read();
  ctrl &= ~source;
  return ctrl_reg.write(ctrl);
}

/**
 * @brief Set the CLKOUT frequency
 * @param mode Square wave frequency mode
 * @return true on success
 * @note CLKOE pin must be tied HIGH for CLKOUT to work
 */
bool Adafruit_RV8803::writeSqwPinMode(rv8803_sqw_mode_t mode) {
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits fd(&ext_reg, 2, 2); // FD is bits 2-3
  return fd.write(mode & 0x03);
}

/**
 * @brief Read the current CLKOUT frequency setting
 * @return Square wave frequency mode
 */
rv8803_sqw_mode_t Adafruit_RV8803::readSqwPinMode() {
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  Adafruit_BusIO_RegisterBits fd(&ext_reg, 2, 2); // FD is bits 2-3
  return (rv8803_sqw_mode_t)fd.read();
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

  Adafruit_BusIO_Register offset_reg(i2c_dev, RV8803_REG_OFFSET, 1);
  Adafruit_BusIO_RegisterBits offset_bits(&offset_reg, 6, 0);
  return offset_bits.write(offset & 0x3F);
}

/**
 * @brief Read the current calibration offset
 * @return Signed offset value (-32 to +31)
 */
int8_t Adafruit_RV8803::getCalibration() {
  Adafruit_BusIO_Register offset_reg(i2c_dev, RV8803_REG_OFFSET, 1);
  Adafruit_BusIO_RegisterBits offset_bits(&offset_reg, 6, 0);
  uint8_t regval = offset_bits.read();
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
  Adafruit_BusIO_Register flag_reg(i2c_dev, RV8803_REG_FLAG, 1);
  Adafruit_BusIO_RegisterBits v1f(&flag_reg, 1, 0); // V1F is bit 0
  return v1f.read();
}

/**
 * @brief Clear both power flags (V1F and V2F)
 * @return true on success
 */
bool Adafruit_RV8803::clearPowerFlags() {
  Adafruit_BusIO_Register flag_reg(i2c_dev, RV8803_REG_FLAG, 1);
  Adafruit_BusIO_RegisterBits v1f(&flag_reg, 1, 0); // V1F is bit 0
  Adafruit_BusIO_RegisterBits v2f(&flag_reg, 1, 1); // V2F is bit 1
  v1f.write(0);
  return v2f.write(0);
}

/**
 * @brief Write to the 1-byte RAM register
 * @param value Byte to store
 * @return true on success
 */
bool Adafruit_RV8803::writeRAM(uint8_t value) {
  Adafruit_BusIO_Register ram_reg(i2c_dev, RV8803_REG_RAM, 1);
  return ram_reg.write(value);
}

/**
 * @brief Read from the 1-byte RAM register
 * @return Stored byte value
 */
uint8_t Adafruit_RV8803::readRAM() {
  Adafruit_BusIO_Register ram_reg(i2c_dev, RV8803_REG_RAM, 1);
  return ram_reg.read();
}

/**
 * @brief Write to general purpose bits (GP0-GP5)
 * @param bits 6-bit value (bit 0=GP0, bit 5=GP5)
 * @return true on success
 * @note GP bits are scattered across multiple registers
 */
bool Adafruit_RV8803::writeGP(uint8_t bits) {
  // GP0 is in Hours Alarm [6]
  Adafruit_BusIO_Register hours_alarm_reg(i2c_dev, RV8803_REG_HOURS_ALARM, 1);
  Adafruit_BusIO_RegisterBits gp0(&hours_alarm_reg, 1, 6);
  gp0.write(bits & 0x01);

  // GP1 is in Weekday/Date Alarm [6]
  Adafruit_BusIO_Register wd_alarm_reg(i2c_dev, RV8803_REG_WEEKDAY_DATE_ALARM,
                                       1);
  Adafruit_BusIO_RegisterBits gp1(&wd_alarm_reg, 1, 6);
  gp1.write((bits >> 1) & 0x01);

  // GP2-GP5 are in Timer Counter 1 [7:4]
  Adafruit_BusIO_Register tc1_reg(i2c_dev, RV8803_REG_TIMER_COUNTER1, 1);
  Adafruit_BusIO_RegisterBits gp2_5(&tc1_reg, 4, 4);
  return gp2_5.write((bits >> 2) & 0x0F);
}

/**
 * @brief Read general purpose bits (GP0-GP5)
 * @return 6-bit value (bit 0=GP0, bit 5=GP5)
 */
uint8_t Adafruit_RV8803::readGP() {
  uint8_t result = 0;

  // GP0 is in Hours Alarm [6]
  Adafruit_BusIO_Register hours_alarm_reg(i2c_dev, RV8803_REG_HOURS_ALARM, 1);
  Adafruit_BusIO_RegisterBits gp0(&hours_alarm_reg, 1, 6);
  result |= gp0.read();

  // GP1 is in Weekday/Date Alarm [6]
  Adafruit_BusIO_Register wd_alarm_reg(i2c_dev, RV8803_REG_WEEKDAY_DATE_ALARM,
                                       1);
  Adafruit_BusIO_RegisterBits gp1(&wd_alarm_reg, 1, 6);
  result |= (gp1.read() << 1);

  // GP2-GP5 are in Timer Counter 1 [7:4]
  Adafruit_BusIO_Register tc1_reg(i2c_dev, RV8803_REG_TIMER_COUNTER1, 1);
  Adafruit_BusIO_RegisterBits gp2_5(&tc1_reg, 4, 4);
  result |= (gp2_5.read() << 2);

  return result;
}

/**
 * @brief Read the Extension Register (0x0D)
 * @return Register value
 */
uint8_t Adafruit_RV8803::readExtensionRegister() {
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  return ext_reg.read();
}

/**
 * @brief Write to the Extension Register (0x0D)
 * @param value Value to write (TEST bit should be 0)
 * @return true on success
 */
bool Adafruit_RV8803::writeExtensionRegister(uint8_t value) {
  value &= ~RV8803_EXT_TEST; // Ensure TEST bit is always 0
  Adafruit_BusIO_Register ext_reg(i2c_dev, RV8803_REG_EXTENSION, 1);
  return ext_reg.write(value);
}

/**
 * @brief Read the Flag Register (0x0E)
 * @return Register value
 */
uint8_t Adafruit_RV8803::readFlagRegister() {
  Adafruit_BusIO_Register flag_reg(i2c_dev, RV8803_REG_FLAG, 1);
  return flag_reg.read();
}

/**
 * @brief Write to the Flag Register (0x0E)
 * @param value Value to write (write 0 to clear flags)
 * @return true on success
 */
bool Adafruit_RV8803::writeFlagRegister(uint8_t value) {
  Adafruit_BusIO_Register flag_reg(i2c_dev, RV8803_REG_FLAG, 1);
  return flag_reg.write(value);
}

/**
 * @brief Read the Control Register (0x0F)
 * @return Register value
 */
uint8_t Adafruit_RV8803::readControlRegister() {
  Adafruit_BusIO_Register ctrl_reg(i2c_dev, RV8803_REG_CONTROL, 1);
  return ctrl_reg.read();
}

/**
 * @brief Write to the Control Register (0x0F)
 * @param value Value to write
 * @return true on success
 */
bool Adafruit_RV8803::writeControlRegister(uint8_t value) {
  Adafruit_BusIO_Register ctrl_reg(i2c_dev, RV8803_REG_CONTROL, 1);
  return ctrl_reg.write(value);
}

/**
 * @brief Read the Event Control Register (0x2F)
 * @return Register value
 */
uint8_t Adafruit_RV8803::readEventControl() {
  Adafruit_BusIO_Register evctrl_reg(i2c_dev, RV8803_REG_EVENT_CONTROL, 1);
  return evctrl_reg.read();
}

/**
 * @brief Write to the Event Control Register (0x2F)
 * @param value Value to write
 * @return true on success
 */
bool Adafruit_RV8803::writeEventControl(uint8_t value) {
  Adafruit_BusIO_Register evctrl_reg(i2c_dev, RV8803_REG_EVENT_CONTROL, 1);
  return evctrl_reg.write(value);
}

/**
 * @brief Perform software reset
 * @return true on success
 * @note Resets prescaler and all registers except time/calendar
 */
bool Adafruit_RV8803::reset() {
  Adafruit_BusIO_Register ctrl_reg(i2c_dev, RV8803_REG_CONTROL, 1);
  Adafruit_BusIO_RegisterBits reset_bit(&ctrl_reg, 1, 0); // RESET is bit 0
  return reset_bit.write(1);
  // RESET bit auto-clears
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
