#include <Arduino.h>
#include <LocoNet.h>
#include "config.h"
#include "eeprom.hpp"

void printSettings();
void printSensor(int i);

void handleSerialInput() {
  String cmd = Serial.readStringUntil(' ');
  if (cmd.equals("LN")) {
    // Emulate a LocoNet command of format "opcode data1 data2 checksum"
    Serial.println("Received LN Command");
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
    Serial.println("Opcode: " + opcode + " Data1: " + data1 + " Data2 " + data2 + " Chk: " + checksum);
  } else if (cmd.equals("E")) {
    // Enable/Disable a sensor
    Serial.println(F("Received E Command (Enable Sensor)"));
    int sensor = Serial.readStringUntil(' ').toInt();
    int enable = Serial.readStringUntil(' ').toInt();
    sensor_enable[sensor] = (enable == SENSOR_ENABLED ? SENSOR_ENABLED : SENSOR_DISABLED);
    //writeSensorEnableToEEPROM();

  } else if (cmd.equals("W")) {
    // Write values to EEPROM
    Serial.println(F("Received W Command (Write to EEPROM)"));
    writeSensorEnableToEEPROM();
    Serial.println("Settings stored to EEPROM.");
  } else if (cmd.equals("A")) {
    Serial.println("Setting address...");
    base_address = Serial.readStringUntil(' ').toInt();
    for (int i = 0; i < NUM_SENSORS; i++) {
      sensor_addr[i] = base_address + i;
    }
    writeAddressToEEPROM(EEPROM_BASE_ADDRESS, base_address);
    printSettings();
  } else if (cmd.equals("S")) {
    printSettings();
  } else if (cmd.equals("H")) {
    Serial.println(F("LNSensor Serial Port Commands"));
    Serial.println(F("LN <opcode> <data1> <data2> <checksum> : Simulate LocoNet command"));
    Serial.println(F("E <sensor> <enable> : Enable (1) or Disable (0) a sensor"));
    Serial.println(F("W : Store servo limits to EEPROM"));
    Serial.println(F("A <address> : Set base LocoNet address (and by extension all addresses) for device"));
    Serial.println(F("S : Print status/settings"));
    Serial.println(F("H : Print help"));
  }
}

void printSettings() {
  Serial.print(F("Board Type: "));
#if (BOARD_ID == BOARD_ID_LNTOWER)
  Serial.println(F("LN-Tower"));
#else
  Serial.println(F("LN-IR"));
#endif
  Serial.println(F("Board Settings"));
  Serial.println("\tA = " + String(base_address));
  for (int i = 0; i < NUM_SENSORS; i++) {
    printSensor(i);
  }
}

void printSensor(int i) {
  Serial.print(F("Sensor "));
  Serial.print(i);
  Serial.print(F("\t: A="));
  Serial.print(sensor_addr[i]);
  Serial.print(F("\t: E="));
  Serial.print(sensor_enable[i]);
  Serial.print(F("\t: S="));
  Serial.println(sensorstate[i] == SENSOR_ACTIVE ? "Active" : "Inactive");
}


