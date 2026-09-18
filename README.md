# 2GWaterMeter

Open-source hardware water meter: RPE flow body and latching valve in a junction box, Microduino Core+ (ATmega644PA), SIM800 2G, MQTT.

This is a **2016 prototype**. It is not ready to deploy. 2G is end-of-life in most markets — read [`docs/2G-EOL.md`](docs/2G-EOL.md) before buying SIMs or enclosures.

## Layout

```
firmware/waterbox/     Arduino sketch: FSM + class modules (open this folder in the IDE)
libraries/             sim800, GSMPubSubClient, Time, TimeAlarms, ArduinoJson
hardware/cad/          SketchUp 2014 model
hardware/photos/       as-built photos (restored)
hardware/renders/      SketchUp screenshots
hardware/datasheets/   RPE meter, RPE coils, L9110
hardware/schematics/   vendor Eagle files for Core+ and SIM800
docs/                  BOM, wiring, 2G sunset
```

## What the firmware does now

1. **Volume** is millilitres from pulse count. RPE BVS_RF-3-30L calibration is **245 pulses/L (Hall)** or **490 (Reed)** (`FLOW_SENSOR_REED` in `settings.h`). Totals are `tl_ml` in JSON — not litres mislabeled from a YF-S201 example.
2. **Flow input is D6**, mapped with `digitalPinToInterrupt(PIN_FLOW)` (Core+ external interrupts: D2, D3, D6; D2/D3 are Serial1).
3. **JSON is compact** (`snprintf`/`dtostrf`), MQTT buffer is 256 bytes, `flujo = random()` is gone, timestamp comes from the SIM800 RTC (`unset` until the modem clock is valid).
4. **Topics are per IMEI**: `waterbox/<IMEI>/data`, `/cmd`, `/info`. Command payload `on` / `off`. Set `mqttUser` / `mqttPass` for any non-lab broker.
5. **Radio is down between publishes.** Bounded retries (`RADIO_RETRY_MAX`). After a publish the device listens `CMD_LISTEN_MS` (8 s) for a valve command, then `CIPCLOSE`/`CIPSHUT`. Pulses keep counting in the ISR while the radio sleeps.

## Build

Arduino IDE: install a Core+ / ATmega644PA core, copy `libraries/*` into the IDE libraries folder, open `firmware/waterbox/waterbox.ino`.

```ini
; optional PlatformIO (16 MHz Core+; use 644pa8m for 8 MHz 3.3 V boards)
[env:microduino_coreplus]
platform = atmelavr
board = 644pa16m
framework = arduino
```

## Hardware

- BOM: [`docs/BOM.md`](docs/BOM.md)
- Wiring: [`docs/WIRING.md`](docs/WIRING.md)
- CAD vs prototype: [`hardware/cad/README.md`](hardware/cad/README.md)

STL/STEP were **not** exported here (SketchUp 2014 OLE file; no converter in this environment). Export from SketchUp into `hardware/cad/exports/`.

## License

GPL-3.0 (`LICENSE`). `libraries/sim800` is GPL-2-or-later; MQTT client is Nick O’Leary’s PubSubClient (see that folder’s license).
