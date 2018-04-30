#include <AccelStepper.h>
#include <MultiStepper.h>
#include <EEPROM.h>
#include "config.h"
#include "eemap.h"

#define SERIAL_PORT_BUTTON 0
// CHOOSE ONLY ONE (Defaults to MEGA)
#define ARDUINO_PRO_MINI_PCB 1

#define CONTINUOUS_MODE 0

// Generic stepper drive pins
// Customize when implemented
// Pin 2 : A
// Pin 3 : B
// Pin 4 : C
// Pin 5 : D

// Artdumoto Driver Control pins (standard names)
// Pin 3  : A Speed
// Pin 11 : B speed
// Pin 12 : A Direction
// Pin 13 : B Direction

// EasyDriver MS1/MS2 Settings:
// MS1 | MS2 | Microstep Resolution
//  L  |  L  | Full step (2-phase)
//  H  |  L  | Half Step
//  L  |  H  | Quarter Step
//  H  |  H  | Eighth Step

#if (ARDUINO_PRO_MINI_PCB > 0)

// PCB APM Pinout
#define MOTOR_PIN1 4 // Step
#define MOTOR_PIN2 5 // Dir
#define PIN_HOME     6
#define PIN_BUTTON1  9
#define PIN_BUTTON2  8
#define PIN_ENABLE   7 // Active LOW
#define PIN_SLEEP     14 // Hight to SLEEP, Low to Wake
#define PIN_MS1       13
#define PIN_MS2       10
#define PIN_PFD       16

#else

// Assume Arduino UNO
#define MOTOR_PIN1   2
#define MOTOR_PIN2   3
#define MOTOR_PIN3   4
#define MOTOR_PIN4   5
#define PIN_HOME     6
#define PIN_BUTTON1   7

#endif

// Stepper motor parameters
// Moved to config.h
// Pinout: nA A B nB
//#define STEPS_PER_REV 32
//#define GEAR_RATIO    64 // Approximately
//#define MOTOR_RPM    200
//#define STEPS_PER_TURNTABLE_REV (STEPS_PER_REV * GEAR_RATIO)
//#define MAX_SPEED 1000

int current_position = 0; // Current track position of turntable
int requested_position = 0;
int num_positions = MAX_NUM_POSITIONS;
int snapTarget = 0;
boolean isMoving = false; // True if the motor is running currently
boolean isSnapping = false; // True if motor is snapping to a position
boolean button_pressed[2];  // True if the button has been pressed but not yet released
int button_pin[2] = { PIN_BUTTON1, PIN_BUTTON2};
#define DIR_CW 1
#define DIR_CCW (-1)
int direction = DIR_CW; // 1 = clockwise, -1 = counterclockwise
int active_button;

// Position Information
#define NUM_POSITIONS 16
// These are placeholders. The real positions need to be stored in EEPROM
// because they can change and are not likely to be evenly spaced.
// position indexes go up in CW direction.
long positions[NUM_POSITIONS] = {
  0,    // 0
  128,  // 1
  256,  // 2
  384,  // 3
  512,  // 4
  640,  // 5
  768,  // 6
  896,  // 7
  1024, // 8
  1152, // 9
  1280, // 10
  1408, // 11
  1536, // 12
  1664, // 13
  1792, // 14
  1920  // 15
};

const char IDLE = 0;
const char MOVING = 1;
const char STOPPING = 2;
const char SNAPPING = 3;
const char BUTTON_DOWN = 4;

char myState = IDLE;

#if ((ARDUINO_PRO_MINI_PCB > 0))
AccelStepper myMotor(AccelStepper::DRIVER, MOTOR_PIN1, MOTOR_PIN2);
#else
AccelStepper myMotor(AccelStepper::HALF4WIRE,
                     MOTOR_PIN1, MOTOR_PIN2, MOTOR_PIN3, MOTOR_PIN4, true); // check pins
#endif
//AccelStepper myMotor;

boolean isButtonPressed();

