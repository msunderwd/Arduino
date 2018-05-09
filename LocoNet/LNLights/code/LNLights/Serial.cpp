#include <Arduino.h>
#include <LocoNet.h>
#include <EEPROM.h>
#include "config.h"
#include "globals.h"
#include "eeprom.hpp"
#include "eemap.h"

#define USE_ALT_SERIAL_INPUT 0

#define INPUT_SIZE 30

#define UNITS_PERCENT "P"
#define UNITS_255 "B"

extern int default_daytime[4];
extern int default_nighttime[4];
extern int default_sunrise[DEFAULT_TABLE_LENGTH][4];
extern int default_sunset[DEFAULT_TABLE_LENGTH][4];

void printHelp(void);
void toUpper(char *s);
int getInt(char *p, char *d, char *s = NULL);
byte getLEDValue(String unit);
void writeDefaultTablesToEEPROM(void);

#if (USE_ALT_SERIAL_INPUT > 0) 

void handleSerialInput() {
  char input[INPUT_SIZE+1];
  byte size = Serial.readBytes(input, INPUT_SIZE);
  input[size] = 0;

  char *command = strtok(input, " ");
  if (command != 0) {
    if (strcmp(command, "LN") == 0) {
      // Emulate a LocoNet command of format "opcode data1 data2 checksum"
      Serial.println(F("Received LN Command"));
      LnPacket = &SerialLnPacket;
      LnPacket->data[0] = (byte) (getInt(NULL, " ", "data[0]") & 0xFF);
      LnPacket->data[1] = (byte) (getInt(NULL, " ", "data[1]") & 0xFF);
      LnPacket->data[2] = (byte) (getInt(NULL, " ", "data[2]") & 0xFF);
      LnPacket->data[3] = (byte) (getInt(NULL, " ", "data[3]") & 0xFF);
      LnPacket->sz.command = 0xB0;
      serial_ln_message = true;
      Serial.print(F("Opcode: "));
      Serial.print(LnPacket->data[0]);
      Serial.print(F(" Data1: "));
      Serial.print(LnPacket->data[1]);
      Serial.print(F(" Data2 "));
      Serial.print(LnPacket->data[2]); 
      Serial.print(F(" Chk: "));
      Serial.println(LnPacket->data[3]);
    }
    else if (strcmp(command, "SD") == 0) {
    // Write Light Data to EEPROM
    // SD <mode> [<step>] <red> <green> <blue> <white> : Store light data
    //    <mode> = "D" (day) "N" (night) "SR" (sunrise) "SS" (sunset)
    //    <step> = Step number for sunrise/sunset table (1-N)
    int addr, lstep;
    Serial.println(F("Received SD Command (store light settings to EEPROM)"));
    char *val = strtok(NULL, " ");
    if (val == 0) {
      Serial.println("Invalid mode");
      return;
    }
    toUpper(val);
    if (strcmp(val,"D") == 0) {
      addr = EEPROM_DAY_SETTINGS;
      Serial.println(F("Daylight settings"));
    } else if (strcmp(val,"N") == 0) {
      addr = EEPROM_NIGHT_SETTINGS;
      Serial.println(F("Night settings"));
    } else if (strcmp(val,"SR") == 0) {
      Serial.print(F("Sunrise settings step: "));
      lstep = getInt(NULL, " ", "step number");
      if (lstep > EEPROM.read(EEPROM_SUNRISE_STEPS)) {
        // Invalid value. Ignore.
        return;
      }
      if (lstep == 0) {
        // Assume this was a mistake, and bump it to 1.
        lstep = 1;
      }
      addr = EEPROM_SUNRISE_TABLE + lstep - 1;
      Serial.println(lstep);
    } else if (strcmp(val,"SS") == 0) {
      Serial.print(F("Sunset settings step: "));
      lstep = getInt(NULL, " ", "step number");
      if (lstep > EEPROM.read(EEPROM_SUNSET_STEPS)) {
        // Invalid value. Ignore.
        return;
      }
      if (lstep == 0) {
        // Assume this was a mistake, and bump it to 1.
        lstep = 1;
      }
      addr = EEPROM_SUNSET_TABLE + lstep - 1;
      Serial.println(lstep);
    } else {
      // Invalid mode. Ignore.
      return;
    }
    Serial.print(F("Addr = "));
    Serial.println(addr);
    led_t light;
    light.red = (byte) (getInt(NULL, " ") & 0xFF);
    light.green = (byte) (getInt(NULL, " ") & 0xFF);
    light.blue = (byte) (getInt(NULL, " ") & 0xFF);
    light.white = (byte) (getInt(NULL, " ") & 0xFF);
    Serial.print(F("Light red = "));
    Serial.print(light.red);
    Serial.print(F("\tgreen = "));
    Serial.print(light.green);
    Serial.print(F("\tblue = "));
    Serial.print(light.blue);
    Serial.print(F("\twhite = "));
    Serial.println(light.white);
    writeLightDataToEEPROM(addr, &light);
      
    }
    else if (strcmp(command, "ST") == 0) {
      // Write number of steps for sunrise/sunset to EEPROM
      // ST <mode> <steps> <increment>: Set number of steps for sunrise or sunset
      //    <mode> = "SR" (sunrise) or "SS" (sunset)
      //    <steps> = 1-32
      //    <increment> = time in fast clock minutes between steps
      char *mode = strtok(NULL, " ");
      if (mode == 0) {
        Serial.println("Invalid mode");
        return;
      }
      toUpper(mode);
      byte steps = (byte) (getInt(NULL, " ", "steps") & 0xFF);
      int increment = getInt(NULL, " ", "increment");
      Serial.print(F(" Read: steps = "));
      Serial.print(steps);
      Serial.print(F(" inc = "));
      Serial.println(increment);
      if (strcmp(mode,"SR") == 0) {
        EEPROM.write(EEPROM_SUNRISE_STEPS, steps);
        EEPROM.put(EEPROM_SUNRISE_INCREMENT, increment); 
      } else if (strcmp(mode,"SS") == 0) {
        EEPROM.write(EEPROM_SUNSET_STEPS, steps);
        EEPROM.put(EEPROM_SUNSET_INCREMENT, increment); 
      } // else do nothing.
    }
    else if (strcmp(command, "TS") == 0) {
      // Set start time of sunset or sunrise
      // TS <mode> <hours>:<mins>
      //    <mode> = "SR" (sunrise) or "SS" (sunset)
      //    <mins> <hours> = start time of transition
      char *mode = strtok(NULL, " ");
      if (mode == 0) {
        Serial.println("Invalid mode");
        return;
      }
      toUpper(mode);
      Time* t = new Time();
      t->setHours(getInt(NULL, " ", "hours"));
      t->setMins(getInt(NULL, " ", "minutes"));
      //t->setDays(0);
      if (strcmp(mode,"SR") == 0) {
        EEPROM.put(EEPROM_SUNRISE_START_TIME, *t);
        Serial.print(F("Sunrise:         "));
        Serial.println(t->toStringHHMM());
      } else if (strcmp(mode,"SS") == 0) {
        EEPROM.put(EEPROM_SUNSET_START_TIME, *t);
        Serial.print(F("Sunset:          "));
        Serial.println(t->toStringHHMM());
      }
      
    }
    else if (strcmp(command, "A") == 0) {
      // A <address> : Set base LocoNet address (and by extension all addresses) for device
      Serial.println(F("Setting address..."));
      base_address = getInt(NULL, " ", "base address");
      writeLNAddressToEEPROM(EEPROM_BASE_ADDRESS, base_address);
      Serial.print(F(" A = "));
      Serial.println(base_address);
    
    }
    else if (strcmp(command, "P") == 0) {
      // P <on/off> : Turn master power on or off
      Serial.print(F("Master Power: "));
      char *val = strtok(NULL, " ");
      if (val == 0) {
        Serial.println(F("Invalid power value"));
      }
      toUpper(val);
      if (strcmp(val, "ON") == 0) {
        Serial.println(F("ON"));
        setPower(true);
      } else if (strcmp(val, "OFF") == 0) {
        Serial.println(F("OFF"));
        setPower(false);
      } else {
        Serial.print(val); Serial.println("??");
      }
      
    }
    else if (strcmp(command, "SM") == 0) {
      char *mode = strtok(NULL, " ");
      if (mode == 0) {
        Serial.println("Invalid mode");
        return;
      }
      char *sval = strtok(NULL, " ");
      if (sval == 0) {
        Serial.println("Invalid value");
        return;
      }
      toUpper(mode);
      toUpper(sval);
      Serial.println("Set Mode " + String(mode) + " to " + String(sval) + ".");
      bool val = (strcmp(sval,"TRUE") == 0 ? true : false);
      if (strcmp(mode,"CLOCK") == 0) {
        setClockMode(val);
      } else if (strcmp(mode,"DAY") == 0) {
        setDayNightMode(true);
      } else if (strcmp(mode,"NIGHT") == 0) {
        setDayNightMode(false);
      } else if (strcmp(mode,"TRANSITION") == 0) {
        setTransitionMode(val);
      } else if (strcmp(mode,"FOLLOW") == 0) {
        setFollowLNPower(val);
      }
      
    }
    else if (strcmp(command, "FC") == 0) {
      char *v = strtok(NULL, " ");
      int h = getInt(NULL, " ", "hours");
      int m = getInt(NULL, " ", "minutes");
      Serial.print(F("Set Clock "));
      Serial.print(h);
      Serial.print(F(":"));
      Serial.print(m);
      current_time.setHours(h);
      current_time.setMins(m);
      Serial.print(F("\tNew Clock: "));
      Serial.println(current_time.toStringHHMM());
      
    }
    else if (strcmp(command, "H") == 0) {
      printHelp();
    }
    else if (strcmp(command, "WD") == 0) {
      writeDefaultTablesToEEPROM();
    }
  }
}

