#ifndef LOCK_HPP
#define LOCK_HPP

class Lock {
  bool state;
  unsigned int _address;

public:
  Lock();
  bool isLocked(void);
  bool setLocked(bool l);
  bool lock(void);
  bool unlock(void);
  void setAddress(unsigned int a);
  unsigned int address(void);
};

#endif // LOCK_HPP
