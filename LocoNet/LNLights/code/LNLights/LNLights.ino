// TODO: Broadcast address stuff is broken.  See pages 12-13 of LocoNet PE spec.

// Original code by RMWeb.co.uk member "antogar"
// http://www.rmweb.co.uk/community/index.php?/topic/92932-arduino-loconet-occupancy-detector/

// This code handles N RGBW light drivers (currently N=4 for the V1.1 board)
// LocoNet commands (0xB0 or 0xBD) can indirectly trigger light events 
// by changing mode settings (see below).
//
// Uses this library for control of the PCA9685
// https://github.com/NachtRaveVL/PCA9685-Arduino/blob/master/README.md
//
// LocoNet Addresses:
// Base Address = Master On/Off
//                TRUE = Power ON
//                FALSE = Power OFF
// Base + 1     = Clock / Static mode
//                TRUE = Use Fast Clock
//                FALSE = Static clock mode
// Base + 2     = Master Power follows LocoNet Global Power
//                TRUE = Follow LocoNet Power Messages
//                FALSE = Ignore LocoNet Power Messages
// Base + 3     = Fixed Day / Night
//                TRUE = Static DAY mode
//                FALSE = Static NIGHT mode
// Base + 4     = Direct toggle vs. sunrise/sunset transition
//                TRUE = Use Transition between Day/Night
//                FALSE = Do not Transition between Day/Night
// Base + 5-8   = Begin/End storm mode (per zone)
//                TRUE = Start the storm
//                FALSE = End the storm
// 
// Serial Port Commands:
// LN <opcode> <data1> <data2> <checksum> : Simulate LocoNet command
// SD <mode> [<step>] <red> <green> <blue> <white> : Store light data
//    <mode> = "D" (day) "N" (night) "SR" (sunrise) "SS" (sunset)
//    <step> = Step number for sunrise/sunset table (1-N)
// ST <mode> <steps> <increment> : Set number of steps for sunrise or sunset
//    <mode> = "SR" (sunrise) or "SS" (sunset)
//    <steps> = 1-32
//    <increment> = time in fast clock minutes between steps
// TS <mode> <hours>:<mins>
//    <mode> = "SR" (sunrise) or "SS" (sunset)
//    <hours>:<mins> = start time of transition
// A <address> : Set base LocoNet address (and by extension all addresses) for device
// P <val> : Master Power ON/OFF
// SM <mode> <value> : Set global mode setting
//    <mode> = "follow", "clock", "day", "night", "transition"
//    <value> = "true", "false" (NOTE: <value> not specified for "day" or "night")
// FC <hours>:<mins> : Set the fast clock manually
//    <hours>:<mins> = time to set clock
//
// *** LocoNet IO Pinouts ***
//
// For Arduino MEGA, RX = Pin 48 (ICP5)
// For Arduino Uno, RX = Pin 8 (ICP1) 
// For Arduino ProMini, RX = Pin 8 (ICP1)
// We assume TX = Pin 6 on all boards.
//
// PCA9685 channel assignments:
// * PWM0  = Zone 1 Red
// * PWM1  = Zone 1 Green
// * PWM2  = Zone 1 Blue
// * PWM3  = Zone 1 White
//
// * PWM4  = Zone 4 Red
// * PWM5  = Zone 4 Green
// * PWM6  = Zone 4 Blue
// * PWM7  = Zone 4 White
//
// * PWM8  = Zone 3 Red
// * PWM9  = Zone 3 Green
// * PWM10 = Zone 3 Blue
// * PWM11 = Zone 3 White
//
// * PWM12 = Zone 2 Red
// * PWM13 = Zone 2 Green
// * PWM14 = Zone 2 Blue
// * PWM15 = Zone 2 White

#define LN_TX_PIN 6

// Set this to allow sending LocoNet commands over the serial interface.
// Otherwise the "LN" Serial command will do nothing.
#define SERIAL_EMULATE_LN 0

