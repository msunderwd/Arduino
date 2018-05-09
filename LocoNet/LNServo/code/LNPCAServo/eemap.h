#define EEPROM_BASE_ADDRESS 0 /* 2 bytes -- Decoder base address */
#define EEPROM_LOCK_ADDRESS 2   /* 2 bytes -- Lock address */
#define EEPROM_SERVO_ADDR_BASE 4
#define EEPROM_SERVO1_ADDR     4
#define EEPROM_SERVO2_ADDR     6
#define EEPROM_SERVO3_ADDR     8
#define EEPROM_SERVO4_ADDR    10
#define EEPROM_SERVO5_ADDR    12
#define EEPROM_SERVO6_ADDR    14
#define EEPROM_SERVO7_ADDR    16
#define EEPROM_SERVO8_ADDR    18
#define EEPROM_SERVO_STATE_BASE 20
#define EEPROM_SERVO1_STATE        20  /* 1 byte -- Servo 1 state */
#define EEPROM_SERVO2_STATE        21 /* 1 byte -- Servo 2 state */
#define EEPROM_SERVO3_STATE        22 /* 1 byte -- Servo 3 state */
#define EEPROM_SERVO4_STATE        23 /* 1 byte -- Servo 4 state */
#define EEPROM_SERVO5_STATE        24 /* 1 byte -- Servo 5 state */
#define EEPROM_SERVO6_STATE        25 /* 1 byte -- Servo 6 state */
#define EEPROM_SERVO7_STATE        26 /* 1 byte -- Servo 7 state */
#define EEPROM_SERVO8_STATE        27 /* 1 byte -- Servo 8 state */
#define EEPROM_SERVO_CLOSED_BASE   30
#define EEPROM_SERVO1_CLOSED_ANGLE 30 /* 4 byte -- Servo 1 closed angle (float) */
#define EEPROM_SERVO2_CLOSED_ANGLE 34 /* 4 byte -- Servo 2 closed angle (float) */
#define EEPROM_SERVO3_CLOSED_ANGLE 38 /* 4 byte -- Servo 3 closed angle (float) */
#define EEPROM_SERVO4_CLOSED_ANGLE 42 /* 4 byte -- Servo 4 closed angle (float) */
#define EEPROM_SERVO5_CLOSED_ANGLE 46 /* 4 byte -- Servo 5 closed angle (float) */
#define EEPROM_SERVO6_CLOSED_ANGLE 50 /* 4 byte -- Servo 6 closed angle (float) */
#define EEPROM_SERVO7_CLOSED_ANGLE 54 /* 4 byte -- Servo 7 closed angle (float) */
#define EEPROM_SERVO8_CLOSED_ANGLE 58 /* 4 byte -- Servo 8 closed angle (float) */
#define EEPROM_SERVO_THROWN_BASE   60
#define EEPROM_SERVO1_THROWN_ANGLE 60 /* 4 byte -- Servo 1 thrown angle (float) */
#define EEPROM_SERVO2_THROWN_ANGLE 64 /* 4 byte -- Servo 2 thrown angle (float) */
#define EEPROM_SERVO3_THROWN_ANGLE 68 /* 4 byte -- Servo 3 thrown angle (float) */
#define EEPROM_SERVO4_THROWN_ANGLE 72 /* 4 byte -- Servo 4 thrown angle (float) */
#define EEPROM_SERVO5_THROWN_ANGLE 76 /* 4 byte -- Servo 5 thrown angle (float) */
#define EEPROM_SERVO6_THROWN_ANGLE 80 /* 4 byte -- Servo 6 thrown angle (float) */
#define EEPROM_SERVO7_THROWN_ANGLE 84 /* 4 byte -- Servo 7 thrown angle (float) */
#define EEPROM_SERVO8_THROWN_ANGLE 88 /* 4 byte -- Servo 8 thrown angle (float) */
#define EEPROM_LOCK_ADDR_BASE      90
#define EEPROM_LOCK1_ADDR          92
#define EEPROM_LOCK2_ADDR          94
#define EEPROM_LOCK3_ADDR          96
#define EEPROM_LOCK4_ADDR          98
#define EEPROM_LOCK5_ADDR          100
#define EEPROM_LOCK6_ADDR          102
#define EEPROM_LOCK7_ADDR          104
#define EEPROM_LOCK8_ADDR          106
#define EEPROM_LOCK_STATE_BASE     110
#define EEPROM_LOCK1_STATE         110
#define EEPROM_LOCK2_STATE         111
#define EEPROM_LOCK3_STATE         112
#define EEPROM_LOCK4_STATE         113
#define EEPROM_LOCK5_STATE         114
#define EEPROM_LOCK6_STATE         115
#define EEPROM_LOCK7_STATE         116
#define EEPROM_LOCK8_STATE         117


// CVs
#define EEPROM_CV_DECODER_ADDRESS_LSB 80 /* 1 byte -- decoder CV address */
#define EEPROM_CV_DECODER_ADDRESS_MSB 81 /* 1 byte -- decoder CV address */
#define EEPROM_CV29                   82     /* 1 byte -- CV29 value */
// These must remain in order and consecutive.
#define EEPROM_CV_HEADSERVO_BASE  83
#define EEPROM_CV_HEAD1_FUNCTION  83     /* 1 byte -- Head 1 IO Mode */
#define EEPROM_CV_HEAD2_FUNCTION  84     /* 1 byte -- Head 2 IO Mode */
#define EEPROM_CV_HEAD3_FUNCTION  85     /* 1 byte -- Head 3 IO Mode */
#define EEPROM_CV_HEAD4_FUNCTION  86     /* 1 byte -- Head 4 IO Mode */
#define EEPROM_CV_SERVO1_FUNCTION 87     /* 1 byte -- Servo 1 IO Mode */
#define EEPROM_CV_SERVO2_FUNCTION 88     /* 1 byte -- Servo 2 IO Mode */
#define EEPROM_CV_SERVO3_FUNCTION 89     /* 1 byte -- Servo IO Mode */
#define EEPROM_CV_SERVO4_FUNCTION 90     /* 1 byte -- Servo 4 IO Mode */

