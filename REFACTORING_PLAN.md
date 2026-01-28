# CO2 Ampel Refactoring Plan

## Overview
Comprehensive refactoring of the CO2 Ampel codebase to improve code quality, maintainability, and follow modern Arduino/ESP32 development practices. The work is organized into 10 focused GitHub pull requests.

## Current State Analysis
- **4 firmware variants** totaling 1,456 lines of Arduino C/C++ code
  - `co2ampel.ino` (273 lines): Standalone Bluetooth
  - `co2ampel_cloud_wlan.ino` (390 lines): WiFi cloud
  - `co2ampel_cloud_lora.ino` (441 lines): LoRaWAN cloud
  - `co2ampel_webserver.ino` (352 lines): Local webserver

- **Critical Issues Identified:**
  - ~70% code duplication across all variants
  - 15-20+ magic numbers per file (thresholds, colors, pins, delays)
  - No testing infrastructure
  - Hardcoded credentials and expired SSL certificate
  - No formal build system or dependency management
  - Binary files tracked in git (~5MB)
  - Long functions (150-208 lines)
  - Extensive commented-out dead code

## Refactoring Strategy: 10 Pull Requests

### Phase 1: Foundation & Safety

#### PR #1: Git Hygiene and Project Structure
**Goal:** Establish proper version control practices

**Changes:**
- Create `.gitignore` to exclude build artifacts, IDE files, and secrets
- Move 5 pre-compiled binaries to `firmware/` directory for web-based flasher distribution
- Add patterns for: `code/*.bin`, `*.elf`, `.pio/`, `secrets.h`, `.DS_Store`
- Note: Will transition to GitHub Releases with automated builds in PR #7

**Files:**
- Create: `.gitignore`, `firmware/` directory
- Move: `code/co2ampel_latest_*.bin` (5 files) → `firmware/`

**Risk:** Low (no code changes, improves repo structure)

---

#### PR #2: Extract Configuration Constants
**Goal:** Eliminate magic numbers with centralized configuration

**Changes:**
- Create `code/config.h` with all hardcoded constants:
  - Hardware config: `LED_PIN=15`, `LED_COUNT=12`, `BUTTON_PIN=0`
  - CO2 thresholds: `HIGH_CO2_BOUNDARY=1200`, `LOW_CO2_BOUNDARY=800`
  - Display settings: `LED_BRIGHTNESS=64`, `CO2_MAP_MIN=350`
  - Timing: `CALIBRATION_DURATION=30000`, `DISPLAY_UPDATE_INTERVAL=2000`
  - Colors: `COLOR_RED`, `COLOR_ORANGE`, `COLOR_GREEN`, `COLOR_BLUE`
  - IAQ weights: `HUM_WEIGHTING=0.25`, `GAS_WEIGHTING=0.75`

**Files:**
- Create: `code/config.h`
- Modify: All 4 .ino files (add `#include "config.h"`, replace magic numbers)

**Testing:**
- Compile all 4 variants
- Verify identical behavior (LED colors, thresholds, timing)

**Risk:** Low (refactoring only, no logic changes)

---

#### PR #3: Security - Externalize Credentials
**Goal:** Remove hardcoded secrets and follow security best practices

**Changes:**
- Create `code/secrets.h.template` with placeholders for:
  - WiFi credentials (`WIFI_SSID`, `WIFI_PASSWORD`)
  - Cloud API config (`CLOUD_API_DEVICE_ID`, `CLOUD_API_AUTH_TOKEN`)
  - LoRaWAN keys (`APPEUI`, `DEVEUI`, `APPKEY`)
  - Access point config (`AP_SSID`, `AP_PASSWORD`)
- Create `docs/SECURITY.md` with credential setup instructions
- Update `.gitignore` to exclude `secrets.h`
- Modify variants to include `secrets.h` and use constants

**Files:**
- Create: `code/secrets.h.template`, `docs/SECURITY.md`
- Modify: `co2ampel_cloud_wlan.ino`, `co2ampel_cloud_lora.ino`
- Update: `.gitignore`

**Testing:**
- Build should fail without secrets.h (with helpful error)
- After creating secrets.h, builds succeed
- Verify credentials work at runtime

**Risk:** Medium (requires user configuration, critical for security)

---

### Phase 2: Code Consolidation

#### PR #4: Extract Shared LED Display Logic
**Goal:** Create reusable LED display library (eliminates ~100 lines of duplication)

