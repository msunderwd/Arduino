#include <Arduino.h>
#include <EEPROM.h>
#include <LocoNet.h>
#include "config.h"
#include "eemap.h"

unsigned int getAddressFromEEPROM(int idx) {
  // Read address from memory in LocoNet format
  // LSB is lower 7 bits in first byte (idx)
  // MSB is upper 4 bits in second byte (idx+1)
  unsigned int lsb = EEPROM.read(idx) & 0x7F;
  unsigned int msb = EEPROM.read(idx+1) & 0x0F;
  return(((msb << 7) + lsb) & 0x7FF);
}

void writeAddressToEEPROM(int idx, unsigned int addr) {
  // Store in the LocoNet format of Lower 7 Bits plus Upper 4 bits.
  // LSB is lower 7 bits in first byte (idx)
  // MSB is upper 4 bits in second byte (idx+1)
  EEPROM.write(idx, addr & 0x7F);
  EEPROM.write(idx+1, (addr >> 7) & 0x0F);
}

void readSensorEnableFromEEPROM() {
  for (int i = 0; i < NUM_SENSORS; i++) {
    sensor_enable[i] = EEPROM.read(EEPROM_SENSOR_ENABLE_BASE + i);
  }
}

void writeSensorEnableToEEPROM() {
  for (int i = 0; i < NUM_SENSORS; i++) {
    EEPROM.write(EEPROM_SENSOR_ENABLE_BASE+i, sensor_enable[i]);
  }
  
}

