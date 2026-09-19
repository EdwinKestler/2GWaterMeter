"""Run one fingerprint pass against Postgres and publish retained …/fp messages."""

from __future__ import annotations

import logging
from datetime import datetime, timedelta, timezone
from zoneinfo import ZoneInfo

import paho.mqtt.client as mqtt

from . import config, db
from .fingerprint import fit, to_mqtt_json

log = logging.getLogger("waterbox.fit_once")


def main():
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    conn = db.connect()
    tz = ZoneInfo(config.TIMEZONE)
    since = datetime.now(timezone.utc) - timedelta(days=config.FIT_LOOKBACK_DAYS)
    client = mqtt.Client(client_id="waterbox-fit-once", clean_session=True)
    client.username_pw_set(config.MQTT_USER, config.MQTT_PASS)
    client.connect(config.MQTT_HOST, config.MQTT_PORT, keepalive=30)
    n = 0
    for imei in db.meter_ids(conn):
        rows = db.samples_for_fit(conn, imei, since)
        if len(rows) < config.FIT_MIN_SAMPLES:
            log.info("skip %s samples=%s", imei, len(rows))
            continue
        hours, lms = [], []
        for row in rows:
            ts = row["ts"]
            if ts.tzinfo is None:
                ts = ts.replace(tzinfo=timezone.utc)
            local = ts.astimezone(tz)
            hours.append(local.hour + local.minute / 60.0 + local.second / 3600.0)
            lms.append(float(row["lm"]))
        try:
            params = fit(hours, lms)
        except ValueError as exc:
            log.warning("fit %s: %s", imei, exc)
            continue
        db.save_fingerprint(conn, imei, params)
        topic = "{}/{}/fp".format(config.TOPIC_ROOT, imei)
        body = to_mqtt_json(params)
        client.publish(topic, body, qos=0, retain=True)
        db.mark_published(conn, imei)
        log.info("published %s n=%s", topic, params.get("n_samples"))
        n += 1
    client.disconnect()
    log.info("fitted %s meters", n)


if __name__ == "__main__":
    main()
