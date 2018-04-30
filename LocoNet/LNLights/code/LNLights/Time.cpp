#include <Arduino.h>
#include "Time.hpp"

Time::Time() {
  _time = 0;
}

Time::Time(unsigned long t) {
  _time = t;
}
Time::Time(int m, int h, int d) {
  _time = m + h*24 + d*(60*24);
}

Time::Time(Time& t) {
  _time = t._time;
}

unsigned long Time::makeLong() {
  return(_time);
}

bool Time::operator==(Time& t) {
  //if (this->makeLong() == t.makeLong()) {
  if (this->_time == t._time) {
    return(true);
  } else {
    return(false);
  }
}

bool Time::operator<(Time& t) {
  //if (this->makeLong() < t.makeLong()) { return(true); }
  if (this->_time < t._time) { return(true); }
  else { return(false); }
}

bool Time::operator>(Time& t) {
  //if (this->makeLong() > t.makeLong()) { return(true); }
  if (this->_time > t._time) { return(true); }
  else { return(false); }
}

bool Time::operator<=(Time& t) {
  return(!( *this > t));
}

bool Time::operator>=(Time& t) {
  return(!(*this < t));
}

void Time::addMinutes(int m) {
  _time += m;
}

void Time::setMins(int m) {
  int mins = _time % 60;
  _time = _time - this->mins() + m;
}

void Time::setHours(int h) {
  //int hours = _time % 24;
  _time = _time - (this->hours()*60) + h*60;
}

void Time::setDays(int d) {
  int days = _time % (60*24);
  _time = _time - (this->days()*(60*24)) + d*(60*24);
  //_days = d;
}

int Time::mins(void) {
  return(_time % 60);
}

int Time::hours(void) {
  return((_time % 1440)/60);
}

int Time::days(void) {
  return(_time / 1440);
}

int Time::minutesAfter(Time& t) {
  if (*this < t) {
    return(0);
  }

  return(this->_time - t._time);
}

String Time::toStringHHMM(void) {
  char buf[6];
  sprintf(buf, "%02d:%02d", this->hours(), this->mins());
  return(String(buf));
}

