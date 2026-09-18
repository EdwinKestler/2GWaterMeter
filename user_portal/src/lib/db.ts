import { Pool } from "pg";

const connectionString =
  process.env.DATABASE_URL ||
  `postgres://${process.env.PGUSER || "waterbox"}:${process.env.PGPASSWORD || "waterbox"}@${
    process.env.PGHOST || "localhost"
  }:${process.env.PGPORT || "5432"}/${process.env.PGDATABASE || "waterbox"}`;

const globalForPg = globalThis as unknown as { pool?: Pool };

export const pool =
  globalForPg.pool ||
  new Pool({
    connectionString,
    max: 8
  });

if (process.env.NODE_ENV !== "production") {
  globalForPg.pool = pool;
}

export async function ensurePortalSchema() {
  await pool.query(`
    CREATE TABLE IF NOT EXISTS portal_users (
      id SERIAL PRIMARY KEY,
      email TEXT UNIQUE NOT NULL,
      password_hash TEXT NOT NULL,
      created_at TIMESTAMPTZ NOT NULL DEFAULT now()
    )
  `);
  await pool.query(`
    CREATE TABLE IF NOT EXISTS portal_meters (
      user_id INTEGER NOT NULL REFERENCES portal_users (id) ON DELETE CASCADE,
      imei TEXT NOT NULL REFERENCES meters (imei) ON DELETE CASCADE,
      label TEXT,
      address TEXT,
      lat DOUBLE PRECISION,
      lon DOUBLE PRECISION,
      geofence_m DOUBLE PRECISION NOT NULL DEFAULT 500,
      PRIMARY KEY (user_id, imei)
    )
  `);
}

export async function ensureMeter(imei: string) {
  await pool.query(
    `INSERT INTO meters (imei) VALUES ($1)
     ON CONFLICT (imei) DO NOTHING`,
    [imei]
  );
}
