#ifndef TIME_H
#define TIME_H

class Time {
protected:
  //int _mins;
  //int _hours;
  //int _days;
  unsigned long _time;

public:
  Time();
  Time(unsigned long t);
  Time(int m, int h, int d);
  //Time(int m, int h);
  Time(Time& t);
  bool operator>(Time& t);
  bool operator>=(Time& t);
  bool operator<(Time& t);
  bool operator<=(Time& t);
  bool operator==(Time& t);
  void addMinutes(int m);
  void setMins(int m);
  void setHours(int h);
  void setDays(int d);
  int mins(void);
  int hours(void);
  int days(void);
  int minutesAfter(Time& t);
  String toStringHHMM(void);
  unsigned long makeLong();
};

#endif // TIME_H
