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
// Functions for handling Servo data


// Read ALL of the relevant stuff for a single servo from EEPROM
// Index assumed to start from 0.
void readServoDataFromEEPROM(int i) {
  float cv, tv;
  EEPROM.get(EEPROM_SERVO_CLOSED_BASE + (i*4), cv);
  EEPROM.get(EEPROM_SERVO_THROWN_BASE + (i*4), tv);
  Servos[i].initFromEEPROM(readLNAddressFromEEPROM(EEPROM_SERVO_ADDR_BASE + (i*2)),
                           readLNAddressFromEEPROM(EEPROM_LOCK_ADDR_BASE + (i*2)),
                           cv,
                           tv,
                           (EEPROM.read(EEPROM_SERVO_STATE_BASE + i) > 0 ? true : false),
                           (EEPROM.read(EEPROM_LOCK_STATE_BASE + i) > 0 ? true : false),
                           (EEPROM.read(EEPROM_MODE_BASE + i)));
}

// Write ALL of the relevant stuff for a single servo to EEPROM
// Index assumed to start from 0
void writeServoDataToEEPROM(int i) {
  writeLNAddressToEEPROM(i, Servos[i].address());
  writeLNAddressToEEPROM(i, Servos[i].lockAddress());
  EEPROM.put(EEPROM_SERVO_CLOSED_BASE + (i*4), Servos[i].closedVal());
  EEPROM.put(EEPROM_SERVO_THROWN_BASE + (i*4), Servos[i].thrownVal());
  EEPROM.put(EEPROM_MODE_BASE + i, Servos[i].getModeInt());
  writeServoStateToEEPROM(i);
  writeLockStateToEEPROM(i);
}

void writeServoLimitsToEEPROM() {
  for (int i = 0; i < NUM_SERVOS; i++) {
    EEPROM.put(EEPROM_SERVO_CLOSED_BASE + (i*4), Servos[i].closedVal());
    EEPROM.put(EEPROM_SERVO_THROWN_BASE + (i*4), Servos[i].thrownVal());
  }
}

void writeServoStateToEEPROM(int servo) {
  // NOTE: Assumes servo numbering 0-7
  EEPROM.write(EEPROM_SERVO_STATE_BASE + (servo), Servos[servo].isThrown() ? 1 : 0);
  //EEPROM.write(EEPROM_SERVO_STATE_BASE, servo_state);
}

void writeLockStateToEEPROM(int servo) {
  // NOTE: Assumes servo numbering 0-7
  EEPROM.write(EEPROM_LOCK_STATE_BASE + (servo), Servos[servo].isLocked() ? 1 : 0);
}

void writeServoAddressesToEEPROM() {
  unsigned int addr;
  addr = Servos[0].address();
  for (int i = 0; i < NUM_SERVOS; i++) {
    writeLNAddressToEEPROM(EEPROM_SERVO_ADDR_BASE + (i*2), Servos[i].address());
    writeLNAddressToEEPROM(EEPROM_LOCK_ADDR_BASE + (i*2), Servos[i].lockAddress());
  }
}

void writeServoModeToEEPROM(int servo) {
  // NOTE: Assumes servo numbering 0-7
  EEPROM.write(EEPROM_MODE_BASE + (servo), Servos[servo].getModeInt());
}

//---------------------------------------------------------
// Functions for handling LocoNet CVs (when/if implemented)

unsigned int getDecoderCVAddressFromEEPROM() {
  return((EEPROM.read(EEPROM_CV_DECODER_ADDRESS_MSB) << 8) +  EEPROM.read(EEPROM_CV_DECODER_ADDRESS_LSB));
}

void writeCVToEEPROM(int idx, byte val) {
  EEPROM.write(idx, val);
}

byte readCVFromEEPROM(int idx) {
  return(EEPROM.read(idx));
}
