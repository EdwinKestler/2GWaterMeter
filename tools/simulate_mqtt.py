#!/usr/bin/env python3
"""Replay public smart-meter CSVs as waterbox MQTT telemetry.

Primary source: DAIAD Alicante hourly meters (user.key, datetime, meter.reading, diff).
diff is treated as litres in the hour; meter.reading as cumulative litres.

  python3 tools/simulate_mqtt.py --meters 3 --hours 336
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import sys
import time
from collections import defaultdict
from datetime import datetime, timedelta

try:
    import paho.mqtt.client as mqtt
except ImportError:
    sys.exit("install paho-mqtt: pip install paho-mqtt==1.6.1")

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DAIAD = os.path.join(
    ROOT,
    "tools/demo_water_meter_databases/DAIAD-Alicante-Smart-Water-Meter-Dataset/"
    "swm_trialA_1k_clean/swm_trialA_1k_clean.csv",
)
KAGGLE = os.path.join(
    ROOT,
    "tools/demo_water_meter_databases/Kaggle-Household-Water-Consumption/"
    "archive/household_water_consumption.csv",
)
WEUSEDTO = os.path.join(
    ROOT,
    "tools/demo_water_meter_databases/WEUSEDTO-Water-End-Use-Dataset-and-Tools/"
    "Water-Usage-Dataset/Dataset/Trainset.csv",
)

PROFILE = [
    0.02, 0.01, 0.01, 0.01, 0.02, 0.05, 0.08, 0.09,
    0.06, 0.04, 0.04, 0.05, 0.06, 0.05, 0.04, 0.04,
    0.05, 0.08, 0.09, 0.07, 0.05, 0.04, 0.03, 0.02,
]


def imei_from_key(key):
    digest = hashlib.sha1(str(key).encode("utf-8")).hexdigest()
    digits = "".join(ch for ch in digest if ch.isdigit()) + "000000000000000"
    body = digits[:15]
    if body[0] == "0":
        body = "8" + body[1:]
    return body


def parse_daiad_dt(text):
    for fmt in ("%d/%m/%Y %H:%M:%S", "%Y-%m-%d %H:%M:%S"):
        try:
            return datetime.strptime(text.strip(), fmt)
        except ValueError:
            continue
    raise ValueError(text)


def load_daiad(path, meters, hours):
    wanted = []
    buckets = defaultdict(list)
    with open(path, newline="") as fh:
        reader = csv.DictReader(fh, delimiter=";")
        for row in reader:
            key = row["user.key"]
            if key not in wanted:
                if len(wanted) >= meters:
                    if all(len(buckets[k]) >= hours for k in wanted):
                        break
                    continue
                wanted.append(key)
            if len(buckets[key]) >= hours:
                continue
            litres = float(row["diff"] or 0)
            total = float(row["meter.reading"] or 0)
            ts = parse_daiad_dt(row["datetime"])
            buckets[key].append((ts, litres, total))
    series = []
    for key in wanted:
        rows = sorted(buckets[key], key=lambda x: x[0])
        samples = []
        for ts, litres, total in rows:
            samples.append(
                {
                    "ts": ts,
                    "lm": litres / 60.0,
                    "mls": int(round(litres * 1000.0)),
                    "tl_ml": int(round(total * 1000.0)),
                }
            )
        series.append((imei_from_key(key), key, samples))
    return series


def load_kaggle(path, meters, hours):
    by_house = defaultdict(list)
    with open(path, newline="") as fh:
        reader = csv.DictReader(fh)
        for row in reader:
            hid = row["Household_ID"]
            day = datetime.strptime(row["Date"], "%Y-%m-%d")
            total = float(row["Total_Liters"])
            by_house[hid].append((day, total))
    series = []
    for hid, days in list(by_house.items())[:meters]:
        samples = []
        running = 0.0
        for day, total in sorted(days):
            for h, frac in enumerate(PROFILE):
                litres = total * frac
                running += litres
                samples.append(
                    {
                        "ts": day + timedelta(hours=h),
                        "lm": litres / 60.0,
                        "mls": int(round(litres * 1000.0)),
                        "tl_ml": int(round(running * 1000.0)),
                    }
                )
                if len(samples) >= hours:
                    break
            if len(samples) >= hours:
                break
        series.append((imei_from_key("kaggle-" + hid), hid, samples))
    return series


def load_weusedto(path, meters, hours):
    hourly = []
    acc = 0.0
    n = 0
    start = datetime(2024, 1, 1)
    with open(path, newline="") as fh:
        reader = csv.DictReader(fh)
        for row in reader:
            acc += float(row["TOTAL"] or 0)
            n += 1
            if n == 360:
                hourly.append(acc)
                acc = 0.0
                n = 0
            if len(hourly) >= hours:
                break
    samples = []
    running = 0.0
    for i, litres in enumerate(hourly):
        running += litres
        samples.append(
            {
                "ts": start + timedelta(hours=i),
                "lm": litres / 60.0,
                "mls": int(round(litres * 1000.0)),
                "tl_ml": int(round(running * 1000.0)),
            }
        )
    return [(imei_from_key("weusedto-train"), "weusedto", samples)][:meters]


def shift_to_now(series):
    latest = None
    for _, _, samples in series:
        if samples:
            end = samples[-1]["ts"]
            if latest is None or end > latest:
                latest = end
    if latest is None:
        return series
    delta = datetime.now() - latest
    out = []
    for imei, src, samples in series:
        shifted = []
        for sample in samples:
            item = dict(sample)
            item["ts"] = sample["ts"] + delta
            shifted.append(item)
        out.append((imei, src, shifted))
    return out


def publish_meter(client, imei, samples, sleep_s, heartbeat_every):
    info = {"imei": imei, "ppl": 245, "reed": 0, "v": 1, "fp": 0}
    client.publish("waterbox/{}/info".format(imei), json.dumps(info), qos=0)
    for i, sample in enumerate(samples, 1):
        payload = {
            "imei": imei,
            "lm": round(sample["lm"], 4),
            "mls": sample["mls"],
            "tl_ml": sample["tl_ml"],
            "ts": sample["ts"].strftime("%Y-%m-%dT%H:%M:%S"),
            "v": 1,
            "wm": 0,
            "lk": 1 if sample["ts"].hour < 5 and sample["lm"] > 0.2 else 0,
            "oc": 1 if 7 <= sample["ts"].hour <= 22 and sample["lm"] > 0.8 else 0,
        }
        client.publish("waterbox/{}/data".format(imei), json.dumps(payload), qos=0)
        if heartbeat_every and i % heartbeat_every == 0:
            hb = {
                "imei": imei,
                "ts": payload["ts"],
                "up": i * 3600,
                "v": 1,
                "fp": 0,
                "lk": payload["lk"],
                "oc": payload["oc"],
                "rssi": 18,
                "tl_ml": sample["tl_ml"],
                "ok": 1,
            }
            client.publish("waterbox/{}/hb".format(imei), json.dumps(hb), qos=0)
        if sleep_s:
            time.sleep(sleep_s)
    client.loop_write()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", choices=("daiad", "kaggle", "weusedto"), default="daiad")
    parser.add_argument("--csv", default="")
    parser.add_argument("--host", default=os.environ.get("MQTT_HOST", "127.0.0.1"))
    parser.add_argument("--port", type=int, default=int(os.environ.get("MQTT_PORT", "1883")))
    parser.add_argument("--user", default=os.environ.get("MQTT_USER", "waterbox"))
    parser.add_argument("--password", default=os.environ.get("MQTT_PASS", "waterbox"))
    parser.add_argument("--meters", type=int, default=3)
    parser.add_argument("--hours", type=int, default=24 * 21, help="samples per meter")
    parser.add_argument("--sleep", type=float, default=0.0, help="seconds between publishes")
    parser.add_argument("--heartbeat-every", type=int, default=24)
    parser.add_argument("--keep-original-time", action="store_true",
                        help="do not shift timestamps so the last sample is now")
    args = parser.parse_args()

    path = args.csv
    if args.source == "daiad":
        path = path or DAIAD
        series = load_daiad(path, args.meters, args.hours)
    elif args.source == "kaggle":
        path = path or KAGGLE
        series = load_kaggle(path, args.meters, args.hours)
    else:
        path = path or WEUSEDTO
        series = load_weusedto(path, args.meters, args.hours)

    if not series:
        sys.exit("no samples loaded from {}".format(path))
    if not args.keep_original_time:
        series = shift_to_now(series)

    client = mqtt.Client(client_id="waterbox-sim", clean_session=True)
    client.username_pw_set(args.user, args.password)
    client.connect(args.host, args.port, keepalive=60)
    client.loop_start()
    print("mqtt {}:{} source={} meters={}".format(args.host, args.port, args.source, len(series)))
    total = 0
    for imei, src, samples in series:
        print("  {} <- {} n={}".format(imei, src, len(samples)))
        publish_meter(client, imei, samples, args.sleep, args.heartbeat_every)
        total += len(samples)
    time.sleep(0.5)
    client.loop_stop()
    client.disconnect()
    print("published {} data messages".format(total))


if __name__ == "__main__":
    main()
