#include <Arduino.h>
#include <EEPROM.h>
#include <HX711.h>
#include <MenuBackend.h>
#include "LCD.h"

#define STATE_IDLE 0
#define STATE_FIRST_TRUCK 1
#define STATE_SECOND_TRUCK 2
#define STATE_DO_MENUS 3
#define STATE_DO_MENU_ACTION 4

// Menu subsystem "stuff"
void menuUseEvent(MenuUseEvent e);
void menuChangeEvent(MenuChangeEvent e);
MenuBackend *menu = new MenuBackend(menuUseEvent, menuChangeEvent);

#define FAKE_WEIGHT 123
#define CALIBRATION_VALUE 2280.f
HX711 scale(A2, A1);

LCD lcd;
int scaleState = STATE_IDLE;

char display[2][17];

#define SCALE_REAL         0
#define SCALE_PROTO_EMPTY  1
#define SCALE_PROTO_LOADED 2

bool doOunces = false;
byte doProtoScale = SCALE_REAL;
float calValue = CALIBRATION_VALUE;

#define AVERAGING_TIMES 10

int weight;

// TODO ITEMS:
// Make Calibration action a menu selection
// Make Scale vs. Real weight a menu selection
// Store Calibration value in EEPROM (if we implement calibration action)
// Store Scale vs Real and Oz vs Gm in EEPROM
// Handle rounding properly in G->Oz conversion (right now it's floor(), not round())
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("HX711 Load Cell Test");
  EEPROM_GetAll();
  if (calValue == 0.0f) {
    // Cal Value hasn't ever been set. (for now) Set to dummy value
    calValue = CALIBRATION_VALUE;
    EEPROM_StoreCal();
  }
  Serial.println("Calibration Value: " + String(calValue));
  Serial.println("Units: " + String(doOunces ? "oz" : "g"));
  Serial.print("Scale Measure: ");
  Serial.println((doProtoScale == SCALE_REAL ? "Real" : (doProtoScale == SCALE_PROTO_EMPTY ? "Proto-Empty" : "Proto-Loaded")));
  scale.set_scale(calValue);
  scale.tare(AVERAGING_TIMES);
  weight = 0;
  menuSetup();
  lcd.begin();
  lcd.updateDisplay("Model Scale", "Select 1st Truck");
}

void loop() {
  // Run the LCD and check for a button press.
  lcd.run();
  int button = lcd.getButtons();
  if (button != KEYS_NONE) {
    Serial.println("Button = " + String(button));
  }
  
  switch(scaleState) {
  case STATE_IDLE:
  default:
    if (button == KEYS_SELECT) {
      scaleState = STATE_FIRST_TRUCK;
      // Measure truck here.
      weight = measure();
      lcd.updateDisplay("Model Scale", "Select 2nd Truck");
    } else if (button == KEYS_LONG_SELECT) {
      scaleState = STATE_DO_MENUS;
      if (menu->getCurrent() == menu->getRoot()) {
	menu->moveDown();
      }
      doMenuDisplay();
      button = KEYS_NONE;
    }
    break;
    
  case STATE_FIRST_TRUCK:
    if (button == KEYS_SELECT) {
      scaleState = STATE_SECOND_TRUCK;
      weight += measure();
      if (doOunces) {
	weight = toOunces(weight);
      }
      doDisplayWeight(weight);
    } else if (button == KEYS_LONG_SELECT) {
      scaleState = STATE_DO_MENUS;
      if (menu->getCurrent() == menu->getRoot()) {
	menu->moveDown();
      }
      doMenuDisplay();
      button = KEYS_NONE;
    }
    break;
    
  case STATE_SECOND_TRUCK:
    if (button == KEYS_SELECT) {
      scaleState = STATE_IDLE;
      weight = 0;
      lcd.updateDisplay("Model Scale", "Select 1st Truck");
    } else if (button == KEYS_LONG_SELECT) {
      scaleState = STATE_DO_MENUS;
      if (menu->getCurrent() == menu->getRoot()) {
	menu->moveDown();
      }
      doMenuDisplay();
      button = KEYS_NONE;
    }
    break;
    
  case STATE_DO_MENUS:
    switch(button) {
    case KEYS_DOWN:
    case KEYS_RIGHT:
      menu->moveDown();
      doMenuDisplay();
      break;
    case KEYS_UP:
    case KEYS_LEFT:
      menu->moveUp();
      doMenuDisplay();
      break;
    case KEYS_SELECT:
      scaleState = STATE_DO_MENU_ACTION;
      doMenuAction(button);
      break;
    case KEYS_LONG_SELECT:
      scaleState = STATE_IDLE;
      weight = 0;
      lcd.updateDisplay("Model Scale", "Select 1st Truck");
      break;
    }
    break;
    
  case STATE_DO_MENU_ACTION:
    switch(button) {
    case KEYS_LEFT:
    case KEYS_SELECT:
      EEPROM_StoreAll();
      scaleState = STATE_DO_MENUS;
      doMenuDisplay();
      break;
      
    case KEYS_UP:
    case KEYS_DOWN:
      doMenuAction(button);
      break;
      
    case KEYS_LONG_SELECT:
      EEPROM_StoreAll();
      scaleState = STATE_IDLE;
      if (menu->getCurrent() == menu->getRoot()) {
	menu->moveDown();
      }
      lcd.updateDisplay("Model Scale", "Select 1st Truck");
      button = KEYS_NONE;
      break;
      
    default:
      break;
    }
    break;
  }
}

  int measure() {
  // Measure the weight here.
  // return 10x the weight in oz.
  //return(scale.get_units(AVERAGING_TIMES));
  return(FAKE_WEIGHT);
}

 int toOunces(int grams) {
   long lounces = (((long)grams) * 10000) / 28350;
   Serial.println("g = " + String(grams) + " oz = " + String(lounces));
   return((int) ((lounces) & 0xFFFF));
 }

