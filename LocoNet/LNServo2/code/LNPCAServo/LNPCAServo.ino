// TODO: Broadcast address stuff is broken.  See pages 12-13 of LocoNet PE spec.

// Original code by RMWeb.co.uk member "antogar"
// http://www.rmweb.co.uk/community/index.php?/topic/92932-arduino-loconet-occupancy-detector/

// This code handles N servos (currently N=8)
// LocoNet commands (0xB0 or 0xBD) can command the servos to CLOSED or THROWN position.
//
// Uses this library for control of the PCA9685
// https://github.com/NachtRaveVL/PCA9685-Arduino/blob/master/README.md
//
// LocoNet Addresses:
// * Servo 1 = Base Address
// * Servo 2 = Base Address + 1
// * Servo 3 = Base Address + 2
// * Servo 4 = Base Address + 3
// ... and so on ...
// * Lock 1 = Servo 1 Address + NUM_SERVOS
// * Lock 2 = Servo 2 Address + NUM_SERVOS
// ... and so on ...
// 
// Serial Port Commands:
// LN <opcode> <data1> <data2> <checksum> : Simulate LocoNet command
// C <servo> <value> : Set CLOSED state servo limit (0-255)
// T <servo> <value> : Set THROWN state servo limit (0-255)
// S : Store servo limits to EEPROM
// A <address> : Set base LocoNet address (and by extension all addresses) for device
// c <servo> : CLOSE Servo
// t <servo> : THROW Servo
//
// *** LocoNet IO Pinouts ***
//
// For Arduino MEGA, RX = Pin 48 (ICP5)
// For Arduino Uno, RX = Pin 8 (ICP1) 
// For Arduino ProMini, RX = Pin 8 (ICP1)
// We assume TX = Pin 6 on all boards.
//
// PCA9685 channel assignments:
// * Servo 1 = PWM0
// * Servo 2 = PWM1
// * Servo 3 = PWM2
// * Servo 4 = PWM3
// * Servo 5 = PWM4
// * Servo 6 = PWM5
// * Servo 7 = PWM6
// * Servo 8 = PWM7

#define LN_TX_PIN 6

// NOTE: Always BOARD_MODE_SERVO
#define BOARD_MODE_SERVO 0
#define BOARD_MODE_SIGNAL 1
#define BOARD_MODE BOARD_MODE_SERVO

// Set this to allow sending LocoNet commands over the serial interface.
// Otherwise the "LN" Serial command will do nothing.
#define SERIAL_EMULATE_LN 0

#include <LocoNet.h>
#include <EEPROM.h>
#include <Wire.h>
#include <PCA9685.h>

#define LNSERVO_INO 1
#define GLOBALS_GO_HERE 1
#include "pinout.h"
#include "config.h"
#include "Serial.hpp"
#include "LocoNet.hpp"
#include "eeprom.hpp"
#include "eemap.h"
#include "globals.h"
#include "lnaddr.h"
#include "Servo.hpp"

#define WIRE_CLOCK_SPEED 400000

// PCA9685 devoce
//PCA9685 pwmController(Wire, PCA9685_PhaseBalancer_Weaved);
PCA9685 pwmController(Wire, WIRE_CLOCK_SPEED, B000001);
//PCA9685_ServoEvaluator pwmServos[NUM_SERVOS];
PCA9685_ServoEval pwmServos[NUM_SERVOS];

// Millisecond timer tracking
unsigned long lasttimerpop;

/** configurePins()
 * 
 * Set all the pinouts and Servos up for operation
 */
void configurePins() {
  // Set the current monitor input pin.
  //imonitor = IMONITOR_PIN;
  imonitor = A4;
}

void setup()
{
  unsigned long currentmillis = millis();
  lasttimerpop = currentmillis;
  

  // LocoNet Addresses are 12 bits so stored as 2 bytes
  //decoder_cv_address = getDecoderCVAddressFromEEPROM();

  // Configure the pins here.   We need to have already set the servo addresses.
  configurePins();

  // Configure the serial port for 57600 baud
  Serial.begin(57600);
  
  // Update Servo settings from EEPROM
  //initServosFromEEPROM();

  // Initialize LocoNet interface. Note that with the official Arduino library
  // you can't choose the TX pin.
  LocoNet.init(LN_TX_PIN);
  
  // Initialize a LocoNet packet buffer to buffer bytes from the PC 
  initLnBuf(&LnTxBuffer) ;
  serial_ln_message = false;

  // Set up the PCA9685 and initialize the Servos.
  Wire.begin();
  Wire.setClock(WIRE_CLOCK_SPEED);

  pwmController.resetDevices();
  //pwmController.init();
  pwmController.init(PCA9685_OutputDriverMode_TotemPole,
                    PCA9685_OutputEnabledMode_Normal,
                    PCA9685_OutputDisabledMode_Low,
                    PCA9685_ChannelUpdateMode_AfterStop,
                    PCA9685_PhaseBalancer_None);
  pwmController.setPWMFrequency(50);

  for (int i = 0; i < NUM_SERVOS; i++) {
    pwmController.setChannelPWM(i, 128 << 4);
    Servos[i].setDriver(&pwmController);
    Servos[i].setIndex(i);
    readServoDataFromEEPROM(i);
  }

  // Print information to the Serial port
  Serial.println(F("Ciao Bello!"));
  Serial.print(F("decoder address"));
  Serial.print(F("\t"));
  //Serial.println(base_address);
  Serial.print(F("broadcast address"));
  Serial.print(F("\t"));
  Serial.println(broadcast);
  for (int i = 0; i < NUM_SERVOS; i++) {
    printServoInfo(i);
  }
}