#include <LocoNet.h>
#include <EEPROM.h>
#include <Wire.h>
#include <PCA9685.h>

#define LNLIGHTS_INO 1
#define GLOBALS_GO_HERE 1
#include "pinout.h"
#include "config.h"
#include "Serial.hpp"
#include "LocoNet.hpp"
#include "eeprom.hpp"
#include "eemap.h"
#include "globals.h"
//#include "lnaddr.h"
#include "Time.hpp"
#include "colors.h"

// PCA9685 devoce
PCA9685 pwmController(Wire, PCA9685_PhaseBalancer_Weaved);

// Storm mode settings
#define MAX_BETWEEN 2579 // Max milliseconds between flashes
#define MAX_DURATION 43  // Max duration of a flash
#define MAX_SUBFLASHES 7       // Max subflashes within a flash
#define LIGHTNING_FLASH_RED_PWM 0
#define LIGHTNING_FLASH_GREEN_PWM 0
#define LIGHTNING_FLASH_BLUE_PWM 0
#define LIGHTNING_FLASH_WHITE_PWM 255
#define LIGHTNING_FLASH_DARK_PWM 0

// Static mode transition settings
#define TRANSITION_INTERVAL_SEC 2
bool transitionActive;
unsigned long nextTransition;
int transitionStep;

// Last EEPROM address used to update lighting
// Used to keep from unnecessarily updating the PWM generator.
int mostRecentAddress;

// RTC information
// NOTE: These are real (wall) clock times, not fast clock times
#define CLOCK_TIME_UPDATE_MSECS 10000
#define MILLIS_PER_MIN 60000
unsigned long lastTimeUpdate;

/** configurePins()
 * 
 * Set all the pinouts and Servos up for operation
 */
void configurePins() {
  // Set the current monitor input pin.
  // imonitor = IMONITOR_PIN;
  pinMode(12, INPUT);
  pinMode(13, INPUT);
  pinMode(GPIO1_PIN, OUTPUT); // This will be the power ON signal to the PSU
  pinMode(GPIO2_PIN, INPUT_PULLUP); // This pin is (for now) unused.
  digitalWrite(PSU_CONTROL, PSU_OFF); // Default to master power OFF
}

void setup()
{
  // LocoNet Addresses are 12 bits so stored as 2 bytes
  //decoder_cv_address = getDecoderCVAddressFromEEPROM();

  // Configure the pins here.
  configurePins();

  // DON'T DO THIS. Use the "WD" command over the serial interface instead.
  //loadDefaultSunrise();
  //loadDefaultSunset();

  // Configure the serial port for 57600 baud
  Serial.begin(57600);
  
  // Initialize LocoNet interface. Note that with the official Arduino library
  // you can't choose the TX pin.
  LocoNet.init(LN_TX_PIN);
  
  // Initialize a LocoNet packet buffer to buffer bytes from the PC 
  initLnBuf(&LnTxBuffer) ;
  serial_ln_message = false;

  // Set up the PCA9685 and initialize the Lights.
  Wire.begin();
  Wire.setClock(400000);

  Serial.println(F("Initializing PWM Controller"));
  pwmController.resetDevices();
  pwmController.init(B000000);
  pwmController.setPWMFrequency(1024);
  // TODO: see if you can merge this code into writeLightDataToPWM()
  
  transitionActive = false;

  base_address = readLNAddressFromEEPROM(EEPROM_BASE_ADDRESS);
  readGlobalStateFromEEPROM();
  for (int i = 0; i < NUM_LIGHTS; i++) {
    if (day_night == true) {
      readLightDataFromEEPROM(EEPROM_DAY_SETTINGS, lights+i);
    } else {
      readLightDataFromEEPROM(EEPROM_NIGHT_SETTINGS, lights+i);
    }
    writeLightDataToPWM(i);
    //stormState[i].enabled=false;
    //stormState[i].flashState=0;
  }

  setDefaultFastClock();

  // Print information to the Serial port
  Serial.println(F("LN Light Controller"));
  printGlobalState();
  for (int i = 0; i < NUM_LIGHTS; i++) {
    printLightInfo(i);
  }

  Serial.println("----");
  calcAddressFromCurrentTime();
  //Serial.flush();
}

