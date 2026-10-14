# Arduino Tea Mixer

ESP32-based firmware for an automated tea brewing and dispensing machine. The system controls ingredient pumps via relays, measures liquid volumes using a load cell or timed pumping, displays status on an I2C OLED screen, and interfaces with a companion mobile app over Bluetooth Low Energy (BLE).

## Features

* **Dual Dispensing Strategies:** Supports closed-loop weight-based dispensing via an HX711 load cell amplifier or open-loop time-based pump actuation.
* **BLE Remote Control:** Serialized JSON protocol over BLE for remote brewing triggers, real-time telemetry (progress, cup presence, errors), and profile sync.
* **Physical Controls & OLED UI:** Three-button tactile navigation with live telemetry (boiler temperature, active recipe, connection status, brewing progress) on an SSD1306 128x64 display.
* **Hardware Calibration:** In-firmware routine to tare the load cell and set minimum cup-detection thresholds.
* **Automated Flush Cycle:** Two-stage maintenance cycle to clean liquid lines and pumps.
* **NVS Persistence:** Stores calibrated tare offsets, recipe volume targets, and system preferences across reboots using ESP32 Preferences.

## Hardware Configuration & Pinout

| Peripheral | Component | ESP32 GPIO | Notes |
| --- | --- | --- | --- |
| Display | SSD1306 OLED (I2C) | GPIO 21 (SDA), GPIO 22 (SCL) | Default ESP32 I2C bus |
| Buttons | Tactile Push Buttons (x3) | GPIO 25, 26, 27 | Active-low, debounced via `OneButton` |
| Scale | HX711 Amplifier | GPIO 13 (DOUT), GPIO 14 (SCK) | Connect to strain gauge / load cell |
| Temperature | DS18B20 1-Wire | GPIO 4 | Requires 4.7kΩ pull-up to 3.3V |
| Actuators | 4-Channel Relay Module | GPIO 16, 17, 18, 19 | Controls pump channels (Tea, Milk, Sweetener, Spare) |

> **Power note:** Ensure the relay board and 12V/5V pumps have an independent power supply with a shared common ground to the ESP32 to prevent brownouts during pump inrush.

## Dependencies

Install the following libraries via Arduino Library Manager or PlatformIO:

* `ESP32 BLE Arduino`
* `Adafruit SSD1306` & `Adafruit GFX Library`
* `OneButton`
* `HX711`
* `DallasTemperature` & `OneWire`
* `ArduinoJson` (v6 or v7)

## Setup & Flashing

### Arduino IDE

1. Install the ESP32 board package (`Tools` > `Board` > `Boards Manager`).
2. Select your board (e.g., `ESP32 Dev Module`).
3. Open `project_tea.ino`.
4. Compile and flash via USB serial.

### PlatformIO

Add the required dependencies to your `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    adafruit/Adafruit SSD1306
    adafruit/Adafruit GFX Library
    mathertel/OneButton
    bogde/HX711
    milesburton/DallasTemperature
    paulstoffregen/OneWire
    bblanchon/ArduinoJson
```

## Operation

### Standard Dispensing

1. **Idle State:** Displays liquid temperature, BLE connection status, and recipe presets assigned to Buttons 1–3.
2. **Preview:** Press Button 1, 2, or 3 to inspect ingredient amounts.
3. **Confirm:** Press the same button within 5 seconds to start brewing.
4. **Safety Check:** The system verifies cup presence (weight must exceed `minCupWeight`). If missing, an on-screen warning halts the cycle until placed.
5. **Dispense:** Dispenses fluids sequentially or proportionally based on the active strategy.
6. **Completion:** Screen notifies completion; removing the cup resets the system back to `IDLE`.

### Device Modes & Combos

| Action | Combination | Description |
| --- | --- | --- |
| **Calibration Mode** | Hold **Btn 1 + Btn 2** (3s) | Enter calibration. Press **Btn 1** to save empty cup threshold; press **Btn 3** to tare. |
| **Strategy Toggle** | Hold **Btn 1 + Btn 3** (3s) | Switches between Weight-based and Time-based dispensing. Saved to NVS. |
| **Target Volume (ML)** | Hold **Btn 2 + Btn 3** (3s) | Adjust baseline volume: **Btn 1** (+10ml), **Btn 2** (-10ml), **Btn 3** to save. |
| **Maintenance Cycle** | Hold **Btn 1 + Btn 3** (3s while idle) | Initiates 60s timed line flush. Any button cancels. |

### BLE Interface

The ESP32 runs a BLE GATT server accepting JSON commands and streaming telemetry payloads:

* **Incoming Commands:** `MAKE_TEA`, `SET_BUTTON_INFO`, `GET_DEVICE_INFO`, `SET_TARGET_ML`
* **Outgoing Notifications:** `TEA_PROGRESS`, `TEA_FINISH`, `CUP_WARNING`, `CLEANING_PROGRESS`

## Project Structure

```text
arduino-tea-mixer/
├── docs/                      # UML diagrams (class, state machine, sequence)
├── src/
│   ├── BluetoothManager.cpp   # BLE GATT server and JSON dispatch
│   ├── ButtonManager.cpp      # Debounced button inputs and long-press handlers
│   ├── CleaningMode.cpp       # Pump flushing state machine
│   ├── DispensingStrategy.cpp # Base strategy abstraction
│   ├── WeightDispensingStrategy.cpp # Closed-loop load cell dispensing
│   ├── TimeDispensingStrategy.cpp   # Open-loop time-based dispensing
│   ├── DisplayManager.cpp     # SSD1306 rendering routines
│   ├── RelayManager.cpp       # Actuator GPIO switching
│   ├── SensorManager.cpp      # HX711 and DS18B20 polling
│   ├── TeaManager.cpp         # Core state machine coordinator
│   └── Models.h               # Data structs, enums, and system constants
├── project_tea.ino            # Main firmware entrypoint (setup & loop)
└── README.md
```

## License

Copyright (c) 2026 Naing Htet Linn. All rights reserved.

This project and its source code are proprietary. Unauthorized copying, modification, distribution, or use of this software, in whole or in part, via any medium, is strictly prohibited without prior written permission from the author.