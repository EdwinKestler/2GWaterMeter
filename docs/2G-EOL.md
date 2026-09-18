# 2G end of life — do not field-deploy without a carrier check

This project’s only wide-area link is a **SIM800 on 2G GSM/GPRS** (original APN `m2mgt.tigo.com`, Tigo Guatemala). That was a reasonable 2016 choice. It is not a 2026 product plan.

## Why this blocks a rollout

- Carriers worldwide have shut down or scheduled shutdown of 2G (and often 3G) to reclaim spectrum for LTE/5G. A box that can only speak 2G will go dark on a date you do not control.
- SIM800 has no LTE, LTE-M, or NB-IoT radio. There is no firmware flag that “turns on 4G”.
- MQTT in this sketch is plaintext TCP port 1883. Even where 2G still exists, traffic is unauthenticated by default and inspectable on the air interface.
- Always-on 2G attach is the dominant energy cost. The firmware now tears the radio down between publishes, but attach/GPRS start is still seconds of high draw — off-grid sizing must use measured traces, not datasheet idle current.

## Required gate before any field unit

1. Written confirmation from the target MNO that 2G (GSM 900/1800 as used by SIM800) remains available for the device lifetime you are selling.
2. A dated sunset clause in the support plan.
3. MQTT TLS + per-device credentials (empty `mqttUser`/`mqttPass` is lab-only).
4. A hardware revision path off SIM800 (see below). Do not buy a large SIM800 reel without that path.

## Migration path (hardware)

Replace the Microduino GSM shield, not the flow/valve hydraulics:

| Option | When to use | Sketch impact |
|--------|-------------|----------------|
| LTE-M / NB-IoT modem (SARA-R4, BG95, SIM7080, …) | Utility metering, deep coverage | New UART AT client; keep MQTT and pulse math |
| LoRaWAN class A | Dense city / campus with a network server | Replace MQTT with LoRa payload; valve command latency becomes class-A receive windows |
| Wi-Fi / Ethernet | Indoor / plant room only | Not a street meter |

Keep `PULSES_PER_LITER`, millilitre totals, per-IMEI (or per-DevEUI) topics, and the latching-valve pulse. Throw away `libraries/sim800` and `GSMPubSubClient` together when the radio changes.

## Firmware already assumes 2G is hostile to battery life

`firmware/waterbox/waterbox.ino` does **not** hold a TCP session. If you are still on SIM800 in the lab, treat that as a compatibility mode, not as the architecture to scale.
