import os


def _env(name, default=""):
    value = os.environ.get(name, default)
    return value if value is not None else default


MQTT_HOST = _env("MQTT_HOST", "rabbitmq")
MQTT_PORT = int(_env("MQTT_PORT", "1883"))
MQTT_USER = _env("MQTT_USER", "waterbox")
MQTT_PASS = _env("MQTT_PASS", "waterbox")
MQTT_CLIENT_ID = _env("MQTT_CLIENT_ID", "waterbox-cloud")

PGHOST = _env("PGHOST", "postgres")
PGPORT = int(_env("PGPORT", "5432"))
PGDATABASE = _env("PGDATABASE", "waterbox")
PGUSER = _env("PGUSER", "waterbox")
PGPASSWORD = _env("PGPASSWORD", "waterbox")

TIMEZONE = _env("WATERBOX_TZ", "America/Guatemala")
FIT_EVERY_SECONDS = int(_env("FIT_EVERY_SECONDS", "3600"))
FIT_LOOKBACK_DAYS = int(_env("FIT_LOOKBACK_DAYS", "14"))
FIT_MIN_SAMPLES = int(_env("FIT_MIN_SAMPLES", "24"))
TOPIC_ROOT = _env("TOPIC_ROOT", "waterbox")