void printGlobalState(void) {
  Serial.print(F("base address"));
  Serial.print(F("\t"));
  Serial.println(base_address);
  Serial.print(F("Clock mode:      "));
  Serial.println(String((clock_mode ? "Fast Clock" : "Static")));
  Serial.print(F("Day/Night:       "));
  Serial.println(String((day_night ? "Day" : "Night")));
  Serial.print(F("Transition:      "));
  Serial.println(String((transition_mode ? "Yes" : "No")));
  Serial.print(F("Master Power:    "));
  Serial.println(String((master_on ? "ON" : "OFF")));
  Serial.print(F("Follow LN Power: "));
  Serial.println(String((follow_ln ? "Follow" : "No Follow")));
  Time sunrise = readTimeFromEEPROM(EEPROM_SUNRISE_START_TIME);
  Time sunset = readTimeFromEEPROM(EEPROM_SUNSET_START_TIME);
  Serial.print(F("Sunrise:         "));
  Serial.println(sunrise.toStringHHMM());
  Serial.print(F("Sunset:          "));
  Serial.println(sunset.toStringHHMM());
}

void printLightInfo(int i) {
  Serial.print((F("Light #")));
  Serial.print(i+1);
  Serial.print(F(" R: "));
  Serial.print(lights[i].red);
  Serial.print(F("\tG: "));
  Serial.print(lights[i].green);
  Serial.print(F("\tB: "));
  Serial.print(lights[i].blue);
  Serial.print(F("\tW: "));
  Serial.println(lights[i].white);
}

void printTimeSettings(void) {
  
}

void setDefaultFastClock(void) {
  fast_clock.rate = 60;
  fast_clock.frac = 0;
  fast_clock.mins = 40;
  fast_clock.hours = 18;
  fast_clock.days = 0;
  updateCurrentTime(true);
}

//-------------------------------------------------------
// Main program loop

void loop()
{ 
  // Check for serial traffic
  if (Serial.available()) {
    handleSerialInput();
  }

  // Handle the LocoNet interface 
#if (SERIAL_EMULATE_LN == 0)
  // Only do this if using the hardware LocoNet interface
  LnPacket = LocoNet.receive() ;
#endif
  if (LnPacket) {
    handleLocoNetInterface(LnPacket);
  }

  // If this was a serial LocoNet message, clean up the mess.
  if (serial_ln_message == true) {
    LnPacket = NULL;
    serial_ln_message = false;
  }

  // Update the RTC
  updateCurrentTime(false);

  // Handle storm mode if enabled in any Zone.
  //handleStormMode();

  // Handle transition mode if we are in the midst of one
  handleTransition();

} // end loop()

//-------------------------------------------------------
// Methods for responding to LocoNet state changes

void setPower(bool mode) {
  // Update the lighting state
  master_on = mode;
  updateLightingState();
  digitalWrite(PSU_CONTROL, master_on ? PSU_ON : PSU_OFF);
  writeGlobalStateToEEPROM();
}

void setFollowLNPower(bool mode) {
  follow_ln = mode;
  writeGlobalStateToEEPROM();
}

void setClockMode(bool mode) {
  // Store the new state and update the lighting
  clock_mode = mode;
  updateLightingState();
  writeGlobalStateToEEPROM();
}

