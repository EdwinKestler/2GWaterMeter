-- Single database "waterbox":
--   worker writes meters / readings / fingerprints / heartbeats
--   portal writes portal_users / portal_meters (login, address, geofence)
--   portal + Grafana read the same tables for household charts and maps
-- Do not add a second Postgres for the user portal.

CREATE TABLE IF NOT EXISTS meters (
    imei TEXT PRIMARY KEY,
    first_seen TIMESTAMPTZ NOT NULL DEFAULT now(),
    last_seen TIMESTAMPTZ NOT NULL DEFAULT now(),
    pulses_per_liter INTEGER,
    valve BOOLEAN,
    fp_valid BOOLEAN NOT NULL DEFAULT false
);

CREATE TABLE IF NOT EXISTS readings (
    id BIGSERIAL PRIMARY KEY,
    imei TEXT NOT NULL REFERENCES meters (imei),
    ts TIMESTAMPTZ NOT NULL,
    ingested_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    lm DOUBLE PRECISION,
    mls BIGINT,
    tl_ml BIGINT,
    valve BOOLEAN,
    wm DOUBLE PRECISION,
    wm_cloud DOUBLE PRECISION,
    lk SMALLINT NOT NULL DEFAULT 0,
    oc SMALLINT NOT NULL DEFAULT 0,
    raw JSONB NOT NULL DEFAULT '{}'::jsonb
);

CREATE INDEX IF NOT EXISTS readings_imei_ts_idx ON readings (imei, ts DESC);
CREATE INDEX IF NOT EXISTS readings_lk_idx ON readings (ts DESC) WHERE lk = 1;
CREATE INDEX IF NOT EXISTS readings_oc_idx ON readings (ts DESC) WHERE oc = 1;

CREATE TABLE IF NOT EXISTS fingerprints (
    id BIGSERIAL PRIMARY KEY,
    imei TEXT NOT NULL REFERENCES meters (imei),
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    w DOUBLE PRECISION NOT NULL,
    a1 DOUBLE PRECISION NOT NULL,
    p1 DOUBLE PRECISION NOT NULL,
    t1 DOUBLE PRECISION NOT NULL DEFAULT 24,
    a2 DOUBLE PRECISION NOT NULL,
    p2 DOUBLE PRECISION NOT NULL,
    t2 DOUBLE PRECISION NOT NULL DEFAULT 12,
    n_samples INTEGER,
    published BOOLEAN NOT NULL DEFAULT false
);

CREATE INDEX IF NOT EXISTS fingerprints_imei_created_idx ON fingerprints (imei, created_at DESC);

CREATE TABLE IF NOT EXISTS heartbeats (
    id BIGSERIAL PRIMARY KEY,
    imei TEXT NOT NULL REFERENCES meters (imei),
    ts TIMESTAMPTZ NOT NULL,
    ingested_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    uptime_s BIGINT,
    valve BOOLEAN,
    fp_valid BOOLEAN,
    lk SMALLINT NOT NULL DEFAULT 0,
    oc SMALLINT NOT NULL DEFAULT 0,
    rssi INTEGER,
    tl_ml BIGINT,
    ok SMALLINT NOT NULL DEFAULT 1,
    raw JSONB NOT NULL DEFAULT '{}'::jsonb
);

CREATE INDEX IF NOT EXISTS heartbeats_imei_ts_idx ON heartbeats (imei, ts DESC);

CREATE TABLE IF NOT EXISTS portal_users (
    id SERIAL PRIMARY KEY,
    email TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS portal_meters (
    user_id INTEGER NOT NULL REFERENCES portal_users (id) ON DELETE CASCADE,
    imei TEXT NOT NULL REFERENCES meters (imei) ON DELETE CASCADE,
    label TEXT,
    address TEXT,
    lat DOUBLE PRECISION,
    lon DOUBLE PRECISION,
    geofence_m DOUBLE PRECISION NOT NULL DEFAULT 500,
    PRIMARY KEY (user_id, imei)
);

CREATE INDEX IF NOT EXISTS portal_meters_geo_idx ON portal_meters (lat, lon)
    WHERE lat IS NOT NULL AND lon IS NOT NULL;

CREATE OR REPLACE VIEW v_household_monthly AS
SELECT
    u.email,
    pm.imei,
    pm.address,
    pm.lat,
    pm.lon,
    date_trunc('month', r.ts) AS month,
    COALESCE(NULLIF(MAX(r.tl_ml) - MIN(r.tl_ml), 0), SUM(r.mls), 0)::bigint AS ml
FROM portal_users u
JOIN portal_meters pm ON pm.user_id = u.id
LEFT JOIN readings r ON r.imei = pm.imei
GROUP BY u.email, pm.imei, pm.address, pm.lat, pm.lon, date_trunc('month', r.ts);
