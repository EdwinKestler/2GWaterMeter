# Household user portal

Next.js app: IMEI/email login, consumption charts, monthly fingerprint overlay, address geocoding, OSM heatmap with geofence comparison.

Uses the **same Postgres** as the MQTT worker (`waterbox`). Accounts and addresses are extra tables; charts and fingerprints are `SELECT`s on `readings` / `fingerprints`.

Default deploy is the **Compose `portal` service** on port **3001**. This app **hosts household dashboards and the map**; Grafana is operator-only. Why: [`docs/PORTAL_DASHBOARDS.md`](../docs/PORTAL_DASHBOARDS.md). Docker vs Vercel: [`docs/DEPLOYMENT.md`](../docs/DEPLOYMENT.md).

## Features

- Register with email, password, and meter **IMEI** (optional street address)
- Login with **email or IMEI**
- Hourly / daily / weekly / monthly volume (from `readings.tl_ml` / `mls`)
- Monthly fingerprints overlaid as \(W(t)\) vs hour of day
- Save address → Nominatim geocode → lat/lon
- Geofence radius vs other registered meters this month: below / normal / above city median

## API (cookie session)

| Method | Path |
|--------|------|
| POST | `/api/register` `{email,password,imei,address?}` |
| POST | `/api/login` `{identifier,password}` |
| POST | `/api/logout` |
| GET | `/api/meters` |
| GET | `/api/consumption?imei=&range=hourly\|daily\|weekly\|monthly` |
| GET | `/api/fingerprint?imei=` |
| POST | `/api/address` `{imei,address,geofence_m}` |
| GET | `/api/heatmap` |
