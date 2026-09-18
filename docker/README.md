# One-house lab (Compose)

This is the supported deploy: **your house, one Core+, one IMEI**, all services on this machine.

**One Postgres.** The worker, Grafana, and the household portal all use database `waterbox`. Registration (`portal_users`, `portal_meters`) lives next to `readings` / `fingerprints` / `heartbeats` — the portal does not get its own database.

```bash
cd docker
cp .env.example .env
docker compose up --build
```

| URL | What |
|-----|------|
| http://localhost:3001 | Household portal — login, charts, fingerprint, map |
| http://localhost:3000 | Grafana (you only — not the household UI) |
| http://localhost:15672 | RabbitMQ  (`waterbox` / `waterbox`) |
| `:1883` | MQTT for the SIM800 |

## Meter

1. Set `mqttServer` in `firmware/waterbox/settings.h` to **this PC’s LAN IP** (not `localhost` — the Core+ is another device).
2. `mqttUser` / `mqttPass` are already `waterbox` / `waterbox`.
3. Flash, open serial at 57600, copy the `IMEI:` line.
4. Open http://localhost:3001 → Register with that IMEI and your home address.

Charts fill after the first MQTT `data` publishes. The worker fits a fingerprint after **12 samples** (~12 minutes at a 60 s interval in this lab `.env`). Heartbeat is hourly on `waterbox/<IMEI>/hb`.

The map heatmap is not meaningful with a single house (you compare to yourself). Address geocoding still places **your** meter on OSM.