**Changes:**
- Create `CO2Display` library:
  - `CO2Display.h` with class interface
  - `CO2Display.cpp` with implementations
  - Methods: `begin()`, `updateCO2(ppm)`, `showCalibrating()`, `clear()`
  - Color mapping based on CO2Level enum (GOOD/MODERATE/POOR)
  - Pixel count calculation
- Update all 4 variants to use library

**Files:**
- Create: `code/lib/CO2Display/CO2Display.h`, `CO2Display.cpp`
- Modify: All 4 .ino files (replace inline LED logic)

**Testing:**
- Verify LED colors match original behavior
- Test transitions between Good/Moderate/Poor levels
- Test calibration animation
- Test with different LED_COUNT values (1, 8, 12, 16, 24)

**Risk:** Medium (changes core display logic)

---

#### PR #5: Extract Sensor Management Library
**Goal:** Consolidate sensor initialization, reading, and IAQ calculation (~150 lines of duplication)

**Changes:**
- Create `AirQualitySensor` library:
  - `AirQualitySensor.h` with class interface and data structures
  - `SensorReading` struct (co2, temperature, humidity, valid flag)
  - `IAQData` struct (score, pressure, gas, quality, available flag)
  - Methods: `begin()`, `isDataAvailable()`, `read()`, `readIAQ()`, `calibrate()`
  - Automatic BME680 detection
  - IAQ calculation implementation
- Update all variants to use library

**Files:**
- Create: `code/lib/AirQualitySensor/AirQualitySensor.h`, `AirQualitySensor.cpp`
- Modify: All 4 .ino files

**Testing:**
- Test with SCD30 only
- Test with SCD30 + BME680
- Verify calibration works
- Verify IAQ calculations

**Risk:** Medium (consolidates sensor logic)

---

#### PR #6: Extract Calibration Button Handler
**Goal:** Create reusable calibration component (~50 lines of duplication)

**Changes:**
- Create `CalibrationManager` library:
  - `CalibrationManager.h` with class interface
  - Methods: `begin()`, `update()`, `isCalibrating()`
  - Button interrupt handling (IRAM_ATTR ISR)
  - Calibration sequence (display blue, wait 30s, calibrate, restart)
- Update all variants to use library

**Files:**
- Create: `code/lib/CalibrationManager/CalibrationManager.h`, `CalibrationManager.cpp`
- Modify: All 4 .ino files

**Testing:**
- Verify button press detection
- Test calibration sequence
- Verify ESP32 restarts after calibration
- Test multiple rapid presses

**Risk:** Low (isolated functionality)

---

### Phase 3: Build System & Testing

#### PR #7: Migrate to PlatformIO Build System
**Goal:** Replace Arduino IDE with formal build system for dependency management and CI/CD

**Changes:**
- Create `platformio.ini` with 4 environments (standalone, cloud_wifi, cloud_lora, webserver)
- Pin library versions:
  - Adafruit NeoPixel @^1.11.0
  - SparkFun SCD30 @^1.0.19
  - Adafruit BME680 @^2.0.2
  - ArduinoJson @^6.21.3
  - MCCI LoRaWAN LMIC @^4.1.1
  - Sensirion Gadget BLE @^1.1.0
- Restructure project:
  - Move .ino files to `src/variants/*.cpp`
  - Libraries to `lib/`
  - Headers to `include/`
- Set up GitHub Actions CI/CD to automatically build binaries and attach to GitHub Releases
- Transition from `firmware/` directory to GitHub Releases for binary distribution
- Create `docs/BUILD.md` with build instructions

**New Structure:**
```
co2ampel/
├── platformio.ini
├── lib/
│   ├── AirQualitySensor/
│   ├── CO2Display/
│   └── CalibrationManager/
├── src/variants/
│   ├── standalone.cpp
│   ├── cloud_wifi.cpp
│   ├── cloud_lora.cpp
│   └── webserver.cpp
├── include/
│   ├── config.h
│   └── secrets.h.template
└── test/
```

**Testing:**
- Build all 4 environments: `pio run`
- Verify library dependencies resolve
- Test upload to hardware: `pio run -e standalone -t upload`
- Test serial monitor: `pio device monitor`

**Risk:** Medium (major structural change, but reversible)

---

#### PR #8: Add Unit Testing Infrastructure
**Goal:** Establish testing framework and create tests for core libraries

