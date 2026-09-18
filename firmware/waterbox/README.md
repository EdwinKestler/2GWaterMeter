# waterbox firmware modules

`waterbox.ino` only constructs `WaterboxFsm` and calls `begin()` / `tick()`.

| Module | Responsibility |
|--------|----------------|
| `settings.h` | Site knobs (APN, broker, pins, pulse/litre, timings) |
| `FlowMeter` | Hall/Reed ISR, pulse totals, L/min |
| `LatchingValve` | L9110 open/close pulse |
| `MeterStore` | EEPROM image (pulses + valve + CRC) |
| `GsmMqttClient` | SIM800 + MQTT; swap this class to change radios |
| `StatusLed` | D13 patterns |
| `Watchdog` | AVR WDT; no-ops on other cores |
| `WaterboxFsm` | Explicit states: HaltAuth, WaitModem, Idle, Connect, Publish, Listen, Disconnect |

State loop: Idle waits `PUBLISH_INTERVAL_MS` → Connect (GPRS+MQTT) → Publish → Listen (`CMD_LISTEN_MS`) → Disconnect → Idle. Pulses keep counting in every state.
