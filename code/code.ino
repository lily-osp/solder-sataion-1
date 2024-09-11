#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "BigNumbers_I2C.h"
#include <avr/wdt.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

// Pin Definitions
#define TEMP_SENSOR_PIN A0
#define IRON_PIN 10
#define WS2812_PIN 12
#define BUZZ_PIN 2
#define ENCODER_CLK_PIN 3
#define ENCODER_DT_PIN 4
#define BUTTON_PIN 5
#define LED_OFF_PIN 8
#define EEPROM_TEMP_ADDRESS 0

// Constants
const int MIN_TEMP = 28;
const int MAX_TEMP = 500;
const int MIN_ADC = 0;
const int MAX_ADC = 290;
const int MAX_PWM = 255;
const int AVG_COUNTS = 20;
const int LCD_INTERVAL = 80;
const int MIN_KNOB = 100;
const int MAX_KNOB = 500;
const unsigned long AUTO_SHUTOFF_TIME = 10 * 60000; // 10 minutes in milliseconds

// LED Colors
const uint32_t COLOR_OFF = 0x000000;      // Black (OFF)
const uint32_t COLOR_HEATING = 0xFF0000;  // Red (Heating)
const uint32_t COLOR_READY = 0x00FF00;    // Green (Ready)
const uint32_t COLOR_COOLING = 0x0000FF;  // Blue (Cooling)
const uint32_t COLOR_WARNING = 0xFFFF00;  // Yellow (Warning)

// Global Variables
LiquidCrystal_I2C lcd(0x27, 16, 2);
BigNumbers_I2C bigNum(&lcd);
Adafruit_NeoPixel pixel(1, WS2812_PIN, NEO_GRB + NEO_KHZ800);

volatile int knob = 100;
int pwm = 0;
int tempRaw = 0;
int counter = 0;
int currentTempAvg = 0;
unsigned long previousMillis = 0;
unsigned long lastActivityTime = 0;
float currentTemp = 0.0;
float store = 0.0;

bool ledOffState = true;
int lastButtonState = HIGH;

void setup() {
  initializeWatchdog();
  initializePins();
  initializeLCD();
  initializeWS2812();
  loadSavedTemperature();
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), encoderISR, FALLING);
  lastActivityTime = millis();
}

void loop() {
  wdt_reset(); // Reset watchdog timer

  readTemperature();
  updateTemperatureAverage();
  handleButtonPress();
  controlIronAndLED();
  updateLCD();
  checkAutoShutoff();
}

void initializeWatchdog() {
  wdt_disable(); // Disable watchdog while setting it up
  wdt_enable(WDTO_4S); // Enable watchdog with 4-second timeout
}

void initializePins() {
  pinMode(TEMP_SENSOR_PIN, INPUT);
  pinMode(IRON_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_OFF_PIN, OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(ENCODER_DT_PIN, INPUT);

  digitalWrite(LED_OFF_PIN, HIGH);
}

void initializeLCD() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  bigNum.begin();
  lcd.setCursor(1, 0);
  lcd.print("SET");
}

void initializeWS2812() {
  pixel.begin();
  pixel.setBrightness(50); // Set to 50% brightness
  pixel.show(); // Initialize all pixels to 'off'
}

void loadSavedTemperature() {
  int savedTemp = EEPROM.read(EEPROM_TEMP_ADDRESS) << 8 | EEPROM.read(EEPROM_TEMP_ADDRESS + 1);
  if (savedTemp >= MIN_KNOB && savedTemp <= MAX_KNOB) {
    knob = savedTemp;
  }
}

void saveTemperature() {
  EEPROM.update(EEPROM_TEMP_ADDRESS, knob >> 8);
  EEPROM.update(EEPROM_TEMP_ADDRESS + 1, knob & 0xFF);
}

void encoderISR() {
  if (digitalRead(ENCODER_DT_PIN) == HIGH && !ledOffState) {
    knob = constrain(knob + 10, MIN_KNOB, MAX_KNOB);
    saveTemperature();
  } else if (digitalRead(ENCODER_DT_PIN) == LOW && !ledOffState) {
    knob = constrain(knob - 10, MIN_KNOB, MAX_KNOB);
    saveTemperature();
  }
  lastActivityTime = millis(); // Update activity time
}

void readTemperature() {
  tempRaw = analogRead(TEMP_SENSOR_PIN);
  currentTemp = map(tempRaw, MIN_ADC, MAX_ADC, MIN_TEMP, MAX_TEMP);
}

void updateTemperatureAverage() {
  if (counter < AVG_COUNTS) {
    store += currentTemp;
    counter++;
  } else {
    currentTempAvg = (store / AVG_COUNTS) - 1;
    store = 0;
    counter = 0;
  }
}

void handleButtonPress() {
  int buttonState = digitalRead(BUTTON_PIN);
  if (buttonState != lastButtonState && buttonState == LOW) {
    ledOffState = !ledOffState;
    digitalWrite(LED_OFF_PIN, ledOffState);
    if (!ledOffState) {
      beep();
    }
    lastActivityTime = millis(); // Update activity time
  }
  lastButtonState = buttonState;
}

void controlIronAndLED() {
  if (ledOffState) {
    pwm = 0;
    setLEDColor(COLOR_OFF);
  } else if (currentTemp < knob - 10) {
    pwm = MAX_PWM;
    setLEDColor(COLOR_HEATING);
  } else if (currentTemp >= knob - 10 && currentTemp <= knob + 10) {
    pwm = (knob - currentTemp) * (MAX_PWM / 10);
    setLEDColor(COLOR_READY);
  } else if (currentTemp > knob + 10) {
    pwm = 0;
    setLEDColor(COLOR_COOLING);
  }

  // Warning for very high temperatures
  if (currentTemp > MAX_TEMP - 50) {
    setLEDColor(COLOR_WARNING);
  }

  analogWrite(IRON_PIN, pwm);
}

void setLEDColor(uint32_t color) {
  pixel.setPixelColor(0, color);
  pixel.show();
}

void updateLCD() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= LCD_INTERVAL) {
    previousMillis = currentMillis;

    lcd.setCursor(1, 0);
    lcd.print(ledOffState ? "OFF  " : "SET  ");

    lcd.setCursor(0, 1);
    if (ledOffState) {
      lcd.print("---");
    } else {
      lcd.print(knob);
    }
    lcd.print((char)223);
    lcd.print("C ");

    bigNum.displayLargeInt(currentTempAvg, 6, 0, 3, false);
    lcd.setCursor(15, 0);
    lcd.print("o");
  }
}

void beep() {
  for (int i = 0; i < 255; i++) {
    tone(BUZZ_PIN, 1000);
    delayMicroseconds(125);
    noTone(BUZZ_PIN);
    delayMicroseconds(125);
  }
}

void checkAutoShutoff() {
  if (!ledOffState && (millis() - lastActivityTime > AUTO_SHUTOFF_TIME)) {
    ledOffState = true;
    digitalWrite(LED_OFF_PIN, HIGH);
    setLEDColor(COLOR_OFF);
    beep(); // Alert user of auto-shutoff
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Auto Shut-off");
    lcd.setCursor(0, 1);
    lcd.print("Activated");
    delay(2000); // Show message for 2 seconds
    lastActivityTime = millis(); // Reset activity timer
  }
}
