# Hardware validation

## 2026-07-29 — Arduino Mega ADK on COM3

Validated the v1.7.0 native engine with
`test/hardware-validation/hardware-validation.ino`.

- Bundled AVR GCC initialization: passed
- Fresh compile: passed, 1/1 translation unit, 521.0739 ms
- ATmega2560 signature check: passed
- STK500v2 synchronization: passed on the first attempt
- Flash programming: passed, 11 pages / 2816 bytes
- Full programmed-image readback: passed, 11 pages / 2816 bytes
- Upload and verification time: 1027.2147 ms
- Post-upload execution: passed
- Serial heartbeat at 115200 baud: three consecutive
  `MONOCON_HW_OK` messages received
- Serial open with DTR disabled and RTS disabled: passed
- Persistent content-cache hit after upload: passed, 0/1 recompiled,
  21.602 ms

The validation sketch only uses `LED_BUILTIN` and the board's USB serial
bridge. It does not drive external I/O pins.
