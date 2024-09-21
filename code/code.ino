#define USE_OLED // Comment this line to use LCD instead of OLED

#ifdef USE_OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#else
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "BigNumbers_I2C.h"
#endif

#include <avr/wdt.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <PID_v1.h>

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

// Overshoot variable
const float MAX_OVERSHOOT = 10.0; // Maximum allowed overshoot in °C
float lastSetpoint = 0;

// Adaptive PID variable
int adaptationLoopCount = 0;  // Loop counter for PID adaptation
const int ADAPT_LOOP_THRESHOLD = 225;  // Number of loops after which PID adapts
float sumError = 0;
int errorCount = 0;

// PID Variables
double Setpoint, Input, Output;
double Kp = 2, Ki = 5, Kd = 1;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

// Ramping Variables
const unsigned long RAMP_DURATION = 20000; // 20 seconds for ramping
bool isRamping = false;
unsigned long rampStartTime = 0;
int originalSetpoint = 0;

// LED Colors
const uint32_t COLOR_OFF = 0x000000;      // Black (OFF)
const uint32_t COLOR_HEATING = 0xFF0000;  // Red (Heating)
const uint32_t COLOR_READY = 0x00FF00;    // Green (Ready)
const uint32_t COLOR_COOLING = 0x0000FF;  // Blue (Cooling)
const uint32_t COLOR_WARNING = 0xFFFF00;  // Yellow (Warning)
const uint32_t COLOR_RAMPING = 0xFF00FF;  // Purple (Ramping)

// Global Variables
#ifdef USE_OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#else
LiquidCrystal_I2C lcd(0x27, 16, 2);
BigNumbers_I2C bigNum(&lcd);
#endif

Adafruit_NeoPixel pixel(1, WS2812_PIN, NEO_GRB + NEO_KHZ800);

// on run variable
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
  initializeDisplay();
  initializeWS2812();
  loadSavedTemperature();
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), encoderISR, FALLING);
  lastActivityTime = millis();

  // Initialize PID
  Input = currentTemp;
  Setpoint = knob;
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(0, MAX_PWM);
}

