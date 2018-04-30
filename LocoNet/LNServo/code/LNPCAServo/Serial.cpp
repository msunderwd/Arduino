#include <Arduino.h>
#include <LocoNet.h>
#include "config.h"
#include "globals.h"
#include "eeprom.hpp"

extern printServoInfo(int servo);

void handleSerialInput() {
  unsigned int base_address;
  String cmd = Serial.readStringUntil(' ');
  if (cmd.equals("LN")) {
    // Emulate a LocoNet command of format "opcode data1 data2 checksum"
    Serial.println(F("Received LN Command"));
    String opcode = Serial.readStringUntil(' ');
    String data1 = Serial.readStringUntil(' ');
    String data2 = Serial.readStringUntil(' ');
    String checksum = Serial.readStringUntil(' ');
    LnPacket = &SerialLnPacket;
    LnPacket->data[0] = (byte) (opcode.toInt() & 0xFF);
    LnPacket->data[1] = (byte) (data1.toInt() & 0xFF);
    LnPacket->data[2] = (byte) (data2.toInt() & 0xFF);
    LnPacket->data[3] = (byte) (checksum.toInt() & 0xFF);
    LnPacket->sz.command = 0xB0;
    serial_ln_message = true;
    Serial.println("Opcode: " + opcode + " Data1: " + data1 + " Data2 " + data2 + " Chk: " + checksum);
    
  } else if (cmd.equals("C")) {
    // Adjust the CLOSED limit for one of the servos
    Serial.println(F("Received C Command (set closed limit)"));
    int servo = Serial.readStringUntil(' ').toInt();
    float value = Serial.readStringUntil(' ').toFloat();
    //value = (value > MAX_SERVO_VALUE ? MAX_SERVO_VALUE : value);
    Servos[servo-1].setClosedVal(value);
    Servos[servo-1].closeServo();
    Serial.println("Servo: " + String(servo) + " Value: " + String(value));
    
  } else if (cmd.equals("T")) {
    Serial.println(F("Received T Command (set thrown limit)"));
    // Adjust the THROWN limit for one of the servos
    int servo = Serial.readStringUntil(' ').toInt();
    float value = Serial.readStringUntil(' ').toFloat();
    Servos[servo-1].setThrownVal(value);
    Servos[servo-1].throwServo();
    Serial.println("Servo: " + String(servo) + " Value: " + String(value));
    
  } else if (cmd.equals("S")) {
    // Write servo limits to EEPROM
    Serial.println(F("Received S Command (store limits to EEPROM)"));
    writeServoLimitsToEEPROM();
    Serial.println(F("Servo Limits stored to EEPROM."));
    
  } else if (cmd.equals("A")) {
    Serial.println(F("Setting address..."));
    base_address = Serial.readStringUntil(' ').toInt();
    for (int i = 0; i < NUM_SERVOS; i++) {
      Servos[i].setAddress(base_address + i);
      Serial.println("Servo[" + String(i) + "].address = " + String(Servos[i].address()));
      Servos[i].setLockAddress(base_address + i + NUM_SERVOS);
      Serial.println("Servo[" + String(i) + "].lockAddress = " + String(Servos[i].lockAddress()));
      
    }

    writeLNAddressToEEPROM(EEPROM_BASE_ADDRESS, base_address);
    //writeAddressToEEPROM(EEPROM_SERVO_ADDR_BASE, base_address);
    writeServoAddressesToEEPROM();
    Serial.println(" A = " + String(base_address));
    
  } else if (cmd.equals("c")) {
    int servo =  Serial.readStringUntil(' ').toInt();
    Serial.print(F("Close Turnout "));
    Serial.println(String(servo));
    //handleLNServoChange(base_address + servo-1, 0, false);
    doServoChange(servo-1, false);
    
  } else if (cmd.equals("t")) {
    int servo =  Serial.readStringUntil(' ').toInt();
    Serial.print(F("Throw Turnout "));
    Serial.println(String(servo));
    //handleLNServoChange(base_address + servo-1, 1, false);
    doServoChange(servo-1, true);
    
  } else if (cmd.equals("s")) {
    Serial.println(F("Status:"));
    Serial.print(F("decoder address"));
    Serial.print(F("\t"));
    for (int i = 0; i < NUM_SERVOS; i++) {
      printServoInfo(i);
    }
    
  } else if (cmd.equals("H")) {
    Serial.println(F("Serial Port Commands"));
    Serial.println(F("LN <opcode> <data1> <data2> <checksum> : Simulate LocoNet command"));
    Serial.println(F("C <servo> <value> : Set CLOSED state servo limit (0-255)"));
    Serial.println(F("T <servo> <value> : Set THROWN state servo limit (0-255)"));
    Serial.println(F("S : Store servo limits to EEPROM"));
    Serial.println(F("A <address> : Set base LocoNet address (and by extension all addresses) for device"));
    Serial.println(F("c <servo> : CLOSE Servo"));
    Serial.println(F("t <servo> : THROW Servo"));
    Serial.println(F("s : Print status"));
    Serial.println(F("H : Print help"));
  }
}