**Changes:**
- Create unit tests for:
  - `test_co2_display/`: Color mapping, pixel count calculation
  - `test_sensor/`: Sensor reading validation
  - `test_calibration/`: Button handling, calibration logic
- Configure PlatformIO for native testing
- Create `docs/TESTING.md`

**Files:**
- Create: `test/test_co2_display/test_display.cpp`
- Create: `test/test_sensor/test_sensor.cpp`
- Create: `test/test_calibration/test_calibration.cpp`
- Create: `docs/TESTING.md`
- Modify: `platformio.ini` (add native test environment)

**Testing:**
- Run native tests: `pio test -e native`
- Run hardware tests: `pio test -e standalone`
- Verify all tests pass

**Risk:** Low (adds tests, doesn't change production code)

---

### Phase 4: Improvements & Documentation

#### PR #9: Improve Error Handling and Input Validation
**Goal:** Add comprehensive error handling and input validation

**Changes:**
- Sensor reading validation:
  - CO2 range check (350-5000 ppm)
  - Temperature range check (-40 to 85°C)
  - Humidity range check (0-100%)
- WiFi connection retry logic with configurable max attempts
- HTTPS error handling with response code checking
- Watchdog timer protection (10-second timeout)
- Exponential backoff for network retries

**Files:**
- Modify: `lib/AirQualitySensor/AirQualitySensor.cpp`
- Modify: `src/variants/cloud_wifi.cpp`
- Modify: `src/variants/cloud_lora.cpp`
- Modify: `src/variants/webserver.cpp`

**Testing:**
- Test with disconnected sensor
- Test with invalid readings
- Test WiFi disconnection/reconnection
- Test HTTPS failures
- Verify watchdog triggers on infinite loops

**Risk:** Low (adds safety, doesn't change core logic)

---

#### PR #10: Comprehensive Documentation and Examples
**Goal:** Create detailed documentation for users and developers

**Changes:**
- Comprehensive `README.md` rewrite with:
  - Features overview
  - Quick start guide
  - Variant descriptions
  - Documentation links
- Create detailed documentation:
  - `docs/ARCHITECTURE.md`: 4-variant architecture, library structure, data flow
  - `docs/HARDWARE.md`: Wiring diagrams, BOM, LED configurations
  - `docs/CONFIGURATION.md`: LED_COUNT, thresholds, WiFi, TTN, API setup
  - `docs/TROUBLESHOOTING.md`: Common issues and solutions
- Create example projects:
  - `examples/basic_standalone/`
  - `examples/cloud_wifi_ttn/`
  - `examples/custom_thresholds/`

**Files:**
- Modify: `README.md`
- Create: `docs/ARCHITECTURE.md`, `docs/HARDWARE.md`, `docs/CONFIGURATION.md`, `docs/TROUBLESHOOTING.md`
- Create: Example projects

**Testing:**
- Have someone unfamiliar with project follow docs
- Verify all links work
- Test examples compile

**Risk:** None (documentation only)

---

## PR Dependencies

```
PR #1 (Git Hygiene)
  └── PR #2 (Configuration Constants)
      ├── PR #3 (Security)
      ├── PR #4 (LED Display Library)
      │   └── PR #6 (Calibration Manager)
      └── PR #5 (Sensor Library)
          └── PR #6 (Calibration Manager)

PR #4, #5, #6 ──→ PR #7 (PlatformIO)
                    ├── PR #8 (Testing)
                    └── PR #9 (Error Handling)

PR #1-9 ──→ PR #10 (Documentation)
```

## Implementation Timeline

- **Week 1:** PRs #1-3 (Foundation & Safety)
- **Week 2:** PRs #4-6 (Code Consolidation)
- **Week 3:** PRs #7-8 (Build System & Testing)
- **Week 4:** PRs #9-10 (Improvements & Documentation)

## Success Criteria

After all PRs are merged:
- ✅ Zero code duplication for core logic
- ✅ All secrets externalized
- ✅ All 4 variants build with PlatformIO
- ✅ 80%+ test coverage for libraries
- ✅ Comprehensive documentation
- ✅ Clean git history
- ✅ No binaries in repository
- ✅ Security best practices followed

## Verification After Each PR

Each PR should include:
1. Successful compilation of all affected variants
2. No behavioral changes (unless explicitly intended)
3. Testing notes in PR description
4. Updated documentation if needed
5. Git commit messages following conventional commits format