#else 

void handleSerialInput() {
  unsigned int base_address;
  String cmd = Serial.readStringUntil(' ');
  cmd.toUpperCase();
  cmd.trim();
  if (cmd.equals("LN")) {
    // Emulate a LocoNet command of format "opcode data1 data2 checksum"
    Serial.println(F("Received LN Command"));
    LnPacket = &SerialLnPacket;
    LnPacket->data[0] = (byte) (Serial.parseInt() & 0xFF);
    LnPacket->data[1] = (byte) (Serial.parseInt() & 0xFF);
    LnPacket->data[2] = (byte) (Serial.parseInt() & 0xFF);
    LnPacket->data[3] = (byte) (Serial.parseInt() & 0xFF);
    LnPacket->sz.command = 0xB0;
    serial_ln_message = true;
    Serial.print(F("Opcode: "));
    Serial.print(LnPacket->data[0]);
    Serial.print(F(" Data1: "));
    Serial.print(LnPacket->data[1]);
    Serial.print(F(" Data2 "));
    Serial.print(LnPacket->data[2]); 
    Serial.print(F(" Chk: "));
    Serial.println(LnPacket->data[3]);
    
  } else if (cmd.equals("SD")) {
    // Write Light Data to EEPROM
    // SD <mode> [<step>] <units> <red> <green> <blue> <white> : Store light data
    //    <mode> = "D" (day) "N" (night) "SR" (sunrise) "SS" (sunset)
    //    <units> = "P" (percent) or "B" (binary - 0-255)
    //    <step> = Step number for sunrise/sunset table (1-N)
    int addr, lstep;
    Serial.println(F("Received SD Command (store light settings to EEPROM)"));
    String val = Serial.readStringUntil(' ');
    val.trim();
    val.toUpperCase();
    if (val.equals("D")) {
      addr = EEPROM_DAY_SETTINGS;
      Serial.println(F("Daylight settings"));
    } else if (val.equals("N")) {
      addr = EEPROM_NIGHT_SETTINGS;
      Serial.println(F("Night settings"));
    } else if (val.equals("SR")) {
      Serial.print(F("Sunrise settings step: "));
      lstep = Serial.parseInt();
      if (lstep > EEPROM.read(EEPROM_SUNRISE_STEPS)) {
        // Invalid value. Ignore.
        Serial.println(F("Step number too high."));
        return;
      }
      if (lstep == 0) {
        // Assume this was a mistake, and bump it to 1.
        lstep = 1;
      }
      addr = EEPROM_SUNRISE_TABLE + lstep - 1;
      Serial.println(lstep);
    } else if (val.equals("SS")) {
      Serial.print(F("Sunset settings step: "));
      lstep = Serial.parseInt();
      if (lstep > EEPROM.read(EEPROM_SUNSET_STEPS)) {
        // Invalid value. Ignore.
        Serial.println(F("Step number too high."));
        return;
      }
      if (lstep == 0) {
        // Assume this was a mistake, and bump it to 1.
        lstep = 1;
      }
      addr = EEPROM_SUNSET_TABLE + lstep - 1;
      Serial.println(lstep);
    } else {
      // Invalid mode. Ignore.
      return;
    }
    Serial.print(F("Addr = "));
    Serial.println(addr);

    String unit = Serial.readStringUntil(' ');
    unit.trim();
    unit.toUpperCase();
    
    led_t light;
    //light.red = (byte) (Serial.parseInt() & 0xFF);
    //light.green = (byte) (Serial.parseInt() & 0xFF);
    //light.blue = (byte) (Serial.parseInt() & 0xFF);
    //light.white = (byte) (Serial.parseInt() & 0xFF);
    light.red = getLEDValue(unit);
    light.green = getLEDValue(unit);
    light.blue = getLEDValue(unit);
    light.white = getLEDValue(unit);
    Serial.print(F("Light red = "));
    Serial.print(light.red);
    Serial.print(F("\tgreen = "));
    Serial.print(light.green);
    Serial.print(F("\tblue = "));
    Serial.print(light.blue);
    Serial.print(F("\twhite = "));
    Serial.println(light.white);
    writeLightDataToEEPROM(addr, &light);
    
  } else if (cmd.equals("ST")) {
    // Write number of steps for sunrise/sunset to EEPROM
    // ST <mode> <steps> <increment>: Set number of steps for sunrise or sunset
    //    <mode> = "SR" (sunrise) or "SS" (sunset)
    //    <steps> = 1-32
    //    <increment> = time in fast clock minutes between steps
    String mode = Serial.readStringUntil(' ');
    mode.trim();
    mode.toUpperCase();
    byte steps = (byte) (Serial.parseInt() & 0xFF);
    int increment = Serial.parseInt();
    Serial.print(F(" Read: steps = "));
    Serial.print(steps);
    Serial.print(F(" inc = "));
    Serial.println(increment);
    if (mode.equals("SR")) {
      EEPROM.write(EEPROM_SUNRISE_STEPS, steps);
      EEPROM.put(EEPROM_SUNRISE_INCREMENT, increment); 
    } else if (mode.equals("SS")) {
      EEPROM.write(EEPROM_SUNSET_STEPS, steps);
      EEPROM.put(EEPROM_SUNSET_INCREMENT, increment); 
    } // else do nothing.
   
  } else if (cmd.equals("TS")) {
    // Set start time of sunset or sunrise
    // TS <mode> <hours>:<mins>
    //    <mode> = "SR" (sunrise) or "SS" (sunset)
    //    <mins> <hours> = start time of transition
    String mode = Serial.readStringUntil(' ');
    mode.trim();
    mode.toUpperCase();
    Time* t = new Time();
    t->setHours(Serial.parseInt());
    t->setMins(Serial.parseInt());
    //t->setDays(0);
    if (mode.equals("SR")) {
      EEPROM.put(EEPROM_SUNRISE_START_TIME, *t);
      Serial.print(F("Sunrise:         "));
      Serial.println(t->toStringHHMM());
    } else if (mode.equals("SS")) {
      EEPROM.put(EEPROM_SUNSET_START_TIME, *t);
      Serial.print(F("Sunset:          "));
      Serial.println(t->toStringHHMM());
    }
  } else if (cmd.equals("A")) {
    // A <address> : Set base LocoNet address (and by extension all addresses) for device
    Serial.println(F("Setting address..."));
    //base_address = Serial.readString().toInt();
    base_address = Serial.parseInt();
    writeLNAddressToEEPROM(EEPROM_BASE_ADDRESS, base_address);
    Serial.print(F(" A = "));
    Serial.println(base_address);
    
  } else if (cmd.equals("P")) {
    // P <on/off> : Turn master power on or off
    Serial.print(F("Master Power: "));
    String val = Serial.readString();
    val.toUpperCase();
    val.trim();
    if (val.equals("ON")) {
      Serial.println(F("ON"));
      setPower(true);
    } else if (val.equals("OFF")) {
      Serial.println(F("OFF"));
      setPower(false);
    } else {
      Serial.print(val); Serial.println("??");
    }
    
  } else if (cmd.equals("SM")) {
    String mode = Serial.readStringUntil(' ');
    String sval = Serial.readString();
    mode.toUpperCase();
    mode.trim();
    sval.toUpperCase();
    sval.trim();
    Serial.println("Set Mode " + mode + " to " + sval + ".");
    bool val = (sval.equals("TRUE") ? true : false);
    if (mode.equals("CLOCK")) {
      setClockMode(val);
    } else if (mode.equals("DAY")) {
      setDayNightMode(true);
    } else if (mode.equals("NIGHT")) {
      setDayNightMode(false);
    } else if (mode.equals("TRANSITION")) {
      setTransitionMode(val);
    } else if (mode.equals("FOLLOW")) {
      setFollowLNPower(val);
    }
  } else if (cmd.equals("FC")) {
    int h = Serial.parseInt();
    int m = Serial.parseInt();
    Serial.print(F("Set Clock "));
    Serial.print(h);
    Serial.print(F(":"));
    Serial.print(m);
    current_time.setHours(h);
    current_time.setMins(m);
    Serial.print(F("\tNew Clock: "));
    Serial.println(current_time.toStringHHMM());
  } else if (cmd.equals("H")) {
    printHelp();
  } else if (cmd.equals("WD")) {
    writeDefaultTablesToEEPROM();
  }
}

