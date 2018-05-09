#ifndef CONFIG_H
#define CONFIG_H

#ifdef LNSERVO_INO
#define EXTERN
#else
#define EXTERN extern
#endif

//#include "Pin.hpp"
#include "Servo.hpp"

// Servos
#define NUM_SERVOS 8
#define NUM_LOCKS NUM_SERVOS

// Current monitor feedback
EXTERN byte imonitor;

// Global Programming State
EXTERN byte stato;

// Current versions of addresses
//EXTERN unsigned int decoder_cv_address;
//EXTERN unsigned int address_received;
//EXTERN unsigned int broadcast;
//EXTERN unsigned int base_address;
//EXTERN unsigned int servo1_address;
//EXTERN unsigned int servo2_address;
//EXTERN unsigned int servo3_address;
//EXTERN unsigned int servo4_address;
//EXTERN unsigned int lock1_address;
//EXTERN unsigned int lock2_address;
//EXTERN unsigned int lock3_address;
//EXTERN unsigned int lock4_address;
//EXTERN unsigned int sensor_base_address;


#define SERVO_CLOSED 1
#define SERVO_THROWN 0
#define SERVO_PERIOD 40

#define LOCK_LOCKED 1
#define LOCK_UNLOCKED 0

// LocoNet Transmit Pin
#define  TX_PIN   6 // Assumed by the library. Do not change.
#define  RX_PIN   8 // Required by the library but here for reference

#define SW_RESPONSE_OPCODE 0xB4

#ifndef LNSERVO_INO
//extern byte servo_state;
//extern byte servo_limits[NUM_SERVOS][2];
#endif

// LocoNet Message Buffers
EXTERN LnBuf        LnTxBuffer;
EXTERN lnMsg        *LnPacket;
EXTERN lnMsg        SerialLnPacket;

EXTERN bool serial_ln_message;

// Pin Objects for handling the IO Pins
EXTERN Servo Servos[8];

#endif // CONFIG_H



