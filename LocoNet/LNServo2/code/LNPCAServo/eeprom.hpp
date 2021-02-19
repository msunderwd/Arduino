#include "eemap.h"

extern unsigned int getDecoderCVAddressFromEEPROM();
extern unsigned int readLNAddressFromEEPROM(int idx);
extern void writeLNAddressToEEPROM(int idx, unsigned int addr);
extern void readServoDataFromEEPROM(int i);
extern void writeServoDataToEEPROM(int i);
extern void writeServoLimitsToEEPROM();
extern void writeServoStateToEEPROM(int servo);
extern void writeLockStateToEEPROM(int servo);
extern void writeServoModeToEEPROM(int servo);
extern void writeServoAddressesToEEPROM();
extern void writeCVToEEPROM(int idx, byte val);
extern byte readCVFromEEPROM(int idx);