#endif // USE_ALT_SERIAL_INPUT

void printHelp(void) {
  Serial.println(F("LnLights Serial Commands:"));
  Serial.println(F("\tLN <opcode> <data1> <data2> <checksum> : Simulate LocoNet command"));
  Serial.println(F("\tSD <mode> [<step>] <red> <green> <blue> <white> : Store light data"));
  Serial.println(F("\t\t<mode> = \"D\" (day) \"N\" (night) \"SR\" (sunrise) \"SS\" (sunset)"));
  Serial.println(F("\t\t<step> = Step number for sunrise/sunset table (1-N)"));
  Serial.println(F("ST <mode> <steps> <increment> : Set number of steps for sunrise or sunset"));
  Serial.println(F("\t\t<mode> = \"SR\" (sunrise) or \"SS\" (sunset)"));
  Serial.println(F("\t\t<steps> = 1-32"));
  Serial.println(F("\t\t<increment> = time in fast clock minutes between steps"));
  Serial.println(F("\tTS <mode> <hours>:<mins>"));
  Serial.println(F("\t\t<mode> = \"SR\" (sunrise) or \"SS\" (sunset)"));
  Serial.println(F("\t\t<hours>:<mins> = start time of transition"));
  Serial.println(F("\tA <address> : Set base LocoNet address (and by extension all addresses) for device"));
  Serial.println(F("\t\tP <val> : Master Power ON/OFF"));
  Serial.println(F("\tSM <mode> <value> : Set global mode setting"));
  Serial.println(F("\t\t<mode> = \"follow\", \"clock\", \"day\", \"night\", \"transition\""));
  Serial.println(F("\t\t<value> = \"true\", \"false\" (NOTE: <value> not specified for \"day\" or \"night\")"));
  Serial.println(F("\tFC <hours>:<mins> : Set the fast clock manually"));
  Serial.println(F("\t\t<hours>:<mins> = time to set clock"));
  Serial.println(F("\tWD : Write defaults to EEPROM (factory reset)"));
  Serial.println(F("\tH : This help message"));
  
}