void setup() {
  // Set up the serial port
  Serial.begin(9600);
  
  // Set up the button and home sensor inputs
  pinMode(PIN_HOME, INPUT_PULLUP);
  pinMode(PIN_BUTTON1, INPUT_PULLUP);
  pinMode(PIN_BUTTON2, INPUT_PULLUP);

  // Enable the driver outputs
  digitalWrite(MOTOR_PIN1, HIGH);
  digitalWrite(MOTOR_PIN2, HIGH);
#if (ARDUINO_PRO_MINI_PCB > 0)
  pinMode(PIN_ENABLE, OUTPUT);
  pinMode(PIN_SLEEP, OUTPUT);
  pinMode(PIN_MS1, OUTPUT);
  pinMode(PIN_MS2, OUTPUT);
  digitalWrite(PIN_MS1, HIGH);
  digitalWrite(PIN_MS2, HIGH);
  digitalWrite(PIN_SLEEP, LOW);   // Sleep == HIGH
  digitalWrite(PIN_ENABLE, LOW);  // Enable == LOW
  myMotor.setEnablePin(0xFF); // I don't trust the library's handling of this
#else
  digitalWrite(MOTOR_PIN3, HIGH);
  digitalWrite(MOTOR_PIN4, HIGH);
#endif


  // Set up the motor control
  myMotor.setMaxSpeed(MAX_SPEED);
  myMotor.setSpeed(MOTOR_RPM);
  myMotor.setAcceleration(200);

  myMotor.moveTo(positions[0]);

  // Set logic state variables to defaults.
  isMoving = false;
  isSnapping = false;
  button_pressed[0] = false;
  button_pressed[1] = false;

  Serial.println(F("Setup Complete."));
}

void loop() {
  // put your main code here, to run repeatedly:

  if (Serial.available()) {
    handleSerialInput();
  }
  
  // Stop if we are at the home sensor, whatever we are doing.
  if ((digitalRead(PIN_HOME) == LOW) && false) {
    Serial.println("Stopping and setting home!");
    myMotor.stop();
    myMotor.setCurrentPosition(positions[0]);
    isMoving = false;
    isSnapping = false;
  }

  if (CONTINUOUS_MODE > 0) {
    runContinuousModeStateMachine();
  } else {
    runIncrementalModeStateMachine();
  }

    
  myMotor.run();
}

void runContinuousModeStateMachine() {
  switch(myState) {
    case IDLE:
      if (isButtonPressed(0)) {
        direction = DIR_CW;
        active_button = 0;
        // Move at most one full revolution.
        // Button release will cut this short.
        myMotor.move(direction *STEPS_PER_TURNTABLE_REV);
        myState = MOVING;
      }
      else if (isButtonPressed(1)) {
        direction = DIR_CCW;
        active_button = 1;
        // Move at most one full revolution.
        // Button release will cut this short.
        myMotor.move(direction *STEPS_PER_TURNTABLE_REV);
        myState = MOVING;
      }
      break;
    
    case MOVING:
      if (isButtonReleased(active_button)) {
        myMotor.stop();
        myState = STOPPING;
        Serial.println("Stopping...");
      }
      break;
      
    case STOPPING:
      if (myMotor.distanceToGo() == 0) {
        long current_pos = myMotor.currentPosition();
        Serial.println("Stopped at " + String(current_pos));
        long snap_target;
        if (current_pos < 0) {
          Serial.println("Current pos = " + String(current_pos));
          Serial.println("Snapping to zero.");
          myMotor.moveTo(0);
        }
        else if (current_pos > positions[NUM_POSITIONS-1]) {
          // We're past the last position, need to snap back to position 0
          snapTarget = 0;
          myState = SNAPPING;
          //myMotor.moveTo(positions[snapTarget]);
          Serial.println("Snapping to 0");
        } else {
          for (int i = 0; i < NUM_POSITIONS-1; i++) {
            if (current_pos == positions[i]) {
              // We stopped exactly on a position. Skip the snap action
              myState = IDLE;
              Serial.println("Stopped on " + String(i));
              break;
            } else if (current_pos > positions[i] && current_pos < positions[i+1]) {
              // We are between steps. Snap to the next one.
              Serial.println("Snapping to " + String(i+1) + " (" + String(positions[i+1]) + ")" );
              //Serial.println(i+1);
              snapTarget = i+1;
              myState = SNAPPING;
              myMotor.moveTo(positions[snapTarget]);
            } else {
              //Serial.println("Not at " + String(i));
            }
          }
        }
      }
      break;
      
    case SNAPPING:
      if (myMotor.distanceToGo() == 0) {
        myState = IDLE;
        Serial.println("Snapped to " + String(snapTarget));
      }
  }
  
} // end runContinuousModeStateMachine()

