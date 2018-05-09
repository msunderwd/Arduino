#ifndef SERVO_HPP
#define SERVO_HPP

#include <Arduino.h>
#include <PCA9685.h>

#define SERVO_BASE_PULSE 1000
#define MIN_SERVO_VALUE (-90.0)
#define MAX_SERVO_VALUE 90.0

class Servo {
 private:
  PCA9685 *driver;
  PCA9685_ServoEvaluator servoEval;
  float closed_val;
  float thrown_val;
  bool _state;
  unsigned int _address;
  unsigned int lockaddr;
  bool locked;
  byte index;

 protected:
  float validateServoValue(float v); 

 public:
  Servo(void);
  void configure(byte idx, PCA9685 *driver, float clv = 0.0, float thv = 0.0, bool st = false);
  void initFromEEPROM(int addr, int laddr, float clv, float thv, bool st, bool lock);
  void setDriver(PCA9685 *d);
  void setIndex(byte i);
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
};

#endif // SERVO_HPP