void setDayNightMode(bool mode) {
  // Activate transition if needed
  if ((day_night != mode) || true) {
    // Store the new state
    day_night = mode;
    writeGlobalStateToEEPROM();
    Serial.println("Static Day/Night = " + String((day_night ? "DAY" : "NIGHT")));

    // If we are in static mode, update the lighting
    if (clock_mode == CLOCK_MODE_STATIC) {
      // Either start a transition or update the lighting state immediately
      if (transition_mode == TRANSITION_YES) {
        transitionActive = true;
        transitionStep = 0;
        nextTransition = millis() + 1000*TRANSITION_INTERVAL_SEC;
      } else {
        updateLightingState();
      }
    }
  } else {
    Serial.println(F("No net change to static mode. Ignoring."));
  }
}

void setTransitionMode(bool mode) {
  // Store the new state. No action required
  transition_mode = mode;
  writeGlobalStateToEEPROM();
}

void setStormMode(int zone, bool thrown) {

  //stormState[zone].enabled = thrown;
  //stormState[zone].flashState = 0;
  
  if (thrown == STORM_MODE_OFF) {
    // Return lighting to normal settings
    updateLightingState();
  }
}

int calcAddressFromCurrentTime() {
  byte sunrise_steps, sunset_steps;
  int sunrise_increment, sunset_increment;
  sunrise_steps = EEPROM.read(EEPROM_SUNRISE_STEPS);
  sunset_steps = EEPROM.read(EEPROM_SUNSET_STEPS);
  EEPROM.get(EEPROM_SUNRISE_INCREMENT, sunrise_increment);
  EEPROM.get(EEPROM_SUNSET_INCREMENT, sunset_increment);
  //Serial.print(F("SR steps = "));
  //Serial.print(sunrise_steps);
  //Serial.print(F(" increment = "));
  //Serial.println(sunrise_increment);
  //Serial.print(F("SS steps = "));
  //Serial.print(sunset_steps);
  //Serial.print(F(" increment = "));
  //Serial.println(sunset_increment);

  // Find the beginning and end of sunrise and sunset
  Time sunset_start = readTimeFromEEPROM(EEPROM_SUNSET_START_TIME);
  Time sunrise_start = readTimeFromEEPROM(EEPROM_SUNRISE_START_TIME);
  // Force the day to be the same so the comparisons will work.
  sunset_start.setDays(current_time.days());
  sunrise_start.setDays(current_time.days());

  Time* sunset_end = new Time(sunset_start);
  sunset_end->addMinutes(sunset_increment * sunset_steps);
  Time* sunrise_end = new Time(sunrise_start);
  sunrise_end->addMinutes(sunrise_increment * sunrise_steps);
  Serial.print("SR Start = " + sunrise_start.toStringHHMM());
  Serial.print(" SR End = " + sunrise_end->toStringHHMM());
  Serial.print(" SS Start = " + sunset_start.toStringHHMM());
  Serial.println(" SS End = " + sunset_end->toStringHHMM());
  Serial.flush();
  //return(EEPROM_DAY_SETTINGS);

  Serial.print(F("Current Time: "));
  Serial.println(current_time.toStringHHMM());
  // Figure out which day state we are in and act accordingly
  if ((current_time >= sunset_start) && (current_time <= *sunset_end)) {
    // Sunset
    Serial.print(F("Sunset "));
    Serial.print(sunset_start.toStringHHMM());
    Serial.print(F(" "));
    Serial.println(sunset_end->toStringHHMM());
    int steps_after = current_time.minutesAfter(sunset_start) / sunset_increment;
    if (steps_after >= sunset_steps) { 
      Serial.println(F("Night")); 
      return(EEPROM_NIGHT_SETTINGS); 
    }
    return(EEPROM_SUNSET_TABLE + (steps_after * sizeof(led_t)));

  } else if ((current_time >= sunrise_start) && (current_time <= *sunrise_end)) {
    // Sunrise
    Serial.print(F("Sunrise "));
    Serial.print(sunrise_start.toStringHHMM());
    Serial.print(F(" "));
    Serial.println(sunrise_end->toStringHHMM());
    int steps_after = current_time.minutesAfter(sunrise_start) / sunrise_increment;
    if (steps_after >= sunrise_steps) { 
      Serial.println(F("Day")); 
      return(EEPROM_DAY_SETTINGS); 
    }
    return(EEPROM_SUNRISE_TABLE + (steps_after * sizeof(led_t)));
    
  } else if ((current_time < sunrise_start) || (current_time > *sunset_end)) {
    // Night
    Serial.println(F("Night"));
    return(EEPROM_NIGHT_SETTINGS);
      
  } else {
    // Day
    Serial.println(F("Day"));
    return(EEPROM_DAY_SETTINGS);
  }
}

