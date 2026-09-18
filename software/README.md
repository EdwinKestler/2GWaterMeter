# Waterbox cloud

Python worker plus the Docker stack: **RabbitMQ MQTT plugin**, **PostgreSQL**, **Grafana**.

```
meter --MQTT:1883--> RabbitMQ --paho--> worker --SQL--> Postgres <-- Grafana
                         ^                |
                         |                +-- fit W(t) --MQTT retain--> waterbox/<IMEI>/fp
                         +---- fingerprint --------------------+
```

Figures: [system](../docs/diagrams/01-system-flow.png) · [database](../docs/diagrams/02-database-flow.png) · [MQTT](../docs/diagrams/03-mqtt-flow.png) · [fingerprint fit](../docs/diagrams/04-ml-analysis-flow.png). Narrative: [`docs/ARCHITECTURE.md`](../docs/ARCHITECTURE.md).

## Run

```bash
cd docker
cp .env.example .env
docker compose up --build
```

| Service | URL / port |
|---------|------------|
| MQTT (meters) | `1883` user/pass `waterbox` / `waterbox` |
| RabbitMQ UI | http://localhost:15672 |
| Postgres | `5432` db `waterbox` |
| Grafana (operators) | http://localhost:3000 admin / admin |
| Household portal | http://localhost:3001 |

Firmware `settings.h` for this broker:

```c
const char mqttServer[] = "<cloud-host>";
const uint16_t mqttPort = 1883;
const char mqttUser[] = "waterbox";
const char mqttPass[] = "waterbox";
#define MQTT_ALLOW_ANONYMOUS 0
```

## Worker

`python -m waterbox_cloud.worker` (inside the `worker` container):

1. Subscribes `waterbox/+/data`, `waterbox/+/info`, and `waterbox/+/hb`.
2. Upserts `meters`, inserts `readings` (`lm`, `tl_ml`, onboard `wm`/`lk`/`oc`, plus `wm_cloud` from the latest fitted fingerprint).
3. Every `FIT_EVERY_SECONDS` (default 1 h) fits  
   \(W(t)=\bar{w}+A_1\sin(2\pi t/24+\varphi_1)+A_2\sin(2\pi t/12+\varphi_2)\)  
   on the last `FIT_LOOKBACK_DAYS` of samples (min `FIT_MIN_SAMPLES`).
4. Stores coefficients in `fingerprints` and publishes retained JSON to `waterbox/<IMEI>/fp` so the Core+ can validate onboard during its listen window.

## Local Python without Docker

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements.txt
export MQTT_HOST=localhost PGHOST=localhost
python -m waterbox_cloud.worker
```
