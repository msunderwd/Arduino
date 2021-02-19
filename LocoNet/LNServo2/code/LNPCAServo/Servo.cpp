#define SERVO_CPP

#include "Servo.hpp"


Servo::Servo() {
  int index = 0;
  driver = NULL;
  closed_val = 0.0;
  thrown_val = 0.0;
  _state = false;
  mode = SERVO_MODE;
}

void Servo::setDriver(PCA9685 *d) {
  driver = d;
}

void Servo::setIndex(int i) {
  index = i;
}

void Servo::configure(int idx, PCA9685 *d, float clv, float thv, bool st, ServoMode md) {
  index = idx;
  driver = d;
  closed_val = validateServoValue(clv);;
  thrown_val = validateServoValue(thv);
  _state = st;
  mode = md;
}

void Servo::initFromEEPROM(int addr, int laddr, float clv, float thv, bool st, bool lock, int mode) {
  _address = addr;
  lockaddr = laddr;
  closed_val = clv;
  thrown_val = thv;
  _state = st;
  locked = lock;
  setThrown(_state);
  setModeInt(mode);
}

void Servo::throwServo(void) {
  _state = true;
  if (mode == SERVO_MODE) {
    driver->setChannelPWM(index, servoEval.pwmForAngle(thrown_val));
  } else {
    driver->setChannelPWM(index, int(thrown_val));
  }
  Serial.println("PWM Readback = " + String(driver->getChannelPWM(index)));
}

void Servo::closeServo(void) {
  _state = false;
  if (mode == SERVO_MODE) {
    driver->setChannelPWM(index, servoEval.pwmForAngle(closed_val));
  } else {
    driver->setChannelPWM(index, int(closed_val));
  }
  Serial.println("PWM Readback = " + String(driver->getChannelPWM(index)));
}

void Servo::toggle(void) {
  if (isThrown()) {
    closeServo();
  } else {
    throwServo();
  }
}

bool Servo::isThrown(void) {
  return(_state == true);
}

void Servo::setThrown(bool th) {
  _state = th;
  if (th) {
    throwServo();
  } else {
    closeServo();
  }
}

void Servo::writeStateToEEPROM() {
  // TODO: Write this
}

float Servo::closedVal(void) {
  return(closed_val);
}

float Servo::thrownVal(void) {
  return(thrown_val);
}

float Servo::validateServoValue(float v) {
  if (mode == SERVO_MODE) {
    // In Servo Mode, clip the servo values at +/- 90 deg. 
    if (v > MAX_SERVO_VALUE) { v = MAX_SERVO_VALUE; }
    if (v < MIN_SERVO_VALUE) { v = MIN_SERVO_VALUE; }
  } else if (mode == ONOFF_MODE) {
    // In OnOff mode, threshold the given value... above 2047, make it 4096 (full on), else 0 (full off)
    v =  (v > 2047.5) ? 4096 : 0;
  }
  // If ONOFF_MODE return the servo value unchanged.
  return(v);
}

void Servo::setClosedVal(float cv) {
  closed_val = validateServoValue(cv);
}

void Servo::setThrownVal(float tv) {
  thrown_val = validateServoValue(tv);
}

float Servo::servoVal(void) {
  return(_state ? thrown_val : closed_val);
}

void Servo::setAddress(unsigned int a) {
  _address = a;
}

unsigned int Servo::address(void) {
  return(_address);
}

void Servo::setLockAddress(unsigned int a) {
  lockaddr = a;
}

unsigned int Servo::lockAddress(void) {
  return(lockaddr);
}

bool Servo::isLocked(void) {
  return(locked);
}

bool Servo::setLocked(bool l) {
  locked = l;
  return(locked);
}

void Servo::setModeInt(int m) {
  if (m == 0) { mode = SERVO_MODE; }
  else if (m == 1) { mode = ONOFF_MODE; }
  else { mode == SERVO_MODE; } // default to servo mode for invalid enum values
}

void Servo::setMode(ServoMode m) {
  mode = m;
}

ServoMode Servo::getMode(void) {
  return(mode);
}

int Servo::getModeInt(void) {
  if (mode == SERVO_MODE) { return(0); }
  else if (mode == ONOFF_MODE) { return(1); }
  else { return(0); } // default to servo mode for invalud enum values
}
