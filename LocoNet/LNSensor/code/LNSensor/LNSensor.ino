// TODO: Broadcast address stuff is broken.  See pages 12-13 of LocoNet PE spec.

// Original code by RMWeb.co.uk member "antogar"
// http://www.rmweb.co.uk/community/index.php?/topic/92932-arduino-loconet-occupancy-detector/

// This code is designed to be common amongst the following boards:
//  * LN_IR
//  * LN_Tower
//
// Board selection is in config.h

// This code supports up to 6 infrared sensors with onboard LEDs and LocoNet outputs.
//
// LocoNet Addresses:
// * Sensor 1 = Base address
// * Sensor 2 = Base + 1
// * Sensor 3 = Base + 2
// * Sensor 4 = Base + 3
// * Sensor 5 = Base + 4
// * Sensor 6 = Base + 5
// * ... and so on.
// 
// NOTE: If using Arduino Nano with LNIR/promini_rj v1.1 board, Sensor 2 is a NC (will not work)
//
// Serial Port Commands:
// LN <opcode> <data1> <data2> <checksum> : Simulate LocoNet command
// E <sensor> <enable> : Enable (1) or Disable (0) a sensor
// W : Store settings to EEPROM
// A <address> : Set base address for device
// S : Print status/settings
// H : Print help

// LNIR SENSOR input convention
// Sensor HIGH == Arduino input LOW == Sensor Inactive == Track unoccupied
// Sensor LOW == Arduino input HIGH == Sensor Active == Track occupied

// LNTOWER SENSOR input convention
// Sensor HIGH == Arduino input HIGH == Sensor Inactive == Track unoccupied
// Sensor LOW == Arduino input LOW == Sensor Active == Track occupied

// LED output convention
// Arduino output LOW == LED ON == Sensor Active == Track occupied
// Arduino output HIGH == LED OFF == Sensor Inactive == Track unoccupied

#define BOARD_REV 1.1
#define BOARD_REV_MIN_LED_SUPPORT 1.1

#define SERIAL_EMULATE_LN 0

#include <LocoNet.h>
#include <EEPROM.h>

#define LNSENSOR_INO 1
#define GLOBALS_GO_HERE 1
//#include "pinout.h"
#include "config.h"
#include "pinout.h"
#include "LocoNet.hpp"
#include "eeprom.hpp"
#include "eemap.h"

extern void printSettings();
extern void handleSerialInput();

unsigned long currentmillis;
unsigned long lasttimerpop;
unsigned long lastsensorcheck;

// Pushbutton and LED stuff
int pushbutton;
int i;

int lastsensorstate[NUM_SENSORS];
int lastlastsensorstate[NUM_SENSORS];

int sensor_pin[NUM_SENSORS];
int led_pin[NUM_SENSORS];
int drive_pin;



void assignPinouts() {
  sensor_pin[0] = SENSOR1_PIN;
  sensor_pin[1] = SENSOR2_PIN;
  sensor_pin[2] = SENSOR3_PIN;
  sensor_pin[3] = SENSOR4_PIN;
  sensor_pin[4] = SENSOR5_PIN;
  sensor_pin[5] = SENSOR6_PIN;
#if (BOARD_ID == BOARD_ID_LNTOWER)
  sensor_pin[6] = SENSOR7_PIN;
  sensor_pin[7] = SENSOR8_PIN;
#else
  led_pin[0] = LED1_PIN;
  led_pin[1] = LED2_PIN;
  led_pin[2] = LED3_PIN;
  led_pin[3] = LED4_PIN;
  led_pin[4] = LED5_PIN;
  led_pin[5] = LED6_PIN;
  drive_pin = DRIVE_PIN;
#endif
}

void setup()
{
  assignPinouts();

  currentmillis = millis();
  lasttimerpop = currentmillis;
  lastsensorcheck = currentmillis;
  
  // Set up LED and Pushbutton pins
#if (BOARD_ID == BOARD_ID_LNIR)
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH); // Hight is OFF
  
  pinMode(button, INPUT);
  digitalWrite(button, HIGH);

  pinMode(drive_pin, OUTPUT);
  digitalWrite(drive_pin, HIGH);