void loop() {
  wdt_reset();

  readTemperature();
  updateTemperatureAverage();
  handleButtonPress();
  updatePID();
  controlIronAndLED();
  updateDisplay();
  checkAutoShutoff();
  handleRamping();
  handleTemperatureOvershootProtection();
  adaptPIDParameters();
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

void initializeDisplay() {
#ifdef USE_OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Initializing...");
  display.display();
#else
  lcd.init();
  lcd.backlight();
  lcd.clear();
  bigNum.begin();
  lcd.setCursor(0, 0);
  lcd.print("SET");
#endif
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

// Temperature Overshoot Protection
void handleTemperatureOvershootProtection() {
  if (Setpoint != lastSetpoint) {
    // Setpoint has changed
    if (Setpoint > lastSetpoint) {
      // We're heating up, apply overshoot protection
      float maxAllowedTemp = Setpoint + MAX_OVERSHOOT;
      if (currentTemp > maxAllowedTemp) {
        pwm = 0; // Cut power to prevent further overshoot
      }
    }
    lastSetpoint = Setpoint;
  }
}

// Adaptive PID
void adaptPIDParameters() {
  // Increment the loop counter
  adaptationLoopCount++;

  // Adapt PID parameters after a certain number of loops
  if (adaptationLoopCount >= ADAPT_LOOP_THRESHOLD) {
    float avgError = sumError / errorCount;

    // Adjust PID parameters based on average error
    if (abs(avgError) > 5) {
      // If average error is large, increase proportional and integral terms
      Kp *= 1.1;
      Ki *= 1.1;
    } else if (abs(avgError) < 2) {
      // If average error is small, slightly decrease proportional and integral terms
      Kp *= 0.95;
      Ki *= 0.95;
    }

    // Limit Kp and Ki to reasonable ranges
    Kp = constrain(Kp, 0.5, 10);
    Ki = constrain(Ki, 0.1, 20);

    // Update PID controller with new parameters
    myPID.SetTunings(Kp, Ki, Kd);

    // Reset for next adaptation cycle
    sumError = 0;
    errorCount = 0;
    adaptationLoopCount = 0;  // Reset loop counter
  } else {
    // Accumulate error for averaging
    sumError += (Setpoint - Input);
    errorCount++;
  }
}

void updatePID() {
  Input = currentTemp;
  Setpoint = isRamping ? calculateRampSetpoint() : knob;
  myPID.Compute();
  pwm = Output;
}

void controlIronAndLED() {
  if (ledOffState) {
    pwm = 0;
    setLEDColor(COLOR_OFF);
  } else if (isRamping) {
    setLEDColor(COLOR_RAMPING);
  } else if (currentTemp < Setpoint - 10) {
    setLEDColor(COLOR_HEATING);
  } else if (currentTemp >= Setpoint - 10 && currentTemp <= Setpoint + 10) {
    setLEDColor(COLOR_READY);
  } else if (currentTemp > Setpoint + 10) {
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

void updateDisplay() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= LCD_INTERVAL) {
    previousMillis = currentMillis;

#ifdef USE_OLED
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    if (isRamping) {
      display.print("RAMP ");
      int remainingTime = (RAMP_DURATION - (currentMillis - rampStartTime)) / 1000;
      if (remainingTime < 0) remainingTime = 0;
      display.setCursor(74, 0);
      display.print(remainingTime);
      display.print("s");
    } else {
      display.print(ledOffState ? "OFF  " : "SET  ");
      display.setCursor(74, 0);
      display.print(ledOffState ? "---" : String(knob));
      display.setTextSize(1);
      display.print((char)247);
      display.print("C");
    }

    display.setTextSize(4);
    display.setCursor(0, 32);
    display.print(currentTempAvg);
    display.setTextSize(3);
    display.print((char)247);
    display.print("C");

    display.display();
#else
    lcd.setCursor(0, 0);
    if (isRamping) {
      lcd.print("RAMP ");
      int remainingTime = (RAMP_DURATION - (currentMillis - rampStartTime)) / 1000;
      if (remainingTime < 0) remainingTime = 0;
      lcd.setCursor(0, 1);
      lcd.print(remainingTime);
      lcd.print("s   ");
    } else {
      lcd.print(ledOffState ? "OFF  " : "SET  ");
      lcd.setCursor(0, 1);
      lcd.print(ledOffState ? "---" : String(knob));
      lcd.print((char)223);
      lcd.print("C ");
    }

    bigNum.displayLargeInt(currentTempAvg, 6, 0, 3, false);
    lcd.setCursor(15, 0);
    lcd.print("o");
#endif
  }
}

void handleButtonPress() {
  int buttonState = digitalRead(BUTTON_PIN);
  if (buttonState != lastButtonState && buttonState == LOW) {
    if (ledOffState) {
      ledOffState = false;
      digitalWrite(LED_OFF_PIN, LOW);
      beep();
    } else if (!isRamping) {
      startRamping();
    } else {
      ledOffState = true;
      isRamping = false;
      digitalWrite(LED_OFF_PIN, HIGH);
    }
    lastActivityTime = millis(); // Update activity time
  }
  lastButtonState = buttonState;
}

void startRamping() {
  isRamping = true;
  rampStartTime = millis();
  originalSetpoint = knob;
}

int calculateRampSetpoint() {
  unsigned long elapsedTime = millis() - rampStartTime;
  if (elapsedTime > RAMP_DURATION) {
    isRamping = false;
    return originalSetpoint;
  }

  int rampTarget = MAX_TEMP;
  float progress = (float)elapsedTime / RAMP_DURATION;
  return originalSetpoint + (rampTarget - originalSetpoint) * progress;
}

void handleRamping() {
  if (isRamping && millis() - rampStartTime > RAMP_DURATION) {
    isRamping = false;
    beep(); // Alert user that ramping is complete
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
#ifdef USE_OLED
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.println("Auto Shut-off");
    display.println("Activated");
    display.display();
#else
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Auto Shut-off");
    lcd.setCursor(0, 1);
    lcd.print("Activated");
#endif
    delay(2000); // Show message for 2 seconds
    lastActivityTime = millis(); // Reset activity timer
  }
}