void updateLightingState(void) {
  int addr;
  Serial.println(F("updateLightingState()"));
  if (master_on == false ) {
    for (int i = 0; i < NUM_LIGHTS; i++) {
      pwmController.setChannelPWM(i*4+RED_CHANNEL, 0);
      pwmController.setChannelPWM(i*4+GREEN_CHANNEL, 0);
      pwmController.setChannelPWM(i*4+BLUE_CHANNEL, 0);
      pwmController.setChannelPWM(i*4+WHITE_CHANNEL, 0);
    }
    return;
  }
  if (clock_mode == CLOCK_MODE_STATIC) {
    // We are in static mode.  Force either Day or Night
    if (day_night == STATIC_DAY) {
      addr = EEPROM_DAY_SETTINGS;
      Serial.println(F("Static Daytime"));
    } else {
      addr = EEPROM_NIGHT_SETTINGS;
      Serial.println(F("Static Nighttime"));
    }
  } else {
    // We are in Fast Clock mode. Must infer from current time what
    // lighting state to load.
    addr = calcAddressFromCurrentTime();
    //addr = EEPROM_DAY_SETTINGS;
    Serial.print(F("Fast Clock "));
    Serial.print(current_time.toStringHHMM());
    Serial.print(F(" addr="));
    Serial.println(addr);
    Serial.flush();
  }

  if (addr == mostRecentAddress) {
    // Same address as last update. Don't bother updating.
    Serial.println("No change. Skipping update.");
    return;
  } else {
    // Update the most recent address cache.
    mostRecentAddress = addr;
  }

  //Serial.println(F("Setting the lights..."));
  // Update the current light settings.
  for (int i = 0; i < NUM_LIGHTS; i++) {
    // Only update the lighting if this Zone is not in storm mode.
    // If in storm mode, handleStormMode() will do the work.
    //if (!stormState[i].enabled) {
    if (true) {
      // Update the PWM generators.
      readLightDataFromEEPROM(addr, lights+i);
      writeLightDataToPWM(i);
      if (i==0) {
        Serial.print(F("Update: r="));
        Serial.print(lights[i].red);
        Serial.print(F(" g="));
        Serial.print(lights[i].green);
        Serial.print(F(" b="));
        Serial.print(lights[i].blue);
        Serial.print(F(" w="));
        Serial.println(lights[i].white);
      }
    }
  }
  Serial.flush();

}

void writeLightDataToPWM(int i) {
  //Serial.println("Setting pwm " + String(i*4+RED_CHANNEL) + " (red) to " + String(lights[i].red << 4));
  pwmController.setChannelPWM(i*4+RED_CHANNEL, lights[i].red << 4);
  //Serial.println("Set to: " + String(pwmController.getChannelPWM(i*4+0)));
  //Serial.println("Setting pwm " + String(i*4+GREEN_CHANNEL) + " (green) to " + String(lights[i].green << 4));
  pwmController.setChannelPWM(i*4+GREEN_CHANNEL, lights[i].green << 4);
  //Serial.println("Set to: " + String(pwmController.getChannelPWM(i*4+0)));
  //Serial.println("Setting pwm " + String(i*4+BLUE_CHANNEL) + " (blue) to " + String(lights[i].blue << 4));
  pwmController.setChannelPWM(i*4+BLUE_CHANNEL, lights[i].blue << 4);
  //Serial.println("Set to: " + String(pwmController.getChannelPWM(i*4+0)));
  //Serial.println("Setting pwm " + String(i*4+WHITE_CHANNEL) + " (white) to " + String(lights[i].white << 4));
  pwmController.setChannelPWM(i*4+WHITE_CHANNEL, lights[i].white << 4);
  //Serial.println("Set to: " + String(pwmController.getChannelPWM(i*4+0)));
}

