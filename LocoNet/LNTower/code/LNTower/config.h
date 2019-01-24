#ifdef LNTOWER_INO
#define EXTERN
#else
#define EXTERN extern
#endif

// Pin configurations
EXTERN int led;
EXTERN int button;

// Sensors
#define NUM_SENSORS 4
#define NUM_SENSOR_SUBADDRS NUM_SENSORS

#define SENSOR_ENABLED 1
#define SENSOR_DISABLED 0

#define SENSOR_ACTIVE 0
#define SENSOR_INACTIVE 1

// External Button Inputs
#define SENSOR_CHECK_INTERVAL_MS 300

// Current versions of addresses
EXTERN unsigned int broadcast;
EXTERN unsigned int base_address;
EXTERN unsigned int address_received;
EXTERN int sensorstate[NUM_SENSORS];
EXTERN byte sensor_enable[NUM_SENSORS];
EXTERN unsigned int sensor_addr[NUM_SENSORS];

// LocoNet Transmit Pin
#define  TX_PIN   6 // Assumed by the library. Do not change.
#define  RX_PIN   8 // Required by the library but here for reference

// LocoNet Message Buffers
EXTERN LnBuf        LnTxBuffer;
EXTERN lnMsg        *LnPacket;
EXTERN lnMsg        SerialLnPacket;

