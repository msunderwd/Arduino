// Pin Definitions

// Pinouts updated for 1.3 board
// NOTE: These should be obsolete for PCA board.

#if ARDUINO_AVR_PRO

#define SERVO1_PIN A5 // Not used v1.3
#define SERVO2_PIN A4 // Not used v1.3
#define SERVO3_PIN A7 // Not used v1.3
#define SERVO4_PIN A6 // Not used v1.3

#define IMONITOR_PIN A0

#elif ARDUINO_AVR_NANO

#define SERVO1_PIN A5 // Not used v1.3 // NC
#define SERVO2_PIN A4 // Not used v1.3 // NC
#define SERVO3_PIN A7 // Not used v1.3 // NC
#define SERVO4_PIN A6 // Not used v1.3 // NC

#define IMONITOR_PIN A4

#endif