/*
// handleStormMode()
// Reference: https://forum.arduino.cc/index.php?topic=105030.0
void handleStormMode() {
  // FOR EACH LIGHT Zone
  for (int i = 0; i < NUM_LIGHTS; i++) {
    storm_state_t* storm = stormState+i;
    // If storm mode is enabled, then do the storm
    if (storm->enabled) {
      // If it's time for the next lightning flash...
      if ((millis() - storm->flashWaitTime) > storm->lastFlashTime) {
        // Update the flash time
        storm->lastFlashTime += storm->flashWaitTime;
        storm->flashWaitTime= random(MAX_BETWEEN);
        // Do the sub-flashes
        for (int j=0; j < random(MAX_SUBFLASHES); j++) {
          // Turn on the LEDs...
          if (storm->flashState == 0) {
            pwmController.setChannelPWM(i*4+RED_CHANNEL, LIGHTNING_FLASH_RED_PWM);
            pwmController.setChannelPWM(i*4+GREEN_CHANNEL, LIGHTNING_FLASH_GREEN_PWM);
            pwmController.setChannelPWM(i*4+BLUE_CHANNEL, LIGHTNING_FLASH_BLUE_PWM);
            pwmController.setChannelPWM(i*4+WHITE_CHANNEL, LIGHTNING_FLASH_WHITE_PWM);
            storm->flashState= 1;
            storm->nextFlashTime = millis() + 20 + random(MAX_DURATION);
          }
          // Turn off the LEDs
          if (storm->flashState == 1 && millis() >= storm->nextFlashTime) {
            pwmController.setChannelPWM(i*4+RED_CHANNEL, LIGHTNING_FLASH_DARK_PWM);
            pwmController.setChannelPWM(i*4+GREEN_CHANNEL, LIGHTNING_FLASH_DARK_PWM);
            pwmController.setChannelPWM(i*4+BLUE_CHANNEL, LIGHTNING_FLASH_DARK_PWM);
            pwmController.setChannelPWM(i*4+WHITE_CHANNEL, LIGHTNING_FLASH_DARK_PWM);
            storm->flashState = 2;
            storm->nextFlashTime = millis() + 10;
          }
          // Pause 10ms before the next sub-flash
          if (storm->flashState = 2 && millis >= storm->nextFlashTime) {
            storm->flashState = 0;
          }
        } // Sub-flash loop
        
      } // flash time loop/if
    } // if Storm mode enabled
  } // Zone loop
} // handleStormMode()
*/
void handleTransition(void) {
  // Only do this if we're in the middle of a transition.
  if ((transitionActive) && (millis() >= nextTransition)) {
    int addr, steps;

    // Figure out which table to look at and how many steps are in the table.
    if (day_night == STATIC_DAY) {
      Serial.println("Using sunrise table");
      addr = EEPROM_SUNRISE_TABLE + transitionStep*sizeof(led_t);
      steps = EEPROM.read(EEPROM_SUNRISE_STEPS);
    } else {
      Serial.println("Using sunset table");
      addr = EEPROM_SUNSET_TABLE + transitionStep*sizeof(led_t);
      steps = EEPROM.read(EEPROM_SUNSET_STEPS);
    }

    // Update the lights for the current step of the current table.
    for (int i =0; i < NUM_LIGHTS; i++) {
      //readLightDataFromEEPROM(addr, lights+i);
      if (day_night == STATIC_DAY) {
        lights[i].red = default_sunrise[transitionStep][0];
        lights[i].green = default_sunrise[transitionStep][1];
        lights[i].blue = default_sunrise[transitionStep][2];
        lights[i].white = default_sunrise[transitionStep][3];
      } else {
        lights[i].red = default_sunset[transitionStep][0];
        lights[i].green = default_sunset[transitionStep][1];
        lights[i].blue = default_sunset[transitionStep][2];
        lights[i].white = default_sunset[transitionStep][3];
      }
      if (i==0) {
        Serial.print(F("Transition ("));
        Serial.print(transitionStep);
        Serial.print(F(") : r="));
        Serial.print(lights[i].red);
        Serial.print(F(" g="));
        Serial.print(lights[i].green);
        Serial.print(F(" b="));
        Serial.print(lights[i].blue);
        Serial.print(F(" w="));
        Serial.println(lights[i].white);
      }
      writeLightDataToPWM(i);
    }

    // Prepare for the next step.
    transitionStep++;
    nextTransition = millis() + 1000*TRANSITION_INTERVAL_SEC;
    
    // If we've reached the end of the table, we're done.
    // Turn off the transition and update lighting state for day/night
    if (transitionStep == steps) {
      transitionStep = 0;
      transitionActive = false;
      updateLightingState();
    }
    
  } // If active
}

