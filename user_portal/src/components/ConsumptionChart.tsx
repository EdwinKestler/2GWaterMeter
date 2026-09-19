"use client";

import {
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
  CartesianGrid
} from "recharts";
import { chartTheme, tooltipStyle } from "@/lib/chartTheme";

export function ConsumptionChart({
  data
}: {
  data: { t: string; liters: number }[];
}) {
  const rows = data.map((d) => ({
    ...d,
    label: new Date(d.t).toLocaleString("es-GT", { month: "short", day: "numeric", hour: "2-digit" })
  }));
  return (
    <div className="chart">
      <ResponsiveContainer>
        <LineChart data={rows} margin={{ top: 8, right: 8, left: 0, bottom: 0 }}>
          <CartesianGrid stroke={chartTheme.grid} strokeDasharray="0" vertical={false} />
          <XAxis
            dataKey="label"
            stroke={chartTheme.tick}
            tick={{ fontSize: 11, fontFamily: "Barlow, system-ui", fill: chartTheme.tick }}
            axisLine={{ stroke: chartTheme.grid }}
            tickLine={false}
          />
          <YAxis
            stroke={chartTheme.tick}
            tick={{ fontSize: 11, fontFamily: "Barlow, system-ui", fill: chartTheme.tick }}
            axisLine={false}
            tickLine={false}
          />
          <Tooltip contentStyle={tooltipStyle} formatter={(v: number) => [`${v.toFixed(2)} L`, "volume"]} />
          <Line type="monotone" dataKey="liters" stroke={chartTheme.accent} dot={false} strokeWidth={1.75} />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}
