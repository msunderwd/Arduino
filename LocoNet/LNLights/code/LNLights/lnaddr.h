// LocoNet Address mappings
// TODO: Clean up or scrap this

// Default device address for configuring the board
// via LocoNet Ops Mode programming
#define DEFAULT_DEVICE_ADDRESS 12000

// Configuration CVs
// Note: these may not correspond directly to EEPROM addresses
// EEPROM addresses are defined in eeprom.h

// NMRA Mandatory CVs
#define CV_DECODER_ADDRESS_LSB 1
#define CV_MANUFACTURER_VERSION 7
#define CV_MANUFACTURER_ID 8
#define CV_DECODER_ADDRESS_MSB 9
#define CV_ACC_DECODER_CONFIG 29
#define CV_HEAD1_FUNCTION 31
#define CV_HEAD2_FUNCTION 32
#define CV_HEAD3_FUNCTION 33
#define CV_HEAD4_FUNCTION 34
#define CV_SERVO1_FUNCTION 35
#define CV_SERVO2_FUNCTION 36
#define CV_SERVO3_FUNCTION 37
#define CV_SERVO4_FUNCTION 38

// CV29 Bits (NMRA definition)
typedef struct {
  unsigned int rfu               : 3; // Bits 0-2 are RFU
  unsigned int bidi_comms        : 1; // Bit 3 0=disabled, 1=enabled
  unsigned int rfu2              : 1; // Bit 4 is RFU
  unsigned int decoder_type      : 1; // Bit 5. 0=basic, 1=extended
  unsigned int address_method    : 1; // Bit 6 0=decoder 1=output
  unsigned int accessory_decoder : 1; // Bit 7 0=multifunction 1=accessory
} cv29_t;

typedef union {
  struct {
    unsigned int grnyel : 2; // Bits 0-1 are Green+Yellow output (Pin 2&3)
    unsigned int red    : 2; // Bits 2-3 are Red output (Pin 4)
    unsigned int servo  : 3; // Bits 4-6 are assigned servo number (0=none)
    unsigned int rfu    : 1; // Bit 7 is reserved
  } head;
  struct {
    unsigned int pinmode : 2; // Bits 0-1 are Servo pin mode
    unsigned int servo   : 3; // Bits 2-4 are assigned servo number (0=none)
    unsigned int rfu     : 3; // Bits 5-7 are reserved
  } servo;
  byte b;
} func_t;

#define HFUNC_SIGNAL  B00 // Pin is a signal output
#define HFUNC_BUTTON  B01 // Pin is a button input
#define HFUNC_LED     B10 // Pin is part of a RED/GREEN 2-led pair
#define HFUNC_SENSOR  B11 // Pin is part of a bicolor led pair

#define SFUNC_SERVO  B00 // Output to control a servo
#define SFUNC_BUTTON B01 // Input, as from a sensor or button
#define SFUNC_OUTPUT B10 // GPIO Output, as for solenoid or something
#define SFUNC_SENSOR B11 // Reserved for future use.
