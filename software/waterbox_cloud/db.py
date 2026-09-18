from __future__ import annotations

import json

import psycopg
from psycopg.rows import dict_row

from . import config


def connect():
    conn = psycopg.connect(
        host=config.PGHOST,
        port=config.PGPORT,
        dbname=config.PGDATABASE,
        user=config.PGUSER,
        password=config.PGPASSWORD,
        row_factory=dict_row,
        autocommit=True,
    )
    conn.execute(
        """
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
        )
        """
    )
    conn.execute(
        "CREATE INDEX IF NOT EXISTS heartbeats_imei_ts_idx ON heartbeats (imei, ts DESC)"
    )
    return conn


def upsert_meter(conn, imei, info=None):
    info = info or {}
    conn.execute(
        """
        INSERT INTO meters (imei, first_seen, last_seen, pulses_per_liter, valve, fp_valid)
        VALUES (%s, now(), now(), %s, %s, COALESCE(%s, false))
        ON CONFLICT (imei) DO UPDATE SET
          last_seen = now(),
          pulses_per_liter = COALESCE(EXCLUDED.pulses_per_liter, meters.pulses_per_liter),
          valve = COALESCE(EXCLUDED.valve, meters.valve),
          fp_valid = COALESCE(EXCLUDED.fp_valid, meters.fp_valid)
        """,
        (
            imei,
            info.get("ppl"),
            _as_bool(info.get("v")),
            _as_bool(info.get("fp")),
        ),
    )


def insert_reading(conn, imei, ts, payload, wm_cloud):
    upsert_meter(conn, imei, payload)
    conn.execute(
        """
        INSERT INTO readings (
          imei, ts, lm, mls, tl_ml, valve, wm, wm_cloud, lk, oc, raw
        ) VALUES (
          %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s::jsonb
        )
        """,
        (
            imei,
            ts,
            _as_float(payload.get("lm")),
            _as_int(payload.get("mls")),
            _as_int(payload.get("tl_ml")),
            _as_bool(payload.get("v")),
            _as_float(payload.get("wm")),
            wm_cloud,
            _as_int(payload.get("lk")) or 0,
            _as_int(payload.get("oc")) or 0,
            json.dumps(payload),
        ),
    )


def latest_fingerprint(conn, imei):
    row = conn.execute(
        """
        SELECT w, a1, p1, t1, a2, p2, t2, n_samples, created_at
        FROM fingerprints
        WHERE imei = %s
        ORDER BY created_at DESC
        LIMIT 1
        """,
        (imei,),
    ).fetchone()
    return row


def save_fingerprint(conn, imei, params):
    conn.execute(
        """
        INSERT INTO fingerprints (imei, w, a1, p1, t1, a2, p2, t2, n_samples, published)
        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, false)
        """,
        (
            imei,
            params["w"],
            params["a1"],
            params["p1"],
            params["t1"],
            params["a2"],
            params["p2"],
            params["t2"],
            params.get("n_samples"),
        ),
    )
    conn.execute("UPDATE meters SET fp_valid = true, last_seen = now() WHERE imei = %s", (imei,))


def mark_published(conn, imei):
    conn.execute(
        """
        UPDATE fingerprints SET published = true
        WHERE id = (
          SELECT id FROM fingerprints WHERE imei = %s ORDER BY created_at DESC LIMIT 1
        )
        """,
        (imei,),
    )


def samples_for_fit(conn, imei, since):
    return conn.execute(
        """
        SELECT ts, lm
        FROM readings
        WHERE imei = %s AND ts >= %s AND lm IS NOT NULL
        ORDER BY ts
        """,
        (imei, since),
    ).fetchall()


def insert_heartbeat(conn, imei, ts, payload):
    upsert_meter(conn, imei, {"v": payload.get("v"), "fp": payload.get("fp")})
    conn.execute(
        """
        INSERT INTO heartbeats (
          imei, ts, uptime_s, valve, fp_valid, lk, oc, rssi, tl_ml, ok, raw
        ) VALUES (
          %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s::jsonb
        )
        """,
        (
            imei,
            ts,
            _as_int(payload.get("up")),
            _as_bool(payload.get("v")),
            _as_bool(payload.get("fp")),
            _as_int(payload.get("lk")) or 0,
            _as_int(payload.get("oc")) or 0,
            _as_int(payload.get("rssi")),
            _as_int(payload.get("tl_ml")),
            _as_int(payload.get("ok")) if payload.get("ok") is not None else 1,
            json.dumps(payload),
        ),
    )


def meter_ids(conn):
    rows = conn.execute("SELECT imei FROM meters ORDER BY imei").fetchall()
    return [row["imei"] for row in rows]


def _as_float(value):
    if value is None or value == "":
        return None
    return float(value)


def _as_int(value):
    if value is None or value == "":
        return None
    return int(value)


def _as_bool(value):
    if value is None:
        return None
    if isinstance(value, bool):
        return value
    return int(value) != 0
