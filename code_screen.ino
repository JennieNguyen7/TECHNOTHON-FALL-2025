#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'}, // 'A' as feet separator
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// ✅ Use ESP32-safe pins
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// User data
String weightInput = "";
String feetInput = "";
String inchesInput = "";
float weight_lb = 0;
int heightFeet = 0;
int heightInches = 0;
int inputStage = 0; // 0=weight, 1=height feet, 2=height inches, 3=done

void setup() {
  Serial.begin(115200);

  // ✅ Initialize I2C pins manually
  Wire.begin(21, 22);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found!");
    for(;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Enter weight (lb):");
  display.display();
}

void loop() {
  char key = keypad.getKey();
  if (!key) return; // only process if a key is pressed

  if (inputStage == 0) {
    if (key >= '0' && key <= '9') weightInput += key;
    else if (key == '*') { if (weightInput.length()>0) weightInput.remove(weightInput.length()-1); }
    else if (key == '#') {
      weight_lb = weightInput.toFloat();
      inputStage = 1;
      feetInput = "";
      displayHeightFeetScreen();
      return;
    }
    displayWeightScreen();
  }
  else if (inputStage == 1) {
    if (key >= '0' && key <= '9') feetInput += key;
    else if (key == '*') { if (feetInput.length()>0) feetInput.remove(feetInput.length()-1); }
    else if (key == 'A') {
      heightFeet = feetInput.toInt();
      inputStage = 2;
      inchesInput = "";
      displayHeightInchesScreen();
      return;
    }
    displayHeightFeetScreen();
  }
  else if (inputStage == 2) {
    if (key >= '0' && key <= '9') inchesInput += key;
    else if (key == '*') { if (inchesInput.length()>0) inchesInput.remove(inchesInput.length()-1); }
    else if (key == '#') {
      heightInches = inchesInput.toInt();
      inputStage = 3;
      displayResults();
      return;
    }
    displayHeightInchesScreen();
  }
}

// ---------------- Display Functions ----------------
void displayWeightScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Weight (lb):");
  display.setTextSize(2);
  display.setCurs"Height (in):");
  display.setTextSize(2);
  display.setCursor(0,20);
  display.println(inchesInput);
  display.display();
}

void displayResults() {
  float totalInches = heightFeet * 12 + heightInches;
  float height_m = totalInches * 0.0254;
  float weight_kg = weight_lb * 0.453592;
  float bmi = weight_kg / (height_m * height_m);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Data Saved!");
  display.print("Weight: "); display.print(weight_lb); display.println(" lb");
  display.print("Height: "); display.print(heightFeet); display.print("'"); display.print(heightInches); display.println("\"");
  display.print("BMI: "); display.println(bmi,1);
  display.display();

  Serial.print("Weight: "); Serial.println(weight_lb);
  Serial.print("Height: "); Serial.print(heightFeet); Serial.print("' "); Serial.println(heightInches);
  Serial.print("BMI: "); Serial.println(bmi);
}or(0,20);
  display.println(weightInput);
  display.display();
}

void displayHeightFeetScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Height (ft):");
  display.setTextSize(2);
  display.setCursor(0,20);
  display.println(feetInput);
  display.display();
}
void displayHeightInchesScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Height (in):");
  display.setTextSize(2);
  display.setCursor(0,20);
  display.println(inchesInput);
  display.display();
}

void displayResults() {
  float totalInches = heightFeet * 12 + heightInches;
  float height_m = totalInches * 0.0254;
  float weight_kg = weight_lb * 0.453592;
  float bmi = weight_kg / (height_m * height_m);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Data Saved!");
  display.print("Weight: "); display.print(weight_lb); display.println(" lb");
  display.print("Height: "); display.print(heightFeet); display.print("'"); display.print(heightInches); display.println("\"");
  display.print("BMI: "); display.println(bmi,1);
  display.display();

  Serial.print("Weight: "); Serial.println(weight_lb);
  Serial.print("Height: "); Serial.print(heightFeet); Serial.print("' "); Serial.println(heightInches);
  Serial.print("BMI: "); Serial.println(bmi);
}