void runIncrementalModeStateMachine() {
  long steps_to_take;

  switch(myState) {
  case IDLE:
    if (isButtonPressed(0)) {
      active_button = 0;
      myState = BUTTON_DOWN;
    } else if (isButtonPressed(1)) {
      active_button = 1;
      myState = BUTTON_DOWN;
    }
    break;
  case BUTTON_DOWN:
    if (isButtonReleased(active_button)) {
      if (active_button == 0) {
        // Run clockwise one position
        // First figure out requested position
        requested_position = current_position + 1;
        if (requested_position == num_positions) {
          requested_position = 0;
          steps_to_take = STEPS_PER_TURNTABLE_REV + positions[0] - positions[current_position];
        } else {
          steps_to_take = positions[requested_position] - positions[current_position];
        }
      }
      else if (active_button == 1) {
        // Run counterclockwise one position
        // First figure out requested position
        requested_position = current_position - 1;
        if (requested_position < 0) {
          requested_position = num_positions - 1;
          steps_to_take = -STEPS_PER_TURNTABLE_REV + positions[num_positions-1] - positions[current_position];
        } else {
          steps_to_take = positions[requested_position] - positions[current_position];
        }
      }
      myMotor.move(steps_to_take);
      myState = MOVING;
    } 
    break;

  case MOVING:
    if (myMotor.distanceToGo() == 0) {
      current_position = requested_position;
      myState = IDLE;
    }
    
  }
}

boolean isButtonPressed(int button) {
  // Implement a falling-edge detector.
  if (!button_pressed[button] && (digitalRead(button_pin[button]) == LOW) ) {
    Serial.println("Button " + String(button) + " is pressed");
    button_pressed[button] = true;
    return(true);
  } else {
    return(false);
  }
}

boolean isButtonReleased(int button) {
  // Implement a rising-edge detector.
  if (button_pressed[button] && (digitalRead(button_pin[button]) == HIGH) ) {
    Serial.println("Button " + String(button) + " is released");
    button_pressed[button] = false;
    return(true);
  } else {
    return(false);
  }
}

void handleSerialInput() {
  String cmd = Serial.readStringUntil(' ');
  if (cmd.equals("SN")) {
    Serial.println(F("Received set number of positions command"));
    int num = Serial.readStringUntil(' ').toInt();
    if (num < 0 || num > MAX_NUM_POSITIONS) {
      Serial.println(F("Sorry, invalid value. Ignoring."));
    }
    if (num < num_positions) {
      Serial.print(F("Warning: Reducing the number of positions might"));
      Serial.println(F(" make some positions unreachable."));
    }
    num_positions = num;
    storeNumberOfPositionsToEEPROM(num);
  } else if (cmd.equals("SP")) {
    Serial.println(F("Received set position command"));
    int num = Serial.readStringUntil(' ').toInt();
    int val = myMotor.currentPosition();
    if (num < 0 || num > num_positions) {
      Serial.print(F("Invalid position index: ")); Serial.print(num); Serial.println(F(" skipping..."));
      return;
    }
    Serial.print(F("Setting position ")); Serial.print(num); Serial.print(F(" to ")); Serial.println(val);
    storePositionToEEPROM(num, val);
  } else if (cmd.equals("CW")) {
    long num = Serial.readStringUntil(' ').toInt();
    Serial.print(F("Moving CW ")); Serial.print(num); Serial.print(F(" steps."));
    myMotor.move(num);
  } else if (cmd.equals("CCW")) {
    long num = Serial.readStringUntil(' ').toInt();
    Serial.print(F("Moving CCW ")); Serial.print(num); Serial.print(F(" steps."));
    myMotor.move(-num);
  } else if (cmd.equals("H")) {
    printHelp();
  }
}

void printHelp() {
  Serial.println(F("Stepper Motor Turntable"));
  Serial.println(F("\tSN <val> : Set number of tracks"));
  Serial.println(F("\tSP <num> : Set current position as track # <num> (tracks 0 to N-1)"));
  Serial.println(F("\tCW <val> : Rotate table Clockwise <val> motor steps"));
  Serial.println(F("\tCCW <val> : Rotate table Counterclockwise <val> motor steps"));
  Serial.println(F("H : Print this help message"));
}
void loadPositionsFromEEPROM() {
  num_positions = EEPROM.read(EEPROM_NUM_POSITIONS);
  for (int i = 0; i < num_positions; i++) {
    EEPROM.get(EEPROM_POSITION_BASE + (i*sizeof(long)), positions[i]); 
  }
}

void storePositionToEEPROM(int pos, int val) {
  EEPROM.put(EEPROM_POSITION_BASE + (pos*sizeof(long)), val);
}

void storeNumberOfPositionsToEEPROM(int num) {
  EEPROM.put(EEPROM_NUM_POSITIONS, num);
}

