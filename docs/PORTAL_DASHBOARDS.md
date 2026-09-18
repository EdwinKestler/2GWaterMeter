# Should the user portal host dashboards and maps?

Research date: 2026-09. **Yes.** Household charts and the georeferenced map belong in `user_portal`. Grafana stays an **operator** tool on :3000, not the household login.

## What “dashboarding and mapping” means here

| Surface | Audience | Needs |
|---------|----------|--------|
| Consumption graphs (hour/day/week/month) | Homeowner | IMEI-scoped, simple, branded |
| Monthly fingerprint overlay | Homeowner | \(W(t)\) vs other months |
| Address, geofence, heatmap vs neighbors | Homeowner | Write lat/lon; OSM; below/normal/above |
| All meters, SQL, alerts, raw `lk`/`oc` | You (lab / utility ops) | Grafana |

## Options

### A. User portal hosts charts + map (chosen)

The Next.js app already does this: Recharts + Leaflet/OSM, cookie auth, same Postgres.

Industry split in 2026 is the same: **Grafana (or similar) for ops; a custom app for the customer portal.** Grafana is a visualization layer with no device identity, weak per-customer login, and no “save my street address” workflow. ThingsBoard-style IoT apps exist, but you already have IMEI login, fingerprint overlay, and geofence logic — duplicating that in Grafana panels is worse UX, not less work.

Leaflet + OSM is the right map stack at this scale (one house now, a city later). MapLibre/Mapbox is for heavy vector heatmaps; Grafana Geomap is for ops, not IMEI-gated households.

### B. Grafana as the household UI

| | |
|--|--|
| Speed to first chart | Fast (already in Compose) |
| IMEI/email registration | Poor (orgs/users are ops accounts, not households) |
| Write address / geocode / geofence | Grafana is read-only |
| Per-meter isolation | Easy to leak other IMEIs via dashboard variables |
| Fingerprint overlay \(W(t)\) | Awkward (SQL-generated series, not a product page) |
| Public dashboard share | Unauthenticated or token URL, not “my meter login” |

Grafana **public dashboards** (2024+) are for sharing a read-only ops view, not a utility bill portal.

### C. Hybrid: portal shells, Grafana iframe

Looks quick; you inherit Grafana auth, CSP, and a UI that still cannot save an address. Skip unless you only need a read-only wall display.

## Lab (your house, one meter)

Still put charts and the map **in the portal**:

- You exercise the real product path (register IMEI → see your litres).
- The map with one point is expected; the band is “normal” because the median is you. That is honest, not a reason to move mapping into Grafana.
- Grafana remains useful **for you**: all SQL, heartbeats, worker health, every IMEI if you add a second later.

Do not send household users to :3000.

## Decision

| Host | What |
|------|------|
| **`user_portal` :3001** | Login, consumption charts, fingerprint overlay, address, geofence, OSM map |
| **Grafana :3000** | Operator dashboards only |
| **Not Vercel / not a third map SaaS** | Same Compose Postgres; OSM tiles are enough |

Revisit only if you have thousands of points on the map (then MapLibre + a tile/heatmap pipeline) or a real multi-tenant utility (then harden portal RBAC; still not Grafana for customers).
