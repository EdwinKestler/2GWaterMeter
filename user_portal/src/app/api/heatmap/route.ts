import { NextResponse } from "next/server";
import { getSession } from "@/lib/auth";
import { haversineM, heatmapPoints, userMeters } from "@/lib/queries";

export async function GET() {
  const session = await getSession();
  if (!session) {
    return NextResponse.json({ error: "auth" }, { status: 401 });
  }
  const mine = await userMeters(session.userId);
  const points = await heatmapPoints();
  const self = mine.find((m) => m.lat != null && m.lon != null);
  let peers: typeof points = [];
  if (self) {
    const radius = Number(self.geofence_m) || 500;
    peers = points.filter(
      (p) => haversineM(Number(self.lat), Number(self.lon), p.lat, p.lon) <= radius
    );
  }
  const peerAvg =
    peers.length > 0 ? peers.reduce((s, p) => s + p.liters, 0) / peers.length : null;
  return NextResponse.json({
    mine,
    points,
    geofence: self
      ? {
          lat: Number(self.lat),
          lon: Number(self.lon),
          radius_m: Number(self.geofence_m) || 500,
          peer_count: peers.length,
          peer_avg_liters: peerAvg
        }
      : null
  });
}
