# CO2 Ampel

A visual indoor air quality monitor that displays real-time CO2 levels using a color-coded LED strip. The device changes color like a traffic light based on air quality, making it easy to know when to ventilate rooms.

![License](https://img.shields.io/badge/license-MIT-blue.svg)

## Overview

CO2 Ampel (German for "CO2 traffic light") is an ESP32-based air quality monitoring device that provides immediate visual feedback about indoor air quality. High CO2 levels indicate poor ventilation and can cause fatigue, reduced concentration, and health issues. This device makes monitoring air quality simple and intuitive.

**Visual Indicators:**
- **Green**: Good air quality (CO2 < 800 ppm)
- **Orange**: Moderate air quality (CO2 800-1200 ppm)
- **Red**: Poor air quality (CO2 > 1200 ppm) - ventilation needed

## Features

- **Real-time CO2 monitoring** using Sensirion SCD30 sensor (accurate NDIR technology)
- **Visual LED feedback** with configurable NeoPixel strips (1, 8, 12, 16, or 24 LEDs)
- **Multiple connectivity options** - choose the variant that fits your needs:
  - Standalone Bluetooth (view data on mobile app)
  - WiFi with cloud integration
  - LoRaWAN for low-power wide-area networks
  - Local web server (no internet required)
- **Optional BME680 sensor** for temperature, humidity, and air quality index (IAQ)
- **One-button calibration** for accurate baseline measurements
- **Open hardware design** with 3D-printable enclosure and optional custom PCB
- **Flexible LED configuration** - works with various LED strip sizes

## Hardware Requirements

### Core Components
- **ESP32 development board** (NodeMCU-32S or similar)
- **Sensirion SCD30** CO2 sensor module (I2C)
- **NeoPixel LED strip** (WS2812B, 1-24 LEDs)
- **Power supply** (5V, 2A recommended)
- **Push button** (for calibration, optional)

### Optional Components
- **Adafruit BME680** sensor (for temperature, humidity, and IAQ)
- **Custom connection board** (Fritzing design in `board/`)
- **3D-printed enclosure** (STL files in `cad/`)

### Wiring
- SCD30: I2C (SDA, SCL) + 5V
- BME680 (optional): I2C (SDA, SCL) + 3.3V
- LED strip: GPIO 15 (data), 5V, GND
- Calibration button: GPIO 0, GND

## Firmware Variants

Choose the firmware variant that matches your use case:

| Variant | File | Description | Use Case |
|---------|------|-------------|----------|
| **Standalone** | `code/co2ampel.ino` | Bluetooth connectivity for mobile app access | Portable, battery-powered, or simple setups |
| **WiFi Cloud** | `code/co2ampel_cloud_wlan.ino` | Sends data to cloud API via WiFi | Remote monitoring, data logging |
| **LoRaWAN Cloud** | `code/co2ampel_cloud_lora.ino` | Low-power wide-area network connectivity | Long-range, battery-powered deployments |
| **Web Server** | `code/co2ampel_webserver.ino` | Built-in web interface on local network | Local monitoring without internet |

## Quick Start

### 1. Flash Pre-built Firmware

Download pre-built binaries from the `firmware/` directory:
- `co2ampel_latest_12led_esp32.bin` (recommended for standard 12-LED strips)
- `co2ampel_latest_8led_esp32.bin` (for 8-LED rings)
- `co2ampel_latest_1led_esp32.bin` (single LED indicator)
- `co2ampel_latest_16led_esp32.bin` (16-LED strips)
- `co2ampel_latest_24led_esp32.bin` (24-LED strips)

Use a web-based flasher or esptool to upload firmware to your ESP32.

### 2. Build from Source

**Arduino IDE:**
1. Install Arduino IDE and ESP32 board support
2. Install required libraries:
   - Adafruit NeoPixel
   - SparkFun SCD30
   - Sensirion Gadget BLE (for standalone)
   - Adafruit BME680 (optional)
   - ArduinoJson
   - MCCI LoRaWAN LMIC (for LoRa variant)
3. Open desired `.ino` file from `code/` directory
4. Configure LED count in code: `#define LED_COUNT 12`
5. Upload to ESP32

**PlatformIO (coming soon):**
A modern build system with dependency management is planned for PR #7.

### 3. Calibrate

1. Place device outdoors or in well-ventilated area (400 ppm baseline)
2. Press calibration button (GPIO 0)
3. LEDs turn blue during 30-second calibration
4. Device automatically restarts with new baseline

## Project Structure

```
co2ampel/
├── code/               # Firmware source files (.ino)
├── firmware/          # Pre-compiled binaries for web flasher
├── cad/               # 3D-printable enclosure (STL files)
├── board/             # Optional custom PCB (Fritzing design)
├── .gitignore         # Git ignore rules
├── LICENSE            # MIT License
└── README.md          # This file
```

## How It Works

1. **Sensor Reading**: SCD30 measures CO2 concentration using NDIR technology
2. **Color Mapping**: CO2 level is mapped to a color (green/orange/red)
3. **LED Display**: Number of illuminated LEDs indicates measurement magnitude
4. **Data Output**: Depending on variant, data is sent via Bluetooth, WiFi, LoRa, or web interface

## Why Monitor CO2?

Indoor CO2 levels directly indicate ventilation quality. High CO2 (>1000 ppm) is associated with:
- Reduced cognitive performance and concentration
- Increased drowsiness and fatigue
- Headaches and discomfort
- Higher risk of airborne disease transmission

The German Federal Environment Agency recommends ventilating when CO2 exceeds 1000 ppm, particularly in schools, offices, and meeting rooms.

## Configuration

### LED Count
Edit the firmware to match your LED strip:
```cpp
#define LED_COUNT 12  // Change to 1, 8, 12, 16, or 24
```

### CO2 Thresholds
Adjust warning thresholds to your preferences:
```cpp
#define HIGH_CO2_BOUNDARY 1200  // Red threshold
#define LOW_CO2_BOUNDARY 800    // Green threshold
```

### WiFi Credentials (cloud variants)
Update WiFi settings in code:
```cpp
const char* ssid = "your-wifi-ssid";
const char* password = "your-wifi-password";
```

## Development Status

This project is actively maintained and undergoing a comprehensive refactoring. See `REFACTORING_PLAN.md` for the roadmap, which includes:
- Eliminating code duplication across variants
- Externalizing secrets and credentials
- Migration to PlatformIO build system
- Unit testing infrastructure
- Comprehensive documentation
- Improved error handling

## Contributing

Contributions are welcome! This is an open-source project under the MIT License. Areas where help is appreciated:
- Testing on different hardware configurations
- Documentation improvements
- Additional firmware variants
- Bug fixes and feature enhancements

Please check `REFACTORING_PLAN.md` for ongoing work to avoid conflicts.

## License

MIT License - Copyright (c) 2020 Markus Leutwyler

See [LICENSE](LICENSE) for full details.

## Resources

- **Sensirion SCD30 Datasheet**: [Link](https://www.sensirion.com/scd30/)
- **Arduino Library**: SparkFun SCD30 Library
- **ESP32 Documentation**: [ESP-IDF](https://docs.espressif.com/projects/esp-idf/)
- **NeoPixel Guide**: [Adafruit Tutorial](https://learn.adafruit.com/adafruit-neopixel-uberguide)

## Safety Notes

- SCD30 requires 5V power supply for accurate measurements
- Use appropriate power supply for LED strips (2A+ for 24 LEDs at full brightness)
- Calibrate sensor regularly for accurate readings (every 1-2 months)
- Allow 2-3 minutes warmup time after power-on for stable readings

## Support

- **Issues**: Report bugs or request features via GitHub Issues
- **Documentation**: See `REFACTORING_PLAN.md` for architecture details
- **Hardware**: Check `board/` and `cad/` directories for design files

---

**Made with care for better indoor air quality.** Breathe easy!
