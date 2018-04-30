#ifndef CONFIG_H
#define CONFIG_H

#include "Time.hpp"

#ifdef LNLIGHTS_INO
#define EXTERN
#else
#define EXTERN extern
#endif

// Light Sections
#define NUM_LIGHTS 4

#define DEFAULT_TABLE_LENGTH 4
#define RED 0
#define GREEN 1
#define BLUE 2
#define WHITE 3

// Current versions of addresses
EXTERN unsigned int address_received;
EXTERN unsigned int base_address;

EXTERN bool master_on;        // TRUE = master power on
EXTERN bool follow_ln;        // TRUE = master power follows LN Power
EXTERN bool clock_mode;       // TRUE = fast clock mode enabled / FALSE = static lighting mode
EXTERN bool day_night;        // TRUE = static mode daytime / FALSE = static mode nighttime
EXTERN bool transition_mode;  // TRUE = static mode do transitions on change.

typedef struct {
  bool enabled;
  unsigned long lastFlashTime;
  unsigned long nextFlashTime;
  int flashWaitTime;
  int flashState;
} storm_state_t;
//EXTERN storm_state_t stormState[NUM_LIGHTS];

// Defines for Operating Modes (for readability)
#define POWER_OFF false
#define POWER_ON  true
#define CLOCK_MODE_STATIC false
#define CLOCK_MODE_FASTCLOCK true
#define STATIC_DAY true
#define STATIC_NIGHT false
#define TRANSITION_YES true
#define TRANSITION_NO false
#define STORM_MODE_OFF false
#define STORM_MODE_ON true
#define PSU_ON LOW
#define PSU_OFF HIGH

// RGBW Pin Mapping
#define RED_CHANNEL   1
#define GREEN_CHANNEL 3
#define BLUE_CHANNEL  2
#define WHITE_CHANNEL 0 // verified

typedef struct {
  int rate;
  int frac;
  int mins;
  int hours;
  int days;
} fast_clock_t;

EXTERN fast_clock_t fast_clock;

// Current Time
EXTERN Time current_time;

typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t white;
} led_t;

EXTERN led_t lights[NUM_LIGHTS];

// LocoNet Transmit Pin
#define  TX_PIN   6 // Assumed by the library. Do not change.
#define  RX_PIN   8 // Required by the library but here for reference

#define SW_RESPONSE_OPCODE 0xB4

// LocoNet Message Buffers
EXTERN LnBuf        LnTxBuffer;
EXTERN lnMsg        *LnPacket;
EXTERN lnMsg        SerialLnPacket;

EXTERN bool serial_ln_message;

#endif // CONFIG_H



