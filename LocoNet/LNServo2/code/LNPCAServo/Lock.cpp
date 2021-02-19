#include <Arduino.h>
#include "Lock.hpp"

Lock::Lock() {
  state = false;
  _address = 0;
}

bool Lock::isLocked(void) {
  return(state);
}

bool Lock::setLocked(bool l) {
  state = l;
  return(state);
}

bool Lock::lock(void) {
  state = true;
  return(state);
}

bool Lock::unlock(void) {
  state = false;
  return(state);
}

void Lock::setAddress(unsigned int a) {
  _address = a;
}

unsigned int Lock::address(void) {
  return(_address);
}
