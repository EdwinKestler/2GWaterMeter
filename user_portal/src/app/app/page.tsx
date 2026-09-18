"use client";

import { FormEvent, useEffect, useMemo, useState } from "react";
import { useRouter } from "next/navigation";
import { ConsumptionChart } from "@/components/ConsumptionChart";
import { FingerprintChart } from "@/components/FingerprintChart";
import { HeatMapLoader } from "@/components/HeatMapLoader";

type Meter = {
  imei: string;
  label: string | null;
  address: string | null;
  lat: number | null;
  lon: number | null;
  geofence_m: number;
  last_seen: string | null;
  fp_valid: boolean;
  valve: boolean | null;
};

type Range = "hourly" | "daily" | "weekly" | "monthly";

export default function AppPage() {
  const router = useRouter();
  const [tab, setTab] = useState<"use" | "fp" | "map">("use");
  const [range, setRange] = useState<Range>("daily");
  const [email, setEmail] = useState("");
  const [meters, setMeters] = useState<Meter[]>([]);
  const [imei, setImei] = useState("");
  const [series, setSeries] = useState<{ t: string; liters: number }[]>([]);
  const [months, setMonths] = useState<{ month: string; curve: { hour: number; lm: number }[] }[]>(
    []
  );
  const [heat, setHeat] = useState<{
    points: { imei: string; lat: number; lon: number; liters: number; band: "below" | "normal" | "above" }[];
    geofence: { lat: number; lon: number; radius_m: number; peer_count: number; peer_avg_liters: number | null } | null;
  } | null>(null);
  const [address, setAddress] = useState("");
  const [geofence, setGeofence] = useState("500");
  const [msg, setMsg] = useState("");

  const selected = useMemo(() => meters.find((m) => m.imei === imei), [meters, imei]);

  async function loadMeters() {
    const res = await fetch("/api/meters");
    if (res.status === 401) {
      router.push("/");
      return;
    }
    const data = await res.json();
    setEmail(data.email);
    setMeters(data.meters || []);
    if (!imei && data.meters?.[0]) {
      setImei(data.meters[0].imei);
      setAddress(data.meters[0].address || "");
      setGeofence(String(data.meters[0].geofence_m || 500));
    }
  }

  useEffect(() => {
    loadMeters();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  useEffect(() => {
    if (!imei) return;
    const m = meters.find((x) => x.imei === imei);
    if (m) {
      setAddress(m.address || "");
      setGeofence(String(m.geofence_m || 500));
    }
    fetch(`/api/consumption?imei=${encodeURIComponent(imei)}&range=${range}`)
      .then((r) => r.json())
      .then((d) => setSeries(d.series || []));
    fetch(`/api/fingerprint?imei=${encodeURIComponent(imei)}`)
      .then((r) => r.json())
      .then((d) => setMonths(d.months || []));
    fetch("/api/heatmap")
      .then((r) => r.json())
      .then(setHeat);
  }, [imei, range, meters]);

  async function logout() {
    await fetch("/api/logout", { method: "POST" });
    router.push("/");
    router.refresh();
  }

  async function saveAddress(e: FormEvent) {
    e.preventDefault();
    setMsg("");
    const res = await fetch("/api/address", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ imei, address, geofence_m: Number(geofence) })
    });
    const data = await res.json();
    if (!res.ok) {
      setMsg(data.error || "Save failed");
      return;
    }
    setMsg(data.lat ? `Mapped to ${data.lat.toFixed(5)}, ${data.lon.toFixed(5)}` : "Saved (no geocode hit)");
    loadMeters();
  }

  const total = series.reduce((s, p) => s + p.liters, 0);
  const selfPoint = heat?.points.find((p) => p.imei === imei);

  return (
    <div className="wrap">
      <div className="top">
        <div>
          <div className="kicker">Household portal</div>
          <h1>Your water use</h1>
          <p className="muted">{email}</p>
        </div>
        <div className="nav">
          <select value={imei} onChange={(e) => setImei(e.target.value)}>
            {meters.map((m) => (
              <option key={m.imei} value={m.imei}>
                {m.label || m.imei}
              </option>
            ))}
          </select>
          <button className="ghost" onClick={logout} type="button">
            Log out
          </button>
        </div>
      </div>

      <div className="row">
        <div className="card stat">
          <span className="muted">Period volume</span>
          <b>{total.toFixed(1)} L</b>
        </div>
        <div className="card stat">
          <span className="muted">Valve</span>
          <b>{selected?.valve ? "open" : "closed"}</b>
        </div>
        <div className="card stat">
          <span className="muted">Fingerprint</span>
          <b>{selected?.fp_valid ? "loaded" : "none"}</b>
        </div>
        <div className="card stat">
          <span className="muted">This month vs city</span>
          <b>{selfPoint ? selfPoint.band : "—"}</b>
        </div>
      </div>

      <div className="tabs">
        <button className={tab === "use" ? "on" : ""} type="button" onClick={() => setTab("use")}>
          Consumption
        </button>
        <button className={tab === "fp" ? "on" : ""} type="button" onClick={() => setTab("fp")}>
          Monthly fingerprint
        </button>
        <button className={tab === "map" ? "on" : ""} type="button" onClick={() => setTab("map")}>
          Map & geofence
        </button>
      </div>

      {tab === "use" ? (
        <div className="card">
          <div className="tabs">
            {(["hourly", "daily", "weekly", "monthly"] as Range[]).map((r) => (
              <button key={r} className={range === r ? "on" : ""} type="button" onClick={() => setRange(r)}>
                {r}
              </button>
            ))}
          </div>
          {series.length ? <ConsumptionChart data={series} /> : <p className="muted">No readings yet for this IMEI.</p>}
        </div>
      ) : null}

      {tab === "fp" ? (
        <div className="card">
          <p className="muted">
            W(t) = mean + daily sine + 12-hour sine. Each line is the latest fingerprint in that month.
          </p>
          {months.length ? <FingerprintChart months={months} /> : <p className="muted">No fingerprint yet. The cloud worker publishes after enough samples.</p>}
        </div>
      ) : null}

      {tab === "map" ? (
        <>
          <form className="card" onSubmit={saveAddress}>
            <label>Service address</label>
            <input value={address} onChange={(e) => setAddress(e.target.value)} placeholder="Street, zone, Guatemala City" />
            <label>Geofence radius (meters)</label>
            <input value={geofence} onChange={(e) => setGeofence(e.target.value)} type="number" min={50} max={20000} />
            <div style={{ marginTop: 12 }}>
              <button type="submit">Geocode & save</button>
            </div>
            {msg ? <p className="muted">{msg}</p> : null}
            {heat?.geofence ? (
              <p className="muted">
                {heat.geofence.peer_count <= 1
                  ? "Lab mode: only your meter is on the map, so the band is relative to yourself (normal). Neighbors appear when more IMEIs register."
                  : `${heat.geofence.peer_count} meters in your fence${
                      heat.geofence.peer_avg_liters != null
                        ? ` · neighborhood average ${heat.geofence.peer_avg_liters.toFixed(1)} L this month`
                        : ""
                    }`}
              </p>
            ) : (
              <p className="muted">Save an address to place your meter on the map.</p>
            )}
          </form>
          <div className="card">
            <p className="legend muted">
              <span className="dot-below" /> below average &nbsp;
              <span className="dot-normal" /> normal &nbsp;
              <span className="dot-above" /> above average
            </p>
            <HeatMapLoader
              points={heat?.points || []}
              center={heat?.geofence ? { lat: heat.geofence.lat, lon: heat.geofence.lon } : null}
              radius_m={heat?.geofence?.radius_m || Number(geofence) || 500}
            />
          </div>
        </>
      ) : null}
    </div>
  );
}
