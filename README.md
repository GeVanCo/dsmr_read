# ESP32-C3 GPIO8 Blink Template

This template provides a basic blink example for GPIO8 on the ESP32-C3 SuperMini using ESP-IDF.

## Features
- Blinks GPIO8 every 1 second.
- Uses ESP-IDF logging for debugging.

## Pinout
- **GPIO8**: Connected to the onboard RGB LED (or external LED).

## Customization
- Change the blink speed by modifying the `vTaskDelay` values in `main.c`.
- Replace `GPIO8` with another GPIO if needed (ensure it doesn’t interfere with boot mode).

## Troubleshooting
- If the board doesn’t boot, check if GPIO8 is pulled high during boot (e.g., by an external circuit).
