# Greenhouse STM32F7 Firmware (DShot600 + Heartbeat Failsafe + Telemetry)

This firmware provides:
- DShot600 motor control via TIM1 + DMA (4 channels)
- Heartbeat-based failsafe from SBC (ramp-to-zero, auto-disarm)
- UART CLI (text) for debugging: STATUS, ARM, DISARM, SET, GET SENSORS, GET TELEM
- Binary command API (framed) for robust SBC control
- SCD41 sensor via I2C (5s cadence, CRC-8 verified)
- ESC Telemetry hook (UART; forwards raw frames; parsed RPM if format matches)

Quick Start
1. Install PlatformIO (VS Code extension) or PlatformIO Core.
2. Open this project folder.
3. Build:
   pio run
4. Flash (put board in DFU mode: BOOT + RESET or BOOT while plugging in):
   pio run -t upload
5. Serial monitor:
   pio device monitor -b 115200

Text CLI (debug)
- STATUS
- ARM
- DISARM
- SET <idx 0..3> <0..1000>       (maps to DShot throttle 48..2047 with ramping)
- GET SENSORS
- GET TELEM

Binary API (preferred for SBC)
Frame: 0xA5, ver=1, msg_id, seq, len, payload[len], crc16-ccitt

Msg IDs:
- 0x01 HEARTBEAT () → ACK only (feeds failsafe)
- 0x02 ARM ()
- 0x03 DISARM ()
- 0x04 SET_MOTOR (u8 idx, u16 value_0_1000)
- 0x05 GET_SENSORS () → response payload: co2_ppm u16, temp_c_x100 s16, rh_x100 u16
- 0x06 GET_TELEM () → response payload: rpm[u16 x4], esc_temp_c u16 (0 if unknown), raw_len u8, raw[raw_len up to 16]

Failsafe
- SBC must send HEARTBEAT at least every 1000 ms (configurable in include/pins.h)
- If missed:
  - soft-failsafe: ramp motors to zero
  - disarm once zeroed
  - state cleared on next ARM + heartbeat

Pins (default NUCLEO-F722ZE; adapt for your FC)
- DShot outputs: TIM1 CH1..CH4 on PA8, PA9, PA10, PA11
- SCD41: I2C1 PB8 (SCL), PB9 (SDA)
- CLI UART2: PA2 (TX), PA3 (RX)
- ESC Telemetry UART3 RX: PB11 (adjust to your hardware)

Notes
- DShot bit timing: 600 kHz (bit period ~1.667 µs). Timer runs at 216 MHz (ARR ~ 360 - 1).
- Duty encoding: '1' ≈ 0.75T, '0' ≈ 0.375T.
- DMA stream/channel mappings are MCU/board-specific. The scaffolding uses HAL TIM PWM DMA; you may need to implement MSP DMA init for your exact MCU to enable DMA transfers.

Roadmap
- Add DShot bidirectional telemetry (pin-swapping, input sampling) if your ESC supports it
- Move binary API to DMA-driven UART with sequence/ACK tracking
- Config persistence and watchdog/BOR enable
