#define BOARD_ID_LNTOWER 2
#define BOARD_ID BOARD_ID_LNTOWER

#if BOARD_ID == BOARD_ID_LNTOWER

// Pin Definitions

// Common pins (may be unused on some boards)

#define SENSOR1_PIN 4 // BOD_A
#define SENSOR2_PIN 5 // BOD_B
#define SENSOR3_PIN 2 // BOD_C
#define SENSOR4_PIN 3 // BOD_D
#define SENSOR5_PIN A0 // BOD_E
#define SENSOR6_PIN A1 // BOD_F
#define SENSOR7_PIN 10 // BOD_G
#define SENSOR8_PIN 11 // BOD_H

#else // Default to LNIR

#if ARDUINO_AVR_PRO

#define DRIVE_PIN 9

#define SENSOR1_PIN A3 // J4
#define SENSOR2_PIN A4 // J5
#define SENSOR3_PIN A2 // J6
#define SENSOR4_PIN A0 // J7
#define SENSOR5_PIN A1 // J8
#define SENSOR6_PIN 13 // J9

#define LED1_PIN 11 // With Sensor 1 / J4
#define LED2_PIN 10 // With Sensor 2 / J5
#define LED3_PIN 4  // With Sensor 3 / J6
#define LED4_PIN 3  // with Sensor 4 / J7
#define LED5_PIN 2  // with Sensor 5 / J8
#define LED6_PIN 12 // With Sensor 6 / J9

#define AUX1_PIN A7
#define AUX2_PIN A6

#elif ARDUINO_AVR_NANO

// Assumes LN_IR board.  LN_TOWER does not support the NANO

#define DRIVE_PIN 9

#define SENSOR1_PIN A7 // J4
#define SENSOR2_PIN A4 // (NC)
#define SENSOR3_PIN A6 // J6
#define SENSOR4_PIN A4 // J7
#define SENSOR5_PIN A5 // J8
#define SENSOR6_PIN A3 // J9

#define LED1_PIN A1 // With Sensor 1 / J4
#define LED2_PIN A0 // With Sensor 2 / J5
#define LED3_PIN 4  // With Sensor 3 / J6
#define LED4_PIN 3  // with Sensor 4 / J7
#define LED5_PIN 2  // with Sensor 5 / J8
#define LED6_PIN A2 // With Sensor 6 / J9

#define AUX1_PIN A7 // (NC)
#define AUX2_PIN A6 // (NC)

#endif // Arduino selection

#endif // Board selection
