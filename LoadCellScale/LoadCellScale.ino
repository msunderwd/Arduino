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

// Const Strings for repetitive Display stuff
const char ModelScaleString[] = "Model Scale";
const char TruckOneString[] = "Select 1st Truck";
const char TruckTwoString[] = "Select 2nd Truck";
const char WeightString[] = "Weight:";
const char MenuString[] = "MENU:";
const char ScaleTypeString[] = "Scale Type:";
const char RealUnitsString[] = "Real Units";
const char ProtoEmptyString[] = "Proto Empty";
const char ProtoLoadedString[] = "Proto Loaded";
const char CalibrateString[] = "Calibrate:";
const char TareMenuItem[] = "Tare";
const char SetUnitsMenuItem[] = "SetUnits";
const char ScaleRealMenuItem[] = "ScaleReal";
const char CalibrateMenuItem[] = "Calibrate";
  



// TODO ITEMS:
// Handle rounding properly in G->Oz conversion (right now it's floor(), not round())

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println(F("HX711 Load Cell Test"));
  EEPROM_GetAll();
  if (calValue == 0.0f) {
    // Cal Value hasn't ever been set. (for now) Set to dummy value
    calValue = CALIBRATION_VALUE;
    EEPROM_StoreCal();
  }
  Serial.print(F("Calibration Value: "));
  Serial.println(calValue);
  Serial.print(F("Units: "));
  Serial.println(doOunces ? "oz" : "g");
  Serial.print(F("Scale Measure: "));
  Serial.println((doProtoScale == SCALE_REAL ? "Real" : (doProtoScale == SCALE_PROTO_EMPTY ? "Proto-Empty" : "Proto-Loaded")));
  scale.set_scale(calValue);
  scale.tare(AVERAGING_TIMES);
  weight = 0;
  menuSetup();
  lcd.begin();
  lcd.updateDisplay(ModelScaleString, TruckOneString);
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
      lcd.updateDisplay(ModelScaleString, TruckTwoString);
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
      lcd.updateDisplay(ModelScaleString, TruckOneString);
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
      lcd.updateDisplay(ModelScaleString, TruckOneString);
      break;
    }
    break;
    
  case STATE_DO_MENU_ACTION:
    switch(button) {
    case KEYS_SELECT:
      EEPROM_StoreAll();
      scaleState = STATE_DO_MENUS;
      lcd.noCursor();
      doMenuDisplay();
      break;
      
    case KEYS_UP:
    case KEYS_DOWN:
    case KEYS_LEFT:
    case KEYS_RIGHT:
      doMenuAction(button);
      break;
      
    case KEYS_LONG_SELECT:
      EEPROM_StoreAll();
      scaleState = STATE_IDLE;
      lcd.noCursor();
      if (menu->getCurrent() == menu->getRoot()) {
	menu->moveDown();
      }
      lcd.updateDisplay(ModelScaleString, TruckOneString);
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
  lcd.updateDisplay(WeightString, display[0]);
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
  if (menu->getCurrent() == TareMenuItem) {
    lcd.updateDisplay(MenuString, "Tare Scale");
  }
  if (menu->getCurrent() == SetUnitsMenuItem) {
    lcd.updateDisplay(MenuString, "Set Units");
  }
  if (menu->getCurrent() == ScaleRealMenuItem) {
    lcd.updateDisplay(MenuString, "Scale vs Proto");
  }
  if (menu->getCurrent() == CalibrateMenuItem) {
    lcd.updateDisplay(MenuString, "Calibrate Scale");
  }
}

float incval;

void doMenuAction(int button) {
  static float incval = 1.0f;
  Serial.println("doMenuAction(" + String(button) + ")");
  if (menu->getCurrent() == TareMenuItem) {
    scale.tare(AVERAGING_TIMES);
    lcd.updateDisplay("Action:", "Scale Tared");
  }
  if (menu->getCurrent() == SetUnitsMenuItem) {
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
  if (menu->getCurrent() == ScaleRealMenuItem) {
    if (button == KEYS_UP) {
      doProtoScale = (doProtoScale == 0 ? SCALE_PROTO_LOADED : doProtoScale - 1);
    } else if (button == KEYS_DOWN) {
      doProtoScale = (doProtoScale == SCALE_PROTO_LOADED ? SCALE_REAL : doProtoScale + 1);
    }
    lcd.updateDisplay(ScaleTypeString,
		      (doProtoScale == SCALE_REAL ? RealUnitsString :
		       doProtoScale == SCALE_PROTO_EMPTY ? ProtoEmptyString : ProtoLoadedString));
  }
  if (menu->getCurrent() == CalibrateMenuItem) {
    // Find the decimal... a bit klugey, but
    static float incval;
    if (button == KEYS_SELECT) {
      incval = pow(10.0f, (String(calValue).indexOf('.') * 1.0f) - 1);
    } else if (button == KEYS_UP) {
      calValue = calValue + incval;
      scale.set_scale(calValue);
    } else if (button == KEYS_DOWN) {
      calValue = calValue - incval;
      scale.set_scale(calValue);
    } else if (button == KEYS_LEFT) {
      incval = calcIncVal(incval, calValue, true);
    } else if (button == KEYS_RIGHT) {
      incval = calcIncVal(incval, calValue, false);
    }
    Serial.println("button: " + String(button) + " incval: " + String(incval));
    // This is the real call
    //sprintf(display[1], "%2.2f", scale.get_units(AVERAGING_TIMES));
    // This is the fake test call
    sprintf(display[1], "%s", String(calValue).c_str());
    Serial.println("cal: " + String(calValue) + " sp: " + String(display[1]));
    lcd.updateDisplay(CalibrateString, String(calValue).c_str());
    Serial.println("incval = " + String(incval));
    setCursor(incval, calValue);
  }
}

void setCursor(float i, float c) {
  if (i < 1) {
    lcd.setCursor(String(i).indexOf('1')+1, 1);
  } else {
    String s = String(i);
    String t = String(c);
    int d = t.indexOf('.') - s.indexOf('.');
    lcd.setCursor(d, 1);
  }
  lcd.cursor();
}

float calcIncVal(float i, float x, bool inc) {
  if (inc) {
    if (i * 10.0f > x) { return(i); } else { return(i*10); }
  } else {
    if (i / 10.0f < 0.009f) { return(i); } else { return(i/10.0f); }
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


