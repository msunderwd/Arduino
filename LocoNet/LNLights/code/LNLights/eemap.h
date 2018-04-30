#define EEPROM_BASE_ADDRESS 0 /* 2 bytes -- Decoder base address */
#define EEPROM_BC_ADDRESS 2   /* 2 bytes -- Broadcast address */

#define EEPROM_DAY_SETTINGS 4    /* 4 bytes sizeof(led_t) */
#define EEPROM_NIGHT_SETTINGS 8  /* 4 bytes sizeof(led_t) */
#define EEPROM_SUNRISE_STEPS 12   /* 1 byte : number of steps in sunrise sequence  RANGE: 1 to 32*/
#define EEPROM_SUNSET_STEPS 13    /* 1 byte : number of steps in sunset sequence RANGE: 1 to 32 */
#define EEPROM_SUNRISE_INCREMENT 14 /* sizeof(int) number of minutes between sunrise table steps */
#define EEPROM_SUNSET_INCREMENT (14+sizeof(int)) /* sizeof(int) number of minutes between sunset table steps */
#define EEPROM_SUNRISE_START_TIME 20 /* sizeof(Time) fast clock time of start of sunrise sequence */
#define EEPROM_SUNSET_START_TIME  (20 + sizeof(Time)) /* sizeof(Time) fast clock time of start of sunset sequence */
#define EEPROM_MODE_SETTINGS 30

// Sunrise and Sunset tables:
// 32 entries, each sizeof(led_t).  Each entry is a step in the lighting change sequence
// from day to night (or night to day).

#define EEPROM_SUNRISE_TABLE 0x100   /* first address of sunrise sequence */
// Addresses 0x100 to 0x17F are 32 4-byte values sizeof(led_t) for the sunrise sequence
#define EEPROM_SUNSET_TABLE  0x180   /* first address of sunset sequence */
// Addresses 0x180 to 0x1FF are 32 4-byte values sizeof(led_t) for the sunset sequence

#define EEPROM_NEXT_ADDRESS 0x200 /* for reference. Next available EEPROM address */

// CVs
// TODO: IFF we use this, update the addresses!
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