void toUpper(char *s) {
  while (*s) {
    *s = toupper((unsigned char) *s);
    s++;
  }
}

int getInt(char *p, char *d, char *s = NULL) {
  char *c = strtok(p, d);
  if (c == 0) {
    if (s != 0) {
      Serial.println("Invalid input: " + String(p));
    }
    return(0);
  }
  return(atoi(c));
}

byte getLEDValue(String units) {
  int v = Serial.parseInt();
  // If the value is <= 0, just return 0
  if (v <= 0) { return(0); }
  // If the units is percent...
  if (units.equals(UNITS_PERCENT)) {
    // Make sure we aren't over 100%
    if (v > 100) { v = 100; }
    // Convert to 255, and return
    return((byte)(((v * 255) / 100) & 0xFF));
    
  } else {
    // Else limit to 255 and return
    if (v > 255) { v = 255; }
    return((byte)(v & 0xFF));
  }
}

void writeDefaultTablesToEEPROM() {
  // Write default daytime and night time values
  Serial.println(F("Writing default lighting values to EEPROM..."));
  led_t p;
  p.red = default_daytime[RED];
  p.green = default_daytime[GREEN];
  p.blue = default_daytime[BLUE];
  p.white = default_daytime[WHITE];
  writeLightDataToEEPROM(EEPROM_DAY_SETTINGS, &p);
  p.red = default_nighttime[RED];
  p.green = default_nighttime[GREEN];
  p.blue = default_nighttime[BLUE];
  p.white = default_nighttime[WHITE];
  writeLightDataToEEPROM(EEPROM_NIGHT_SETTINGS, &p);
  // Write default sunrise table
  EEPROM.write(EEPROM_SUNRISE_STEPS, DEFAULT_TABLE_LENGTH);
  EEPROM.write(EEPROM_SUNSET_STEPS, DEFAULT_TABLE_LENGTH);
  
  for (int i = 0; i < DEFAULT_TABLE_LENGTH; i++) {
    p.red = default_sunrise[i][RED];
    p.green = default_sunrise[i][GREEN];
    p.blue = default_sunrise[i][BLUE];
    p.white = default_sunrise[i][WHITE];
    writeLightDataToEEPROM(EEPROM_SUNRISE_TABLE + i*sizeof(led_t), &p);
    p.red = default_sunset[i][RED];
    p.green = default_sunset[i][GREEN];
    p.blue = default_sunset[i][BLUE];
    p.white = default_sunset[i][WHITE];
    writeLightDataToEEPROM(EEPROM_SUNSET_TABLE + i*sizeof(led_t), &p);
  }
}

