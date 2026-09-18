# Consumption fingerprint

Maps the Flatbox / SOLVE slide **Autonomous Urban Water Utilities** onto this meter.

Architecture figures: [`ARCHITECTURE.md`](ARCHITECTURE.md) · [system](diagrams/01-system-flow.png) · [MQTT](diagrams/03-mqtt-flow.png) · [Python fit](diagrams/04-ml-analysis-flow.png).

## Model (slide step 2–3)

\[
W(t)=\bar{w}+A_1\sin\left(\frac{2\pi}{T_1}t+\varphi_1\right)+A_2\sin\left(\frac{2\pi}{T_2}t+\varphi_2\right)
\]

| Symbol | MQTT key | Default | Meaning |
|--------|----------|---------|---------|
| \(\bar{w}\) | `w` | fitted | mean use (L/min) |
| \(A_1,\varphi_1\) | `a1`,`p1` | fitted | daily harmonic |
| \(T_1\) | `t1` | 24 | hours (slide: 24 h) |
| \(A_2,\varphi_2\) | `a2`,`p2` | fitted | twice-daily harmonic |
| \(T_2\) | `t2` | 12 | hours (slide: 12 h) |
| \(t\) | RTC | — | hour of day, 0–24 |

Cloud **fits** \((\bar{w},A_1,\varphi_1,A_2,\varphi_2)\) on historical `lm` vs hour-of-day (`software/waterbox_cloud/fingerprint.py`, run by the Docker worker). The meter **does not fit**; it only evaluates \(W(t)\) and compares live flow. Offline CSV: `tools/fit_signature.py`.

## Onboard verdicts (slide step 4)

| Flag | JSON | Rule |
|------|------|------|
| night leak on site | `lk=1` | hour in `[0,5)` and `lm > W(t) + 0.15` |
| overconsumption | `oc=1` | daytime and `lm > W(t) + 0.50` |

That is slide **B** (unknown night water = loss at the owner) vs daytime excess. Slide **C** (network loss) needs more than one meter and stays in the cloud.

## MQTT

Publish the fingerprint during the device listen window (8 s after each data publish):

```
waterbox/<IMEI>/fp
{"w":0.42,"a1":0.31,"p1":-0.40,"a2":0.12,"p2":1.10,"t1":24,"t2":12}
```

Device telemetry:

```
waterbox/<IMEI>/data
{"imei":"...","lm":0.55,"mls":9,"tl_ml":1234,"ts":"...","v":1,"wm":0.40,"lk":0,"oc":1}
```

`wm` is \(W(t)\) at the RTC hour. Fingerprint is stored in EEPROM and reused after reboot.

Radio must be enabled (`mqttUser`/`mqttPass` or `MQTT_ALLOW_ANONYMOUS 1`) or the cloud can never push a fingerprint.
