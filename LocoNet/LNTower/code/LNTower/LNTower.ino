// TODO: Broadcast address stuff is broken.  See pages 12-13 of LocoNet PE spec.

// Original code by RMWeb.co.uk member "antogar"
// http://www.rmweb.co.uk/community/index.php?/topic/92932-arduino-loconet-occupancy-detector/

// This code supports the RR-CirKits BOD-4 o BOD-8 coil block detectors.
//
// LocoNet Addresses:
// * Sensor 1 = Base address
// * Sensor 2 = Base + 1
// * Sensor 3 = Base + 2
// * Sensor 4 = Base + 3
// * Sensor 5 = Base + 4
// * Sensor 6 = Base + 5
// * Sensor 7 = Base + 6
// * Sensor 8 = Base + 7
// 
// Serial Port Commands:
// LN <opcode> <data1> <data2> <checksum> : Simulate LocoNet command
// E <sensor> <enable> : Enable (1) or Disable (0) a sensor
// S : Store settings to EEPROM
// A <address> : Set base address for device

// SENSOR input convention
// TODO: Check this against actual BOD specifications
// BOD Output HIGH == Arduino input HIGH == Sensor Inactive == Track unoccupied
// BOD Output LOW == Arduino input LOW == Sensor Active == Track occupied

#define BOARD_REV 1.0
#define BOARD_REV_MIN_LED_SUPPORT 1.0

#define SERIAL_EMULATE_LN 0

#include <LocoNet.h>
#include <EEPROM.h>

#define LNTOWER_INO 1
#define GLOBALS_GO_HERE 1
#define FLIPPED_BOD4_PINOUT 1
#include "pinout.h"
#include "config.h"
#include "LocoNet.hpp"
#include "eeprom.hpp"
#include "eemap.h"

extern void handleSerialInput();
extern void printSettings();
extern void printSensor(int i);

unsigned long currentmillis;
unsigned long lasttimerpop;
unsigned long lastsensorcheck;

// Pushbutton and LED stuff
int pushbutton;
int i;

int lastsensorstate[NUM_SENSORS];
int lastlastsensorstate[NUM_SENSORS];

int sensor_pin[NUM_SENSORS];
//int led_pin[NUM_SENSORS];
//int drive_pin;



void assignPinouts() {
  sensor_pin[0] = SENSOR1_PIN;
  sensor_pin[1] = SENSOR2_PIN;
  sensor_pin[2] = SENSOR3_PIN;
  sensor_pin[3] = SENSOR4_PIN;
//  sensor_pin[4] = SENSOR5_PIN;
//  sensor_pin[5] = SENSOR6_PIN;
//  sensor_pin[6] = SENSOR7_PIN;
//  sensor_pin[7] = SENSOR8_PIN;
//  led_pin[0] = LED1_PIN;
//  led_pin[1] = LED2_PIN;
//  led_pin[2] = LED3_PIN;
//  led_pin[3] = LED4_PIN;
//  led_pin[4] = LED5_PIN;
//  led_pin[5] = LED6_PIN;
//  led_pin[6] = LED7_PIN;
//  led_pin[7] = LED8_PIN;
//  drive_pin = DRIVE_PIN;
}

void setup()
{
  assignPinouts();

  currentmillis = millis();
  lasttimerpop = currentmillis;
  lastsensorcheck = currentmillis;
  
  // Set up LED and Pushbutton pins
//  pinMode(led, OUTPUT);
//  digitalWrite(led, HIGH); // High is OFF
  
//  pinMode(button, INPUT);
//  digitalWrite(button, HIGH);

//  pinMode(drive_pin, OUTPUT);
//  digitalWrite(drive_pin, HIGH);

  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(sensor_pin[i], INPUT);
    sensor_enable[i] = 1;
//    if (BOARD_REV >= BOARD_REV_MIN_LED_SUPPORT) {
//      pinMode(led_pin[i], OUTPUT);
//      digitalWrite(led_pin[i], HIGH);
//    }
  }


  // LocoNet Addresses are 12 bits so stored as 2 bytes
  base_address = getAddressFromEEPROM(EEPROM_BASE_ADDRESS);
  broadcast = getAddressFromEEPROM(EEPROM_BC_ADDRESS);
  for (int i = 0; i < NUM_SENSORS; i++) {
    sensor_addr[i] = base_address + i;
  }
  // Configure the serial port for 57600 baud
  Serial.begin(57600);
  
  // Initialize LocoNet interface. Note that with the official Arduino library
  // you can't choose the TX pin.
  LocoNet.init();
  
  // Initialize a LocoNet packet buffer to buffer bytes from the PC 
  initLnBuf(&LnTxBuffer) ;

  Serial.println("LN Tower Sensor Board");
  printSettings();
  Serial.println();
}

void loop()
{ 
  // Check for serial traffic
  if (Serial.available()) {
    handleSerialInput();
  }
   
#if (SERIAL_EMULATE_LN == 0)
  LnPacket = LocoNet.receive() ;
#endif
  if (LnPacket) {
    handleLocoNetInterface(LnPacket);
  }

  // Check sensor states
  handleSensors();

} // end loop()


void handleSensors() {
  currentmillis = millis();
  if ((currentmillis > lastsensorcheck) && ((currentmillis - lastsensorcheck) > SENSOR_CHECK_INTERVAL_MS)) {
    for (int i = 0; i < NUM_SENSORS; i++) {
      sensorstate[i] = digitalRead(sensor_pin[i]);
      if ((sensorstate[i] == lastsensorstate[i]) && (sensorstate[i] != lastlastsensorstate[i])) {
        // We've had a change that has been stable for two check intervals.  Inform the LocoNet
        // But only if we are "enabled"
        Serial.print("Sensor ");
        Serial.print(i+1);
        Serial.print(" detected " );
        Serial.print(sensorstate[i] == 1 ? "HIGH" : "LOW");
        Serial.println(" (" + String((sensorstate[i] == SENSOR_ACTIVE ? "ACTIVE)" : "INACTIVE)")));
        for (int i = 0; i < NUM_SENSORS; i++) {
          printSensor(i);
        }
        if (sensor_enable[i] == SENSOR_ENABLED) {
          // Report ACTIVE=TRUE regardless of pin voltage (defined in config.h)
          reportSensorState(i, sensorstate[i] == SENSOR_ACTIVE);
          //setSensorLED(i, sensorstate[i] == SENSOR_ACTIVE);
        } else {
          Serial.println(F("Sensor disabled. Not sending LocoNet message."));
        }
      }
      // Regardless, update the sensor state record
      lastlastsensorstate[i] = lastsensorstate[i];
      lastsensorstate[i] = sensorstate[i];
    }
  }
}

//void setSensorLED(int sensor, bool state) {
//  if (BOARD_REV >= BOARD_REV_MIN_LED_SUPPORT) {
//    digitalWrite(led_pin[sensor], state ? LED_ACTIVE : LED_INACTIVE);
//  }
//}