#define MIN_EMPTY_WEIGHT_LBS 50000
#define MIN_LOADED_WEIGHT_LBS 100000
#define EMPTY_RANGE_LBS 20000
#define LOADED_RANGE_LBS 80000
#define MIN_NMRA_OZ 3
#define NMRA_RANGE_OZ 3

#define MIN_EMPTY_WEIGHT_KG 22727
#define MIN_LOADED_WEIGHT_KG 45455
#define EMPTY_RANGE_KG 9090
#define LOADED_RANGE_KG 36364
#define MIN_NMRA_G 28
#define NMRA_RANGE_G 85

void doDisplayWeight(int weight) {
  if (doProtoScale == SCALE_PROTO_EMPTY) {
    if (doOunces == true) {
      // Scale up by some factor that gets us in the ballpark of a real car.
      long dweight = ((long)weight - (MIN_NMRA_OZ *10)) * EMPTY_RANGE_LBS / (NMRA_RANGE_OZ*10) + MIN_EMPTY_WEIGHT_LBS;
      sprintf(display[0], "%ld lb", dweight);
    } else {
      // Scale up by some factor that gets us in the ballpark of a real car.
      long dweight = ((long)weight - (MIN_NMRA_G *10)) * EMPTY_RANGE_KG / (NMRA_RANGE_G*10) + MIN_EMPTY_WEIGHT_KG;
      sprintf(display[0], "%ld kg", dweight);
    }
  } else if (doProtoScale == SCALE_PROTO_LOADED) {
    if (doOunces == true) {
      // Scale up by some factor that gets us in the ballpark of a real car.
      long dweight = ((long)weight - (MIN_NMRA_OZ *10)) * LOADED_RANGE_LBS / (NMRA_RANGE_OZ*10) + MIN_LOADED_WEIGHT_LBS;
      sprintf(display[0], "%ld lb", dweight);
    } else {
      // Scale up by some factor that gets us in the ballpark of a real car.
      long dweight = ((long)weight - (MIN_NMRA_G *10)) * LOADED_RANGE_KG / (NMRA_RANGE_G*10) + MIN_LOADED_WEIGHT_KG;
      sprintf(display[0], "%ld kg", dweight);
    }
  } else {
    // Default to real values.
    sprintf(display[0],"%2d.%1d %s", weight/10, weight%10, (doOunces ? "oz" : "g"));
  }
  lcd.updateDisplay("Weight:", display[0]);
}

void menuSetup() {
  MenuItem *miSetUnits = new MenuItem("SetUnits");
  MenuItem *miTare = new MenuItem("Tare");
  MenuItem *miScaleReal = new MenuItem("ScaleReal");
  MenuItem *miCalibrate = new MenuItem("Calibrate");
  menu->getRoot().add(*miTare);
  miTare->add(*miSetUnits);
  miSetUnits->add(*miScaleReal);
  miScaleReal->add(*miCalibrate);
}

