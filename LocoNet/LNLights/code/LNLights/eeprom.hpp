#include "eemap.h"
#include "Time.hpp"

extern unsigned int readLNAddressFromEEPROM(int idx);
extern void writeLNAddressToEEPROM(int idx, unsigned int addr);
extern void readGlobalStateFromEEPROM(void);
extern void writeGlobalStateToEEPROM(void);
extern void readLightDataFromEEPROM(int a, led_t *p);
extern void writeLightDataToEEPROM(int a, led_t *p);
extern Time& readTimeFromEEPROM(int addr);
extern void writeTimeToEEPROM(int addr, Time& t);

extern unsigned int getDecoderCVAddressFromEEPROM();
extern void writeCVToEEPROM(int idx, byte val);
extern byte readCVFromEEPROM(int idx);

