"use client";

import {
  CartesianGrid,
  Legend,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis
} from "recharts";
import { chartTheme, tooltipStyle } from "@/lib/chartTheme";

type Month = { month: string; curve: { hour: number; lm: number }[] };

export function FingerprintChart({ months }: { months: Month[] }) {
  const hours = Array.from({ length: 49 }, (_, i) => i * 0.5);
  const data = hours.map((hour) => {
    const row: Record<string, number> = { hour };
    months.forEach((m) => {
      const key = new Date(m.month).toLocaleString("es-GT", { month: "short", year: "2-digit" });
      const pt = m.curve.find((c) => c.hour === hour);
      row[key] = pt ? pt.lm : 0;
    });
    return row;
  });
  const keys = months.map((m) =>
    new Date(m.month).toLocaleString("es-GT", { month: "short", year: "2-digit" })
  );
  return (
    <div className="chart">
      <ResponsiveContainer>
        <LineChart data={data} margin={{ top: 8, right: 8, left: 0, bottom: 0 }}>
          <CartesianGrid stroke={chartTheme.grid} vertical={false} />
          <XAxis
            dataKey="hour"
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
          <Tooltip contentStyle={tooltipStyle} />
          <Legend
            wrapperStyle={{ fontFamily: "Barlow Condensed, system-ui", fontSize: 12, letterSpacing: "0.06em" }}
          />
          {keys.map((key, i) => (
            <Line
              key={key}
              type="monotone"
              dataKey={key}
              stroke={chartTheme.series[i % chartTheme.series.length]}
              dot={false}
              strokeWidth={1.6}
            />
          ))}
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}
