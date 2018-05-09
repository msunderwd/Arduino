#define SERVO_CPP

#include "Servo.hpp"


Servo::Servo() {
  index = 0;
  driver = NULL;
  closed_val = 0.0;
  thrown_val = 0.0;
  _state = false;
}

void Servo::setDriver(PCA9685 *d) {
  driver = d;
}

void Servo::setIndex(byte i) {
  index = i;
}

void Servo::configure(byte idx, PCA9685 *d, float clv, float thv, bool st) {
  index = idx;
  driver = d;
  if (clv > MAX_SERVO_VALUE) { clv = MAX_SERVO_VALUE; }
  closed_val = clv;
  if (thv > MAX_SERVO_VALUE) { thv = MAX_SERVO_VALUE; }
  thrown_val = thv;
  _state = st;
}

void Servo::initFromEEPROM(int addr, int laddr, float clv, float thv, bool st, bool lock) {
  _address = addr;
  lockaddr = laddr;
  closed_val = clv;
  thrown_val = thv;
  _state = st;
  locked = lock;
  setThrown(_state);
}

void Servo::throwServo(void) {
  _state = true;
  driver->setChannelPWM(index, servoEval.pwmForAngle(thrown_val));
  Serial.println("PWM Readback = " + String(driver->getChannelPWM(index)));
}

void Servo::closeServo(void) {
  _state = false;
  driver->setChannelPWM(index, servoEval.pwmForAngle(closed_val));
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
  if (v > MAX_SERVO_VALUE) { v = MAX_SERVO_VALUE; }
  if (v < MIN_SERVO_VALUE) { v = MIN_SERVO_VALUE; }
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

