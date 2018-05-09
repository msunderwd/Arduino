#include <Arduino.h>
#include <EEPROM.h>
#include <LocoNet.h>
#include "config.h"
#include "eemap.h"
#include "eeprom.hpp"

// Read a 2-byte LocoNet address from EEPROM and
// format it properly as a LocoNet address.
unsigned int readLNAddressFromEEPROM(int idx) {
  // idx = EEPROM address of servo info
  // Read address from memory in LocoNet format
  // LSB is lower 7 bits in first byte (idx)
  // MSB is upper 4 bits in second byte (idx+1)
  unsigned int lsb = EEPROM.read(idx) & 0x7F;
  unsigned int msb = EEPROM.read(idx+1) & 0x0F;
  return(((msb << 7) + lsb) & 0x7FF);
}

// Write a 2-byte LocoNet address to EEPROM
void writeLNAddressToEEPROM(int idx, unsigned int addr) {
  // Store in the LocoNet format of Lower 7 Bits plus Upper 4 bits.
  // LSB is lower 7 bits in first byte (idx)
  // MSB is upper 4 bits in second byte (idx+1)
  EEPROM.write(idx, addr & 0x7F);
  EEPROM.write(idx+1, (addr >> 7) & 0x0F);
}

//---------------------------------------------------------
// Functions for handling Light data

void writeGlobalStateToEEPROM(void) {
  byte val = (master_on ? 1 : 0) +
              (follow_ln ? 2 : 0) +
              (clock_mode ? 4 : 0) +
              (day_night ? 8 : 0) +
              (transition_mode ? 16 : 0);
  EEPROM.write(EEPROM_MODE_SETTINGS, val);
}

void readGlobalStateFromEEPROM(void) {
  byte val = EEPROM.read(EEPROM_MODE_SETTINGS);
  master_on = ((val & 0x01) > 0 ? true : false);
  follow_ln = ((val & 0x02) > 0 ? true : false);
  clock_mode = ((val & 0x04) > 0 ? true : false);
  day_night = ((val & 0x08) > 0 ? true : false);
  transition_mode = ((val & 0x10) > 0 ? true : false);
}

void readLightDataFromEEPROM(int addr, led_t *p) {
  p->red = EEPROM.read(addr);
  p->green = EEPROM.read(addr+1);
  p->blue = EEPROM.read(addr+2);
  p->white = EEPROM.read(addr+3);
}

void writeLightDataToEEPROM(int addr, led_t *p) {
  Serial.println(String(p->red) + " " + String(p->green) + " " + String(p->blue) + " " + String(p->white));
  EEPROM.write(addr, p->red);
  EEPROM.write(addr+1, p->green);
  EEPROM.write(addr+2, p->blue);
  EEPROM.write(addr+3, p->white);
}

// Time data
Time& readTimeFromEEPROM(int addr) {
  unsigned long v;
  Time* rv = new Time(EEPROM.get(addr, v));
  return(*rv);
}

void writeTimeToEEPROM(int addr, Time& t) {
  EEPROM.put(addr, t.makeLong());
}
//---------------------------------------------------------
// Functions for handling LocoNet CVs (when/if implemented)
/*
unsigned int getDecoderCVAddressFromEEPROM() {
  return((EEPROM.read(EEPROM_CV_DECODER_ADDRESS_MSB) << 8) +  EEPROM.read(EEPROM_CV_DECODER_ADDRESS_LSB));
}

void writeCVToEEPROM(int idx, byte val) {
  EEPROM.write(idx, val);
}

byte readCVFromEEPROM(int idx) {
  return(EEPROM.read(idx));
}
*/