void loadDefaultSunrise(void) {
  led_t p;
  EEPROM.write(EEPROM_SUNRISE_STEPS, DEFAULT_TABLE_LENGTH);
  for (int i = 0; i < DEFAULT_TABLE_LENGTH; i++) {
    p.red = default_sunrise[i][0];
    p.green = default_sunrise[i][1];
    p.blue = default_sunrise[i][2];
    p.white = default_sunrise[i][3];
    writeLightDataToEEPROM(EEPROM_SUNRISE_TABLE + (i*sizeof(led_t)), &p);
  }
}

void loadDefaultSunset(void) {
  led_t p;
  EEPROM.write(EEPROM_SUNSET_STEPS, DEFAULT_TABLE_LENGTH);
  for (int i = 0; i < DEFAULT_TABLE_LENGTH; i++) {
    p.red = default_sunset[i][0];
    p.green = default_sunset[i][1];
    p.blue = default_sunset[i][2];
    p.white = default_sunset[i][3];
    writeLightDataToEEPROM(EEPROM_SUNSET_TABLE + (i*sizeof(led_t)), &p);
  }
}

void updateCurrentTime(bool sync) {
  if (clock_mode != CLOCK_MODE_FASTCLOCK) {
    return;
  }
  if (sync) {
    // Sync pulse received from LocoNet.  Force time to match.
    Serial.println(F("Forcing Clock Sync"));
    current_time.setDays(fast_clock.days);
    current_time.setHours(fast_clock.hours);
    current_time.setMins(fast_clock.mins);
    updateLightingState();
  } else {
    // Regular update.
    unsigned long now = millis();
    if (now > (lastTimeUpdate + CLOCK_TIME_UPDATE_MSECS)) {
      // Get minutes changed, multiplied by fast clock rate. Note that for
      // intervals less than 1 minute, this will not update at all.  Will
      // also drop fractional minutes (i.e. 1.5 minutes will only update 1 minute).
      // That's OK because we don't need to be that precise in this operation.
      unsigned long diff_mins = (fast_clock.rate * (now - lastTimeUpdate)) / MILLIS_PER_MIN;
      // Do a little sanity checking before updating the time.
      if (diff_mins > 32767) {
        diff_mins = 32767;
      }
      int dm = (int) diff_mins & 0xFFFF;
      Serial.print(F("Adding "));
      Serial.print(dm);
      Serial.println(F(" minutes to clock"));
      current_time.addMinutes(dm);
      updateLightingState();
      //Serial.print(".");
      lastTimeUpdate = now;
    }
  }
  Serial.flush();
}

