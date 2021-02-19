#ifndef SERVO_HPP
#define SERVO_HPP

#include <Arduino.h>
#include <PCA9685.h>

#define SERVO_BASE_PULSE 1000
#define MIN_SERVO_VALUE (-90.0)
#define MAX_SERVO_VALUE 90.0

// **********
// Servo Modes
//    SERVO_MODE : Use +/- 90.0 degrees to position servo arm
//    ONOFF_MODE : Use as gpio (0% or 100% PWM emulates logic 0 or 1)
//        When setting the Throw/Close limits in ONOFF_MODE:
//        any value below 2047.5 will be thresholded to 0 (0%)
//        any value above 2047.5 will be thresholded to 4096 (100%)
//
//  
// See setModeInt() / getModeInt() for integer equivalents
// used to store this enum in EEPROM
enum ServoMode {
  SERVO_MODE,
  ONOFF_MODE
};

class Servo {
 private:
  PCA9685 *driver;
//  PCA9685_ServoEvaluator servoEval;
  PCA9685_ServoEval servoEval;
  float closed_val;
  float thrown_val;
  bool _state;
  unsigned int _address;
  unsigned int lockaddr;
  bool locked;
  int index;
  ServoMode mode;

 protected:
  float validateServoValue(float v); 

 public:
  Servo(void);
  void configure(int idx, PCA9685 *driver, float clv = 0.0, float thv = 0.0, bool st = false, ServoMode m = SERVO_MODE);
  void initFromEEPROM(int addr, int laddr, float clv, float thv, bool st, bool lock, int mode);
  void setDriver(PCA9685 *d);
  void setIndex(int i);
  void throwServo(void);
  void closeServo(void);
  void toggle(void);
  bool isThrown(void);
  void setThrown(bool st);
  void writeStateToEEPROM(void);
  float closedVal(void);
  float thrownVal(void);
  float servoVal(void);
  //void doPulse(void);
  void setClosedVal(float cv);
  void setThrownVal(float tv);
  unsigned int address(void);
  void setAddress(unsigned int a);
  void setLockAddress(unsigned int a);
  unsigned int lockAddress(void);
  bool isLocked(void);
  bool setLocked(bool l);
  ServoMode getMode(void);
  int getModeInt(void);
  void setMode(ServoMode m);
  void setModeInt(int m);
};

#endif // SERVO_HPP
