# Where to run the user portal

Research date: 2026-09.

**Chosen for now:** Docker Compose on the lab machine. One house, one meter. Vercel is deferred until there is more than a single household and a public Postgres.

## What the portal needs

| Need | Why it rules out a naive Vercel deploy |
|------|----------------------------------------|
| Reads `readings`, `fingerprints`, `heartbeats` | Data lives in the Compose Postgres |
| Writes `portal_users`, `portal_meters` (address, lat/lon, geofence) | Must be the **same** database the MQTT worker uses |
| Geocodes via Nominatim, draws OSM tiles | Fine from anywhere |
| Auth cookies | Fine on Vercel or Docker |
| No MQTT in the browser | Portal never talks to RabbitMQ |

The portal is a Next.js 14 app (`output: "standalone"`) in `user_portal/`. It is already a Compose service on **port 3001**. **Household charts and mapping live here**, not in Grafana. Assessment: [`PORTAL_DASHBOARDS.md`](PORTAL_DASHBOARDS.md). Grafana stays on 3000 (operators only).

## Options assessed

### 1. Docker Compose on the same cloud VM (recommended)

Put `portal` next to `postgres` on the internal Docker network. Postgres is not exposed to the public internet except if you already published `5432` (close that in production). Put **Caddy or nginx** in front with TLS:

```
household.example.com  →  portal:3000
ops.example.com        →  grafana:3000
mqtt.example.com:1883  →  rabbitmq:1883   (or keep raw 1883)
```

| | |
|--|--|
| Cost | ~$0 extra on the VM you already run for 2G ingest |
| Latency | LAN to Postgres, no cold starts |
| Ops | One `docker compose up`, one backup (pg_dump volume) |
| Fit | Matches current 2G/MQTT/SIM800 architecture |

Next.js self-hosting in 2026 is a supported first-class path (`output: "standalone"`). Vercel is optional, not required.

### 2. Vercel (or Cloudflare Pages + OpenNext)

Use only if you **also** move the database to a public host (Neon, Supabase, RDS public, or Cloudflare Hyperdrive in front of a reachable Postgres).

Then:

- Set Vercel root directory to `user_portal/`
- `DATABASE_URL` = that public Postgres
- `AUTH_SECRET` in Vercel env
- Worker still runs in Docker next to RabbitMQ, pointed at the **same** database URL

Problems if you skip that:

- Vercel serverless cannot reach `postgres:5432` inside Compose
- Exposing Postgres to `0.0.0.0` for Vercel is a worse security posture than a local portal container
- Preview deploys would share production meter data unless you clone the DB

Vercel Pro is typically $20/user/month plus function/bandwidth; a Hetzner/DigitalOcean VM already running MQTT is a few euros and already paid.

Stay on Vercel when you want Git previews, a global edge, and you are willing to pay for **managed Postgres + pooling** (Neon + Vercel). That is a product-company shape, not this prototype.

### 3. Split: Vercel frontend, Docker API

Possible later: FastAPI in Compose, Next.js static on Vercel. Extra hop, two deploys, CORS, duplicated auth. Not worth it until you have a separate frontend team.

## Decision

| Stage | Deploy |
|-------|--------|
| Lab / first field trial | Compose `portal` on :3001, HTTP only |
| Public household login | Same VM + Caddy TLS on a hostname |
| Scale / multiple regions | Managed Postgres, then Vercel or Fly.io for the Next app |

Do **not** put the MQTT broker on Vercel. SIM800 needs a stable TCP :1883 (or 8883) on a VM/IP.

## Run

```bash
cd docker
cp .env.example .env   # set AUTH_SECRET
docker compose up --build
# portal  http://localhost:3001
# grafana http://localhost:3000
```

Local Node without Docker (Postgres must already be up):

```bash
cd user_portal
cp .env.example .env
npm install
npm run dev
```
