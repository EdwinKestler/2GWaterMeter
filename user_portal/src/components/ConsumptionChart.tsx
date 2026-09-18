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
        <LineChart data={rows}>
          <CartesianGrid stroke="#2a3644" />
          <XAxis dataKey="label" stroke="#8d9aaa" tick={{ fontSize: 11 }} />
          <YAxis stroke="#8d9aaa" tick={{ fontSize: 11 }} />
          <Tooltip
            contentStyle={{ background: "#151c25", border: "1px solid #2a3644" }}
            formatter={(v: number) => [`${v.toFixed(2)} L`, "volume"]}
          />
          <Line type="monotone" dataKey="liters" stroke="#3ec6c9" dot={false} strokeWidth={2} />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}
