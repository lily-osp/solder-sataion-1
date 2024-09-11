# Soldering Iron Controller

## Table of Contents
1. [Introduction](#introduction)
2. [Features](#features)
3. [Hardware Requirements](#hardware-requirements)
4. [Software Dependencies](#software-dependencies)
5. [Installation](#installation)
6. [Wiring Guide](#wiring-guide)
7. [Usage](#usage)
8. [Safety Features](#safety-features)
9. [Persistent Settings](#persistent-settings)
10. [Troubleshooting](#troubleshooting)
11. [Maintenance](#maintenance)
12. [Contributing](#contributing)

## Introduction

The Soldering Iron Controller is a robust, Arduino-based system designed to precisely control the temperature of a soldering iron. It features a digital interface, temperature control, and safety mechanisms, making it suitable for both hobbyist and professional use.

## Features

- Precise temperature control (range: 100°C to 500°C)
- Digital LCD display for clear temperature readout
- WS2812 RGB LED for visual status indication
- Rotary encoder for easy temperature adjustment
- Automatic shut-off for safety
- Watchdog timer for system stability
- Temperature sensor with LM358P op-amp for accurate readings
- MOSFET (IRLZ44N) control for efficient power handling
- Persistent temperature setting stored in EEPROM

## Hardware Requirements

- Arduino board (e.g., Arduino Uno, Nano)
- 16x2 I2C LCD Display
- WS2812 RGB LED
- Rotary Encoder with push button
- LM358P Operational Amplifier
- IRLZ44N MOSFET
- Soldering Iron (24V recommended)
- 24V Power Supply
- Thermocouple or thermistor (compatible with your soldering iron)
- Various resistors and capacitors (see wiring guide)
- Cooling fan (optional, but recommended)

## Software Dependencies

Ensure you have the following libraries installed in your Arduino IDE:

- Wire.h (built-in)
- LiquidCrystal_I2C.h
- BigNumbers_I2C.h
- Adafruit_NeoPixel.h
- EEPROM.h (built-in)

## Installation

1. Clone this repository or download the source code.
2. Open the `.ino` file in the Arduino IDE.
3. Install the required libraries through the Arduino Library Manager.
4. Configure the pin assignments in the code to match your wiring setup.
5. Upload the code to your Arduino board.

## Wiring Guide

Follow this wiring guide carefully:

1. **Temperature Sensing Circuit**:
   - Connect the thermocouple/thermistor to the input of the LM358P op-amp.
   - Connect the output of the LM358P to Arduino's A0 pin.

2. **Soldering Iron Control**:
   - Connect the IRLZ44N MOSFET's gate to Arduino's D10 pin.
   - Connect the MOSFET's source to ground.
   - Connect the MOSFET's drain to the negative terminal of the soldering iron.
   - Connect the positive terminal of the soldering iron to the 24V power supply.

3. **LCD Display**:
   - Connect SDA and SCL pins of the LCD to Arduino's A4 and A5 pins respectively.
   - Connect VCC and GND of the LCD to 5V and GND on the Arduino.

4. **WS2812 LED**:
   - Connect the data pin of the WS2812 to Arduino's D12 pin.
   - Connect VCC and GND of the LED to 5V and GND on the Arduino.

5. **Rotary Encoder**:
   - Connect CLK and DT pins to Arduino's D3 and D4 pins respectively.
   - Connect the switch pin to Arduino's D5 pin.
   - Connect VCC and GND to 5V and GND on the Arduino.

Ensure all ground connections are common.

## Usage

1. Power on the controller.
2. The last used temperature setting will be loaded automatically.
3. Use the rotary encoder to adjust the desired temperature.
4. The WS2812 LED will indicate the current status:
   - Red: Heating
   - Green: Ready (at set temperature)
   - Blue: Cooling
   - Yellow: Warning (near maximum temperature)
   - Off: Iron is off or auto-shutoff activated
5. The LCD will display the current and set temperatures.
6. Press the rotary encoder button to toggle the iron on/off.
7. Any changes to the temperature setting are automatically saved and will be restored on the next power-up.

## Safety Features

1. **Automatic Shut-off**: The iron will automatically turn off after 30 minutes of inactivity.
2. **Watchdog Timer**: Resets the system if it becomes unresponsive.
3. **Temperature Warnings**: Visual indication when approaching maximum safe temperature.
4. **Error Detection**: The system monitors for sensor failures and other errors.

## Persistent Settings

The controller uses the Arduino's internal EEPROM to store the last used temperature setting. This feature ensures that your preferred temperature is remembered even after power cycles, providing a consistent user experience without the need for external memory components.

- The temperature setting is automatically saved whenever it's changed using the rotary encoder.
- On startup, the last saved temperature is loaded and set as the target temperature.
- If no valid temperature is found in the EEPROM (e.g., first use), the controller defaults to the minimum temperature setting.

Note: The EEPROM has a limited number of write cycles (typically 100,000). However, at normal usage levels, this should not be a concern for many years of operation.

## Troubleshooting

- If the temperature reading is inaccurate, check the LM358P circuit and connections.
- If the iron isn't heating, verify the IRLZ44N connections and the power supply.
- For LED issues, ensure the WS2812 is correctly wired and the library is properly installed.
- If the system is unstable, check for loose connections and verify the code uploaded successfully.
- If the temperature setting is not being remembered between power cycles, there may be an issue with the EEPROM. Try resetting the Arduino board. If the problem persists, the EEPROM may be faulty.

## Maintenance

- Regularly check all connections for signs of wear or damage.
- Clean the thermocouple/thermistor periodically for accurate temperature readings.
- Update the firmware if new versions become available.
- Calibrate the temperature sensor annually or if temperature inaccuracies are noticed.
- The EEPROM has a limited number of write cycles. While this shouldn't be an issue under normal use, consider updating the firmware to include wear leveling if very frequent temperature changes are expected in your application.

## Contributing

Contributions to this project are welcome. Please fork the repository and submit a pull request with your proposed changes.
