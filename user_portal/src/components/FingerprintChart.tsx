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

const COLORS = ["#3ec6c9", "#5b8def", "#e8a838", "#9b7dff", "#5dce8a", "#e05a5a"];

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
        <LineChart data={data}>
          <CartesianGrid stroke="#2a3644" />
          <XAxis dataKey="hour" stroke="#8d9aaa" tick={{ fontSize: 11 }} />
          <YAxis stroke="#8d9aaa" tick={{ fontSize: 11 }} />
          <Tooltip contentStyle={{ background: "#151c25", border: "1px solid #2a3644" }} />
          <Legend />
          {keys.map((key, i) => (
            <Line key={key} type="monotone" dataKey={key} stroke={COLORS[i % COLORS.length]} dot={false} />
          ))}
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}
