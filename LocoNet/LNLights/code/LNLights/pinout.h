// Pin Definitions

// Pinouts for the LNPCA_LIGHT 1.1 board
#define GPIO1_PIN 2
#define GPIO2_PIN 3
#define PSU_CONTROL GPIO1_PIN

// Pinouts updated for LMPCA_SERVO 1.3 board
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