void printServoInfo(int servo) {
  Serial.print((F("Servo #")));
  Serial.println(servo+1);
  Serial.print(F("\tAddress:\t"));
  Serial.println(Servos[servo].address());
  Serial.print(F("\tLock address:\t"));
  Serial.println(Servos[servo].lockAddress());
  Serial.print(F("\tLimits: C="));
  Serial.print(String(Servos[servo].closedVal()));
  Serial.print(F(" T="));
  Serial.println(String(Servos[servo].thrownVal()));
}

void loop()
{ 
  // Check for serial traffic
  if (Serial.available()) {
    handleSerialInput();
  }
   
#if (SERIAL_EMULATE_LN == 0)
  // Only do this if using the hardware LocoNet interface
  LnPacket = LocoNet.receive() ;
#endif
  if (LnPacket) {
    handleLocoNetInterface(LnPacket);
  }

  // If this was a serial LocoNet message, clean up the mess.
//  if (serial_ln_message == true) {
//    LnPacket = NULL;
//    serial_ln_message = false;
//  }

} // end loop()

// This is the hardware side of a servo change operation.
void doServoChange(byte servo, bool thrown) {
  if (servo >= NUM_SERVOS) {
    // Servo index out of range.  This should not happen.
    return;
  }
  // This is a servo change.
  Serial.print("Servo " + String(servo) + (thrown ? " THROWN" : " CLOSED"));
  Serial.println(" val = " + String(thrown ? Servos[servo].thrownVal() : Servos[servo].closedVal()));
  // Only take action if this is actually a state change (ignore repeated commands to the same state)
  if (thrown != Servos[servo].isThrown()) {
    Servos[servo].setThrown(thrown);
    writeServoStateToEEPROM(servo);
  }
}

/** handleServoChange()
 *  
 *  Called from LocoNet when a OPC_SW_REQ (0xB0) command is received.
 */
void handleLNServoChange(unsigned int servoaddr, uint8_t data2, bool ack) {
  byte index = 0;
  Serial.println("handleLNServoChange() servo=" + String(servoaddr) + " data2=" + String(data2) + " ack=" + String(ack));

  // First, find the Servo we are addressing.
  for (index = 0; index < NUM_SERVOS; index++) {
    if (Servos[index].address() == servoaddr) { break; }
  }
  // Oops. We didn't find it. Return.
  if (index == NUM_SERVOS) { return; }

  Serial.println("servo index = " + String(index));
  
  // D[2].4 (0x10) is ON/OFF (1/0)
  // D[2].5 (0x20) is CLOSE/THROW (1/0)
  if ((data2 & 0x10) == 0x10) {
    doServoChange(index, ((data2 & 0x20) == 0x00));
  }
  
  if (true) {
    //sendTXtoLN(0xB4, (ack ? 0x3D : 0x30), 0x7F);
    reportTurnoutState(servoaddr, Servos[index].isThrown());
  }
}

void handleLockChange(unsigned int lockaddr, uint8_t data2, bool ack) {
  //byte index = lockaddr - lock1_address;
  bool locked = (data2 & 0x20 == 0x20);
  byte index = 0;
  for (index = 0; index < NUM_SERVOS; index++) {
    if (Servos[index].lockAddress() == lockaddr) { break; }
  }
  if (index == NUM_SERVOS) { return; }
  Serial.println("Lock " + String(index) + (locked ? " LOCKED" : " UNLOCKED"));
  bool oldv = Servos[index].isLocked();
  Servos[index].setLocked(locked);
  if (oldv != locked) {
    writeLockStateToEEPROM(index);
  }

  if (ack && true) {
    sendTXtoLN(0xB4, (ack ? 0x3D : 0x30), 0x7F);
    reportTurnoutState(lockaddr, Servos[index].isLocked());
  }
}
