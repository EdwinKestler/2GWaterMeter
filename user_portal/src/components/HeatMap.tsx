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

const COLORS = { below: "#94bce3", normal: "#5980a6", above: "#1d2d3d" };

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
      <TileLayer
        attribution='&copy; OpenStreetMap &copy; CARTO'
        url="https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}{r}.png"
      />
      {center ? <Recenter lat={center.lat} lon={center.lon} /> : null}
      {center ? (
        <Circle
          center={[center.lat, center.lon]}
          radius={radius_m}
          pathOptions={{ color: "#5980a6", fillColor: "#5980a6", fillOpacity: 0.08, weight: 1 }}
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