#endif

  // LocoNet Addresses are 12 bits so stored as 2 bytes
  base_address = getAddressFromEEPROM(EEPROM_BASE_ADDRESS);
  broadcast = getAddressFromEEPROM(EEPROM_BC_ADDRESS);

  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(sensor_pin[i], INPUT_PULLUP);
    sensor_enable[i] = 1;
    sensor_addr[i] = base_address + i;
    if (BOARD_REV >= BOARD_REV_MIN_LED_SUPPORT) {
      pinMode(led_pin[i], OUTPUT);
      digitalWrite(led_pin[i], HIGH);
    }
  }

  // Configure the serial port for 57600 baud
  Serial.begin(57600);
  
  // Initialize LocoNet interface. Note that with the official Arduino library
  // you can't choose the TX pin.
  LocoNet.init();
  
  // Initialize a LocoNet packet buffer to buffer bytes from the PC 
  initLnBuf(&LnTxBuffer) ;

  Serial.println("LNSensor IR Detector Board");
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

  // Check external buttons
  //handleExternalButtons();

  // Check sensor states
  handleSensors();

} // end loop()

void handleSensors() {
  currentmillis = millis();
  if ((currentmillis > lastsensorcheck) && ((currentmillis - lastsensorcheck) > SENSOR_CHECK_INTERVAL_MS)) {
    for (int i = 0; i < NUM_SENSORS; i++) {
      sensorstate[i] = digitalRead(sensor_pin[i]);
      //int x = analogRead(sensor_pin[i]);
      //sensorstate[i] = (x > 768 ? 1 : 0);
      if ((sensorstate[i] == lastsensorstate[i]) && (sensorstate[i] != lastlastsensorstate[i])) {
        // We've had a change that has been stable for two check intervals.  Inform the LocoNet
        // But only if we are "enabled"
        Serial.print("Sensor ");
        Serial.print(i+1);
        Serial.print(" detected " );
        Serial.println(sensorstate[i] == 1 ? "HIGH" : "LOW");
        Serial.println(" (" + String((sensorstate[i] == SENSOR_ACTIVE ? "ACTIVE)" : "INACTIVE)")));
        if (sensor_enable[i] == SENSOR_ENABLED) {
          // Report ACTIVE=TRUE regardless of pin voltage (defined in config.h)
          reportSensorState(i, sensorstate[i] == SENSOR_ACTIVE);
#if (BOARD_ID == BOARD_ID_LNIR)
          setSensorLED(i, sensorstate[i] == SENSOR_ACTIVE);
#endif
        }
      }
      // Regardless, update the sensor state record
      lastlastsensorstate[i] = lastsensorstate[i];
      lastsensorstate[i] = sensorstate[i];
    }
  }
}

void setSensorLED(int sensor, bool state) {
  if (BOARD_REV >= BOARD_REV_MIN_LED_SUPPORT) {
    digitalWrite(led_pin[sensor], state ? LED_ACTIVE : LED_INACTIVE);
  }
}

/*
void handleExternalButtons() {
  currentmillis = millis();
  if ((currentmillis > lastbuttoncheck) && ((currentmillis - lastbuttoncheck) > BUTTON_CHECK_INTERVAL_MS)) {
    for (int i = 0; i < NUM_SERVOS; i++) {
      buttonstate[i] = digitalRead(buttons[i]);
      if ((buttonstate[i] == lastbuttonstate[i]) && (buttonstate[i] != lastlastbuttonstate[i])) {
        // We've had a change that has been stable for two check intervals.  Toggle the servo
        servos[i] = (servos[i] == SERVO_CLOSED ? SERVO_THROWN : SERVO_CLOSED);
        servo_leds[i] = servos[i];
      }
      // Regardless, update the button state record
      lastlastbuttonstate[i] = lastbuttonstate[i];
      lastbuttonstate[i] = buttonstate[i];
    }
  }
}
*/




