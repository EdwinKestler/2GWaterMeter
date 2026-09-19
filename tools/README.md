# MQTT demo replay

`simulate_mqtt.py` reads public household datasets under `demo_water_meter_databases/` and publishes firmware-shaped JSON to RabbitMQ MQTT (`waterbox/<IMEI>/info|data|hb`).

```bash
pip install paho-mqtt==1.6.1
# stack must be up (docker compose in ../docker)
python3 tools/simulate_mqtt.py --source daiad --meters 3 --hours 504
```

| `--source` | File | Native resolution |
|------------|------|-------------------|
| `daiad` (default) | Alicante smart meters, `user.key;datetime;meter.reading;diff` | hourly litres |
| `kaggle` | daily household totals, expanded with a diurnal profile | daily |
| `weusedto` | STREaM 10 s totals, aggregated to hours | 10 s |

UUID / household ids are hashed to 15-digit IMEIs. After replay, run one fingerprint pass:

```bash
docker compose -f docker/docker-compose.yml exec worker python -m waterbox_cloud.fit_once
```

`demo_water_meter_databases/` is gitignored (≈2 GB). Keep it local.
