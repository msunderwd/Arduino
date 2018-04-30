#ifdef LNSENSOR_INO
#define EXTERN
#else
#define EXTERN extern
#endif

// These are the boards this code is compatible with
#define BOARD_ID_LNIR 1
#define BOARD_ID_LNTOWER 2
// Choose the board to build for here.
#define BOARD_ID BOARD_ID_LNTOWER

//#include "pinout.h"

// Pin configurations
EXTERN int led;
EXTERN int button;

// Sensors
#if (BOARD_ID == BOARD_ID_LNIR)
#define NUM_SENSORS 6
#elif (BOARD_ID == BOARD_ID_LNTOWER)
#define NUM_SENSORS 8
#else
// Default to something.
#define NUM_SENSORS 6
#endif

#define NUM_SENSOR_SUBADDRS NUM_SENSORS

#define SENSOR_ENABLED 1
#define SENSOR_DISABLED 0

#if (BOARD_ID == BOARD_ID_LNTOWER)
#define SENSOR_ACTIVE 0
#define SENSOR_INACTIVE 1
#else // default works for BOARD_ID_LNIR
#define SENSOR_ACTIVE 1
#define SENSOR_INACTIVE 0
#endif

// LED output is negative logic.
#define LED_ACTIVE 0
#define LED_INACTIVE 1

// External Button Inputs
#define SENSOR_CHECK_INTERVAL_MS 300

// Current versions of addresses
EXTERN unsigned int broadcast;
EXTERN unsigned int base_address;
EXTERN unsigned int address_received;

EXTERN int sensorstate[NUM_SENSORS];
EXTERN byte sensor_enable[NUM_SENSORS];
EXTERN unsigned int sensor_addr[NUM_SENSORS];

// LED Outputs
#define LED_ON  1
#define LED_OFF 0

// LocoNet Transmit Pin
#define  TX_PIN   6 // Assumed by the library. Do not change.
#define  RX_PIN   8 // Required by the library but here for reference

// LocoNet Message Buffers
EXTERN LnBuf        LnTxBuffer;
EXTERN lnMsg        *LnPacket;
EXTERN lnMsg        SerialLnPacket;



