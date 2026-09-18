"use client";

import dynamic from "next/dynamic";

export const HeatMapLoader = dynamic(() => import("./HeatMap").then((m) => m.HeatMap), {
  ssr: false,
  loading: () => <p className="muted">Loading map…</p>
});
