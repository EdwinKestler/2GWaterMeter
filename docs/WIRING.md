# Wiring

Microduino Core+ (ATmega644PA). Serial1 is D2 (RX1) + D3 (TX1) and **must** stay on the SIM800. External interrupts on this board are **D2, D3, and D6**; D6 is the only free INT pin, which is why the Hall sensor is on D6.

```
                    +------------------+
     water in ------| RPE body + Hall  |------ water out
                    |      valve       |
                    +--------+---------+
                             |
                     Hall out (open collector)
                             |
                             v
                         Core+ D6  (INT2, INPUT_PULLUP)
                         Core+ 5V  (Hall V+)
                         Core+ GND (Hall V-)

     Core+ D9  (PWM) ---- L9110 IA ----+
     Core+ D10 (PWM) ---- L9110 IB ----+-- latching coil
     coil supply 6 V (L6V) -- L9110 VCC
     GND common

     Core+ D2/D3 (Serial1) -- SIM800 UART
     Core+ D5 -- SIM800 PWRKEY
     Core+ D7 -- SIM800 RESET
     Core+ D13 -- status LED (on-board)
```

## Valve drive

`ValveOn` / `ValveOff` send a `VALVE_PULSE_MS` (25 ms) pulse through the L9110, then both outputs low. That is the RPE **latching L6V** profile. A holding coil will not stay actuated; change `VALVE_PULSE_MS` and the hold strategy if you fit a different SKU.

L9110 continuous current is ~800 mA. Stay under that for the chosen coil (L6V latching is ~375 mA; some 12 V windings are ~710 mA).

Message sequence: [`ARCHITECTURE.md`](ARCHITECTURE.md#3-mqtt-message-flow) · [MQTT flow figure](diagrams/03-mqtt-flow.png).

## MQTT topics (after IMEI is read)

| Direction | Topic | Payload |
|-----------|--------|---------|
| device → broker | `waterbox/<IMEI>/data` | JSON: `imei`, `lm`, `mls`, `tl_ml`, `ts`, `v`, `wm` (model L/min), `lk` (night leak), `oc` (overuse) |
| device → broker | `waterbox/<IMEI>/info` | JSON: `imei`, `ppl`, `reed`, `v`, `fp` (1 if fingerprint stored) |
| device → broker | `waterbox/<IMEI>/hb` | Hourly health ping: `imei`, `ts`, `up` (uptime s), `v`, `fp`, `lk`, `oc`, `rssi`, `tl_ml`, `ok` |
| broker → device | `waterbox/<IMEI>/cmd` | Valve: `on`/`open`/`1` or `off`/`close`/`0`, or `{"v":1}`. Only during `CMD_LISTEN_MS`. |
| broker → device | `waterbox/<IMEI>/fp` | Fingerprint JSON `w,a1,p1,a2,p2,t1,t2` (retained). See `docs/SIGNATURE.md`. |

Set `mqttUser` / `mqttPass` in `firmware/waterbox/settings.h`. With `MQTT_ALLOW_ANONYMOUS 0` (default) the radio will not start until both are non-empty. Lab-only: set `MQTT_ALLOW_ANONYMOUS` to 1.

D13 LED: solid = MQTT up; 200 ms blink = modem or auth failure; 80 ms heartbeat every 2 s = idle, extra blink if flow > 0.

## Radio duty cycle

Flow pulses are counted in the ISR at all times. About every `PUBLISH_INTERVAL_MS` (60 s) the firmware brings GPRS+MQTT up, publishes, listens for commands, then `CIPCLOSE` + `CIPSHUT`. Do not expect instant remote shutoff outside that window.
