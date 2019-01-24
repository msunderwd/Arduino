// Pin Definitions
#if ARDUINO_AVR_PRO

#if FLIPPED_BOD4_PINOUT

#define SENSOR1_PIN 10 // BOD_A
#define SENSOR2_PIN 11 // BOD_B
#define SENSOR3_PIN A0 // BOD_C
#define SENSOR4_PIN A1 // BOD_D
#define SENSOR5_PIN 2 // BOD_E
#define SENSOR6_PIN 3 // BOD_F
#define SENSOR7_PIN 4 // BOD_G
#define SENSOR8_PIN 5 // BOD_H

#else

#define SENSOR1_PIN 4 // BOD_A
#define SENSOR2_PIN 5 // BOD_B
#define SENSOR3_PIN 2 // BOD_C
#define SENSOR4_PIN 3 // BOD_D
#define SENSOR5_PIN A0 // BOD_E
#define SENSOR6_PIN A1 // BOD_F
#define SENSOR7_PIN 10 // BOD_G
#define SENSOR8_PIN 11 // BOD_H

#endif

#else

// These are dummy values. This board
// only supports the ProMini
#define SENSOR1_PIN 4 // BOD_A
#define SENSOR2_PIN 5 // BOD_B
#define SENSOR3_PIN 2 // BOD_C
#define SENSOR4_PIN 3 // BOD_D
#define SENSOR5_PIN A0 // BOD_E
#define SENSOR6_PIN A1 // BOD_F
#define SENSOR7_PIN 10 // BOD_G
#define SENSOR8_PIN 11 // BOD_H

#endif
