from __future__ import annotations

import json
import logging
from datetime import datetime
from zoneinfo import ZoneInfo

from . import config, db
from .fingerprint import predict

log = logging.getLogger("waterbox.ingest")


def parse_device_time(raw, tz):
    if not raw or raw == "unset":
        return datetime.now(tz)
    text = str(raw).strip()
    for fmt in ("%Y-%m-%dT%H:%M:%S", "%Y-%m-%d %H:%M:%S"):
        try:
            return datetime.strptime(text, fmt).replace(tzinfo=tz)
        except ValueError:
            continue
    return datetime.now(tz)


def imei_from_topic(topic):
    parts = topic.split("/")
    if len(parts) >= 3 and parts[0] == config.TOPIC_ROOT:
        return parts[1]
    return None


def handle_payload(conn, topic, payload):
    tz = ZoneInfo(config.TIMEZONE)
    imei = imei_from_topic(topic) or str(payload.get("imei") or "")
    if not imei:
        log.warning("no imei on %s", topic)
        return
    if topic.endswith("/info"):
        db.upsert_meter(conn, imei, payload)
        log.info("info %s", imei)
        return
    if topic.endswith("/hb"):
        ts = parse_device_time(payload.get("ts"), tz)
        db.insert_heartbeat(conn, imei, ts, payload)
        log.info("heartbeat %s up=%s rssi=%s ok=%s", imei, payload.get("up"), payload.get("rssi"), payload.get("ok"))
        return
    if not topic.endswith("/data"):
        return
    ts = parse_device_time(payload.get("ts"), tz)
    hour = ts.astimezone(tz).hour + ts.minute / 60.0 + ts.second / 3600.0
    fp = db.latest_fingerprint(conn, imei)
    wm_cloud = predict(fp, hour) if fp else None
    db.insert_reading(conn, imei, ts, payload, wm_cloud)
    log.info("reading %s lm=%s lk=%s oc=%s", imei, payload.get("lm"), payload.get("lk"), payload.get("oc"))
