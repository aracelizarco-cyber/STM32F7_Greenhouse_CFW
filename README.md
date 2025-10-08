# Greenhouse STM32F7 Starter Firmware (UART + I2C SCD41 + PWM Fans)

This is a minimal, working STM32F7 codebase you can build and flash immediately using PlatformIO. It provides:
- UART CLI for control and status (115200 8N1)
- I2C SCD41 sensor read (CO₂ ppm, temperature, humidity) with CRC-8 validation
- 4x PWM outputs for DC fan/ESC tests with arming and ramping
- Clean pin config in `include/pins.h`

Defaults
- Board: NUCLEO-F722ZE (easy to compile/flash; swap pins later to match your FC)
- Upload: DFU

Quick Start
1. Install PlatformIO (VS Code extension) or PlatformIO Core.
2. Open this project folder.
3. Build:
   pio run
4. Flash (put board in DFU mode: BOOT+USB/RESET):
   pio run -t upload
5. Serial monitor:
   pio device monitor -b 115200

CLI Commands
- STATUS
- ARM
- DISARM
- SET <idx 0..3> <duty 0..1000>   (0=0%, 1000=100%)
- GET SENSORS

SCD41 Notes
- Periodic mode at ~5s cadence.
- Implemented CRC-8 (poly 0x31, init 0xFF) per Sensirion spec.
- You can disable SCD41 by setting SCD41_ENABLE to 0 in pins.h, if needed.

Adapting Pins to Your Flight Controller
- Edit include/pins.h and change:
  - UART (which USART, pins)
  - I2C bus (SCL/SDA)
  - PWM timer/channel pins
  - LED pin
- Adjust RCC clock if needed (SystemClock_Config in src/main.c)

Future Work
- Swap PWM for DShot (timer+DMA based), keep the same command API.
- Add failsafe timeout (heartbeat from SBC) to ramp down motors.
- Integrate ESC telemetry (optional).