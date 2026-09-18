import { NextResponse } from "next/server";
import { getSession } from "@/lib/auth";
import { pool } from "@/lib/db";
import { ownsMeter } from "@/lib/queries";

async function geocode(address: string) {
  const url = `https://nominatim.openstreetmap.org/search?format=json&limit=1&q=${encodeURIComponent(address)}`;
  const res = await fetch(url, {
    headers: { "User-Agent": "2GWaterMeter-user-portal/1.0" },
    cache: "no-store"
  });
  if (!res.ok) {
    return null;
  }
  const data = (await res.json()) as { lat: string; lon: string }[];
  if (!data[0]) {
    return null;
  }
  return { lat: Number(data[0].lat), lon: Number(data[0].lon) };
}

export async function POST(req: Request) {
  const session = await getSession();
  if (!session) {
    return NextResponse.json({ error: "auth" }, { status: 401 });
  }
  const body = await req.json();
  const imei = String(body.imei || "");
  const address = String(body.address || "").trim();
  const geofence_m = Number(body.geofence_m || 500);
  if (!(await ownsMeter(session.userId, imei))) {
    return NextResponse.json({ error: "forbidden" }, { status: 403 });
  }
  let lat = body.lat != null ? Number(body.lat) : null;
  let lon = body.lon != null ? Number(body.lon) : null;
  if (address && (lat == null || lon == null || Number.isNaN(lat))) {
    const geo = await geocode(address);
    if (geo) {
      lat = geo.lat;
      lon = geo.lon;
    }
  }
  await pool.query(
    `UPDATE portal_meters
     SET address = $3, lat = $4, lon = $5, geofence_m = $6
     WHERE user_id = $1 AND imei = $2`,
    [session.userId, imei, address || null, lat, lon, geofence_m]
  );
  return NextResponse.json({ ok: true, lat, lon, geofence_m });
}