void doMenuDisplay() {
  if (menu->getCurrent() == "Tare") {
    lcd.updateDisplay("MENU:","Tare Scale");
  }
  if (menu->getCurrent() == "SetUnits") {
    lcd.updateDisplay("MENU:", "Set Units");
  }
  if (menu->getCurrent() == "ScaleReal") {
    lcd.updateDisplay("MENU:", "Scale vs Proto");
  }
  if (menu->getCurrent() == "Calibrate") {
    lcd.updateDisplay("MENU:", "Calibrate Scale");
  }
}

void doMenuAction(int button) {
  Serial.println("doMenuAction(" + String(button) + ")");
  if (menu->getCurrent() == "Tare") {
    scale.tare(AVERAGING_TIMES);
    lcd.updateDisplay("Action:","Scale Tared");
  }
  if (menu->getCurrent() == "SetUnits") {
    // Note the first time this is called it will be with KEYS_SELECT, so
    // the updateDisplay will show the current state.  Subsequent calls
    // may be with KEYS_UP or KEYS_DOWN which will toggle, then show the
    // changed state.  I hope.
    if (button == KEYS_UP || button == KEYS_DOWN) {
      doOunces = !doOunces;
      Serial.print("toggled ");
    }
    Serial.print(doOunces);
    Serial.print("doOunces = " + String(doOunces? "oz" : "g"));
    lcd.updateDisplay("Set Units:", (doOunces ? "Ounces" : "Grams"));
  }
  if (menu->getCurrent() == "ScaleReal") {
    if (button == KEYS_UP) {
      doProtoScale = (doProtoScale == 0 ? SCALE_PROTO_LOADED : doProtoScale - 1);
    } else if (button == KEYS_DOWN) {
      doProtoScale = (doProtoScale == SCALE_PROTO_LOADED ? SCALE_REAL : doProtoScale + 1);
    }
    lcd.updateDisplay("Scale Type:",
		       (doProtoScale == SCALE_REAL ? "Real Units" :
		       doProtoScale == SCALE_PROTO_EMPTY ? "Proto Empty" : "Proto Loaded"));
  }
  if (menu->getCurrent() == "Calibrate") {
    if (button == KEYS_UP) {
      calValue = calValue + 1.0f;
      scale.set_scale(calValue);
    } else if (button == KEYS_DOWN) {
      calValue = calValue - 1.0f;
      scale.set_scale(calValue);
    }
    // This is the real call
    //sprintf(display[1], "%2.2f", scale.get_units(AVERAGING_TIMES));
    // This is the fake test call
    sprintf(display[1], "%s", String(calValue).c_str());
    Serial.println("cal: " + String(calValue) + " sp: " + String(display[1]));
    lcd.updateDisplay("Calibrate:", display[1]);
  }
}

//---------------------------------------------------------------
// MenuBackend support functions

void menuChangeEvent(MenuChangeEvent changed) {
  // Update the display to reflect the current menu state
  // For now we'll use serial output.
  Serial.print("Menu change ");
  Serial.print(changed.from.getName());
  Serial.print(" -> ");
  Serial.println(changed.to.getName());
}

void menuUseEvent(MenuUseEvent used) {
  //Serial.print("Menu use ");
  //Serial.println(used.item.getName());
}

//---------------------------------------------------------------
// EEPROM Interface Functions

// Memory Locations (byte address)
#define EEPROM_BASE 0
#define EEPROM_UNITS (EEPROM_BASE)
#define EEPROM_SCALE (EEPROM_UNITS + sizeof(byte))
// Note: CAL is 4 bytes
#define EEPROM_CAL   (EEPROM_SCALE + sizeof(byte)) 
#define EEPROM_NEXT  (EEPROM_CAL + sizeof(float))

void EEPROM_StoreAll() {
  EEPROM_StoreUnits();
  EEPROM_StoreScale();
  EEPROM_StoreCal();
}

void EEPROM_GetAll() {
  EEPROM_GetUnits();
  EEPROM_GetScale();
  EEPROM_GetCal();
}

void EEPROM_StoreUnits() {
  byte oz = (doOunces == true ? 1 : 0);
  EEPROM.put(EEPROM_UNITS, oz);
}

void EEPROM_GetUnits() {
  byte oz;
  EEPROM.get(EEPROM_UNITS, oz);
  doOunces = ((oz == 1) ? true : false);
}

void EEPROM_StoreScale() {
  EEPROM.put(EEPROM_SCALE, doProtoScale);
}

void EEPROM_GetScale() {
  EEPROM.get(EEPROM_SCALE, doProtoScale);
}

void EEPROM_StoreCal() {
  EEPROM.put(EEPROM_CAL, calValue);

}

void EEPROM_GetCal() {
  EEPROM.get(EEPROM_CAL, calValue);
}


