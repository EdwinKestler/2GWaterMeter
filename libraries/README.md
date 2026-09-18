# Arduino libraries

Point the Arduino IDE sketchbook at the repo root, or copy/symlink these folders into `~/Arduino/libraries/`.

| Library | Used by current firmware |
|---------|--------------------------|
| `sim800` | Yes — SIM800 AT/TCP |
| `GSMPubSubClient` | Yes — MQTT over `sim800Client` (`MQTT_MAX_PACKET_SIZE` 256) |
| `Time` | Yes — timestamp from modem RTC |
| `TimeAlarms` | No (interval is `millis()` so publishes work before RTC is set) |
| `ArduinoJson` | No (payload is `snprintf` + `dtostrf`) |

`ArduinoJson` is kept as a vendored copy of v5 for older sketches; the meter firmware does not compile against it.
