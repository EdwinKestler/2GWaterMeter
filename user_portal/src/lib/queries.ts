import { pool } from "./db";

export type RangeKey = "hourly" | "daily" | "weekly" | "monthly";

const RANGE: Record<RangeKey, { trunc: string; interval: string }> = {
  hourly: { trunc: "hour", interval: "48 hours" },
  daily: { trunc: "day", interval: "30 days" },
  weekly: { trunc: "week", interval: "12 weeks" },
  monthly: { trunc: "month", interval: "12 months" }
};

export async function userMeters(userId: number) {
  const { rows } = await pool.query(
    `SELECT pm.imei, pm.label, pm.address, pm.lat, pm.lon, pm.geofence_m,
            m.last_seen, m.fp_valid, m.valve
     FROM portal_meters pm
     JOIN meters m ON m.imei = pm.imei
     WHERE pm.user_id = $1
     ORDER BY pm.imei`,
    [userId]
  );
  return rows;
}

export async function ownsMeter(userId: number, imei: string) {
  const { rowCount } = await pool.query(
    `SELECT 1 FROM portal_meters WHERE user_id = $1 AND imei = $2`,
    [userId, imei]
  );
  return (rowCount || 0) > 0;
}

export async function consumption(imei: string, range: RangeKey) {
  const spec = RANGE[range];
  const { rows } = await pool.query(
    `SELECT date_trunc($1, ts) AS bucket,
            COALESCE(NULLIF(MAX(tl_ml) - MIN(tl_ml), 0), SUM(mls), 0)::bigint AS ml
     FROM readings
     WHERE imei = $2 AND ts >= now() - $3::interval
     GROUP BY 1
     ORDER BY 1`,
    [spec.trunc, imei, spec.interval]
  );
  return rows.map((r) => ({
    t: r.bucket,
    liters: Number(r.ml) / 1000
  }));
}

export async function fingerprintsByMonth(imei: string) {
  const { rows } = await pool.query(
    `SELECT DISTINCT ON (date_trunc('month', created_at))
            date_trunc('month', created_at) AS month,
            w, a1, p1, t1, a2, p2, t2, n_samples, created_at
     FROM fingerprints
     WHERE imei = $1
     ORDER BY date_trunc('month', created_at) DESC, created_at DESC
     LIMIT 6`,
    [imei]
  );
  return rows;
}

export function fingerprintCurve(row: {
  w: number;
  a1: number;
  p1: number;
  t1: number;
  a2: number;
  p2: number;
  t2: number;
}) {
  const points = [];
  for (let h = 0; h <= 24; h += 0.5) {
    const t1 = Number(row.t1) || 24;
    const t2 = Number(row.t2) || 12;
    let w = Number(row.w);
    w += Number(row.a1) * Math.sin((2 * Math.PI * h) / t1 + Number(row.p1));
    w += Number(row.a2) * Math.sin((2 * Math.PI * h) / t2 + Number(row.p2));
    points.push({ hour: h, lm: Math.max(0, w) });
  }
  return points;
}

export async function heatmapPoints() {
  const { rows } = await pool.query(
    `WITH vol AS (
       SELECT pm.imei, pm.lat, pm.lon, pm.address, pm.geofence_m,
              COALESCE(NULLIF(MAX(r.tl_ml) - MIN(r.tl_ml), 0), SUM(r.mls), 0)::double precision AS ml
       FROM portal_meters pm
       LEFT JOIN readings r
         ON r.imei = pm.imei AND r.ts >= date_trunc('month', now())
       WHERE pm.lat IS NOT NULL AND pm.lon IS NOT NULL
       GROUP BY pm.imei, pm.lat, pm.lon, pm.address, pm.geofence_m
     )
     SELECT v.*,
            (SELECT percentile_cont(0.5) WITHIN GROUP (ORDER BY ml) FROM vol) AS median_ml
     FROM vol v`
  );
  return rows.map((r) => {
    const ml = Number(r.ml) || 0;
    const median = Number(r.median_ml) || 0;
    const ratio = median > 0 ? ml / median : 1;
    let band: "below" | "normal" | "above" = "normal";
    if (ratio < 0.8) band = "below";
    if (ratio > 1.2) band = "above";
    return {
      imei: r.imei as string,
      lat: Number(r.lat),
      lon: Number(r.lon),
      address: r.address as string | null,
      geofence_m: Number(r.geofence_m) || 500,
      liters: ml / 1000,
      medianLiters: median / 1000,
      ratio,
      band
    };
  });
}

export function haversineM(lat1: number, lon1: number, lat2: number, lon2: number) {
  const R = 6371000;
  const toRad = (d: number) => (d * Math.PI) / 180;
  const dLat = toRad(lat2 - lat1);
  const dLon = toRad(lon2 - lon1);
  const a =
    Math.sin(dLat / 2) ** 2 +
    Math.cos(toRad(lat1)) * Math.cos(toRad(lat2)) * Math.sin(dLon / 2) ** 2;
  return 2 * R * Math.asin(Math.sqrt(a));
}
