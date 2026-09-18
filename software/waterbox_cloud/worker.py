from __future__ import annotations

import json
import logging
import threading
import time
from datetime import datetime, timedelta, timezone
from zoneinfo import ZoneInfo

import paho.mqtt.client as mqtt

from . import config, db, ingest
from .fingerprint import fit, to_mqtt_json

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(name)s %(levelname)s %(message)s")
log = logging.getLogger("waterbox.worker")


class CloudWorker:
    def __init__(self):
        self.conn = None
        self.client = mqtt.Client(client_id=config.MQTT_CLIENT_ID, clean_session=True)
        self.client.username_pw_set(config.MQTT_USER, config.MQTT_PASS)
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.client.on_disconnect = self._on_disconnect

    def _on_connect(self, client, userdata, flags, rc):
        if rc != 0:
            log.error("mqtt connect rc=%s", rc)
            return
        data = "{}/+/data".format(config.TOPIC_ROOT)
        info = "{}/+/info".format(config.TOPIC_ROOT)
        hb = "{}/+/hb".format(config.TOPIC_ROOT)
        client.subscribe([(data, 0), (info, 0), (hb, 0)])
        log.info("mqtt up, subscribed %s %s %s", data, info, hb)

    def _on_disconnect(self, client, userdata, rc):
        log.warning("mqtt disconnected rc=%s", rc)

    def _on_message(self, client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode("utf-8"))
        except (ValueError, UnicodeDecodeError):
            log.warning("bad json on %s", msg.topic)
            return
        if not isinstance(payload, dict):
            return
        try:
            ingest.handle_payload(self.conn, msg.topic, payload)
        except Exception:
            log.exception("ingest failed %s", msg.topic)

    def fit_loop(self):
        tz = ZoneInfo(config.TIMEZONE)
        while True:
            time.sleep(config.FIT_EVERY_SECONDS)
            try:
                self._fit_all(tz)
            except Exception:
                log.exception("fingerprint pass failed")

    def _fit_all(self, tz):
        since = datetime.now(timezone.utc) - timedelta(days=config.FIT_LOOKBACK_DAYS)
        for imei in db.meter_ids(self.conn):
            rows = db.samples_for_fit(self.conn, imei, since)
            if len(rows) < config.FIT_MIN_SAMPLES:
                log.info("skip fit %s samples=%s", imei, len(rows))
                continue
            hours = []
            lms = []
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
            db.save_fingerprint(self.conn, imei, params)
            topic = "{}/{}/fp".format(config.TOPIC_ROOT, imei)
            body = to_mqtt_json(params)
            result = self.client.publish(topic, body, qos=0, retain=True)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                db.mark_published(self.conn, imei)
                log.info("published %s %s", topic, body)
            else:
                log.error("publish %s rc=%s", topic, result.rc)

    def run(self):
        log.info("connecting postgres %s:%s/%s", config.PGHOST, config.PGPORT, config.PGDATABASE)
        self.conn = db.connect()
        log.info("connecting mqtt %s:%s", config.MQTT_HOST, config.MQTT_PORT)
        self.client.connect(config.MQTT_HOST, config.MQTT_PORT, keepalive=60)
        threading.Thread(target=self.fit_loop, name="fingerprint", daemon=True).start()
        self.client.loop_forever()


def main():
    CloudWorker().run()


if __name__ == "__main__":
    main()
