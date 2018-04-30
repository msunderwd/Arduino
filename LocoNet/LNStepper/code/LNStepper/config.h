// Configuration.

#define SERIAL_PORT_SPEED 9600

#define MAX_NUM_POSITIONS 16

// Stepper motor parameters
// Pinout: nA A B nB
#define STEPS_PER_REV 32
#define GEAR_RATIO    64 // Approximately
#define MOTOR_RPM    200
#define STEPS_PER_TURNTABLE_REV (STEPS_PER_REV * GEAR_RATIO)
#define MAX_SPEED 1000
