"use client";

import { MapContainer, TileLayer, Circle, CircleMarker, Popup, useMap } from "react-leaflet";
import { useEffect } from "react";
import "leaflet/dist/leaflet.css";

type Point = {
  imei: string;
  lat: number;
  lon: number;
  liters: number;
  band: "below" | "normal" | "above";
  address?: string | null;
};

const COLORS = { below: "#5b8def", normal: "#5dce8a", above: "#e05a5a" };

function Recenter({ lat, lon }: { lat: number; lon: number }) {
  const map = useMap();
  useEffect(() => {
    map.setView([lat, lon], 14);
  }, [map, lat, lon]);
  return null;
}

export function HeatMap({
  points,
  center,
  radius_m
}: {
  points: Point[];
  center: { lat: number; lon: number } | null;
  radius_m: number;
}) {
  const lat = center?.lat ?? Number(process.env.NEXT_PUBLIC_MAP_CENTER_LAT || 14.6349);
  const lon = center?.lon ?? Number(process.env.NEXT_PUBLIC_MAP_CENTER_LON || -90.5069);
  return (
    <MapContainer center={[lat, lon]} zoom={13} scrollWheelZoom className="leaflet-container">
      <TileLayer attribution="&copy; OpenStreetMap" url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png" />
      {center ? <Recenter lat={center.lat} lon={center.lon} /> : null}
      {center ? (
        <Circle
          center={[center.lat, center.lon]}
          radius={radius_m}
          pathOptions={{ color: "#3ec6c9", fillOpacity: 0.08 }}
        />
      ) : null}
      {points.map((p) => (
        <CircleMarker
          key={p.imei}
          center={[p.lat, p.lon]}
          radius={10}
          pathOptions={{ color: COLORS[p.band], fillColor: COLORS[p.band], fillOpacity: 0.85 }}
        >
          <Popup>
            {p.imei}
            <br />
            {p.liters.toFixed(1)} L this month · {p.band}
            {p.address ? (
              <>
                <br />
                {p.address}
              </>
            ) : null}
          </Popup>
        </CircleMarker>
      ))}
    </MapContainer>
  );
}
