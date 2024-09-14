# Soldering Iron Controller

## Table of Contents
1. [Introduction](#introduction)
2. [Features](#features)
3. [Hardware Requirements](#hardware-requirements)
4. [Software Dependencies](#software-dependencies)
5. [Installation](#installation)
6. [Wiring Guide](#wiring-guide)
7. [Usage](#usage)
8. [Temperature Control System](#temperature-control-system)
9. [Display Options](#display-options)
10. [Safety Features](#safety-features)
11. [Persistent Settings](#persistent-settings)
12. [Troubleshooting](#troubleshooting)
13. [Maintenance](#maintenance)
14. [Contributing](#contributing)

## Introduction

The Soldering Iron Controller is a robust, Arduino-based system designed to precisely control the temperature of a soldering iron. It features a digital interface, advanced temperature control, and safety mechanisms, making it suitable for both hobbyist and professional use.

## Features

- Precise temperature control (range: 100°C to 500°C)
- PID control system for stable temperature management
- Temperature ramping feature for controlled heating
- Digital display (OLED or LCD) for clear temperature readout
- WS2812 RGB LED for visual status indication
- Rotary encoder for easy temperature adjustment
- Automatic shut-off for safety
- Watchdog timer for system stability
- Temperature sensor with analog input for accurate readings
- PWM control for efficient power handling
- Persistent temperature setting stored in EEPROM

## Hardware Requirements

- Arduino board (e.g., Arduino Uno, Nano)
- OLED Display (SSD1306) or 16x2 I2C LCD Display
- WS2812 RGB LED
- Rotary Encoder with push button
- Temperature sensor (compatible with analog input)
- MOSFET or other switching device for iron control
- Soldering Iron (voltage depends on your setup)
- Appropriate power supply
- Buzzer for audible alerts
- Various resistors and capacitors (see wiring guide)

## Software Dependencies

Ensure you have the following libraries installed in your Arduino IDE:

- Wire.h (built-in)
- Adafruit_GFX.h (for OLED display)
- Adafruit_SSD1306.h (for OLED display)
- LiquidCrystal_I2C.h (for LCD display)
- BigNumbers_I2C.h (for LCD display)
- Adafruit_NeoPixel.h
- EEPROM.h (built-in)
- PID_v1.h

## Installation

1. Clone this repository or download the source code.
2. Open the `.ino` file in the Arduino IDE.
3. Install the required libraries through the Arduino Library Manager.
4. Configure the pin assignments in the code to match your wiring setup.
5. Choose between OLED and LCD display by commenting/uncommenting the `USE_OLED` define.
6. Upload the code to your Arduino board.

## Wiring Guide

Follow this wiring guide carefully:

1. **Temperature Sensing Circuit**:
   - Connect the temperature sensor to Arduino's A0 pin.

2. **Soldering Iron Control**:
   - Connect the control pin (MOSFET gate or equivalent) to Arduino's D10 pin.

3. **Display**:
   - For OLED: Connect SDA and SCL pins to Arduino's A4 and A5 pins respectively.
   - For LCD: Connect SDA and SCL pins to Arduino's A4 and A5 pins respectively.

4. **WS2812 LED**:
   - Connect the data pin of the WS2812 to Arduino's D12 pin.

5. **Rotary Encoder**:
   - Connect CLK and DT pins to Arduino's D3 and D4 pins respectively.
   - Connect the switch pin to Arduino's D5 pin.

6. **Buzzer**:
   - Connect to Arduino's D2 pin.

7. **LED Off Indicator**:
   - Connect to Arduino's D8 pin.

Ensure all ground connections are common and connect VCC appropriately.

## Usage

1. Power on the controller.
2. The last used temperature setting will be loaded automatically.
3. Use the rotary encoder to adjust the desired temperature.
4. The WS2812 LED will indicate the current status:
   - Red: Heating
   - Green: Ready (at set temperature)
   - Blue: Cooling
   - Yellow: Warning (near maximum temperature)
   - Purple: Ramping
   - Off: Iron is off or auto-shutoff activated
5. The display will show the current and set temperatures.
6. Press the rotary encoder button to toggle the iron on/off or start ramping.
7. During ramping, the display will show a countdown timer.

## Temperature Control System

### PID Control
The system uses a PID (Proportional-Integral-Derivative) controller for precise temperature management. The PID parameters are:
- Kp (Proportional): 2
- Ki (Integral): 5
- Kd (Derivative): 1

These parameters can be adjusted in the code to fine-tune the temperature control for your specific setup.

### Temperature Ramping
The controller includes a temperature ramping feature:
- Activated by pressing the button when the iron is on
- Ramps the temperature to the maximum (500°C) over 30 seconds
- Displays a countdown during ramping
- Automatically returns to the original setpoint after ramping

## Display Options

The controller supports two display options:
1. OLED Display (SSD1306)
2. 16x2 LCD Display with I2C interface

To switch between display types, comment or uncomment the `USE_OLED` define at the top of the code.

## Safety Features

1. **Automatic Shut-off**: The iron will automatically turn off after 10 minutes of inactivity.
2. **Watchdog Timer**: Resets the system if it becomes unresponsive.
3. **Temperature Warnings**: Visual indication when approaching maximum safe temperature.
4. **Error Detection**: The system monitors for sensor failures and other errors.

## Persistent Settings

The controller uses the Arduino's internal EEPROM to store the last used temperature setting. This feature ensures that your preferred temperature is remembered even after power cycles.

## Troubleshooting

- If the temperature reading is inaccurate, check the temperature sensor connections and calibration.
- If the iron isn't heating, verify the MOSFET or switching device connections and the power supply.
- For LED issues, ensure the WS2812 is correctly wired and the library is properly installed.
- If the system is unstable, check for loose connections and verify the code uploaded successfully.
- If the PID control is not performing well, try adjusting the Kp, Ki, and Kd values in the code.

## Maintenance

- Regularly check all connections for signs of wear or damage.
- Clean the temperature sensor periodically for accurate temperature readings.
- Update the firmware if new versions become available.
- Calibrate the temperature sensor annually or if temperature inaccuracies are noticed.

## Contributing

Contributions to this project are welcome. Please fork the repository and submit a pull request with your proposed changes.
