"use client";

import { FormEvent, useEffect, useMemo, useState } from "react";
import { useRouter } from "next/navigation";
import { ConsumptionChart } from "@/components/ConsumptionChart";
import { FingerprintChart } from "@/components/FingerprintChart";
import { HeatMapLoader } from "@/components/HeatMapLoader";
import { Corners } from "@/components/Corners";
import { SiteNav } from "@/components/SiteNav";

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
    points: {
      imei: string;
      lat: number;
      lon: number;
      liters: number;
      band: "below" | "normal" | "above";
    }[];
    geofence: {
      lat: number;
      lon: number;
      radius_m: number;
      peer_count: number;
      peer_avg_liters: number | null;
    } | null;
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
    <>
      <SiteNav current="app" status="lab · 1 meter online">
        <select className="input" style={{ width: "auto", minWidth: 180 }} value={imei} onChange={(e) => setImei(e.target.value)}>
          {meters.map((m) => (
            <option key={m.imei} value={m.imei}>
              {m.label || m.imei}
            </option>
          ))}
        </select>
        <button className="btn btn-secondary" onClick={logout} type="button">
          Log out
        </button>
      </SiteNav>

      <div className="wrap">
        <header className="sheet-head">
          <div>
            <div className="kicker">Household portal</div>
            <h1>Your water use</h1>
          </div>
          <dl className="sheet-meta">
            <div>
              <dt>Account</dt>
              <dd>{email || "—"}</dd>
            </div>
            <div>
              <dt>IMEI</dt>
              <dd>{imei || "—"}</dd>
            </div>
          </dl>
        </header>

        <div className="blueprint specs">
          <div>
            <div className="spec-value">{total.toFixed(0)}</div>
            <div className="spec-label">L this period</div>
          </div>
          <div>
            <div className="spec-value">{selected?.valve ? "open" : "closed"}</div>
            <div className="spec-label">Valve</div>
          </div>
          <div>
            <div className="spec-value">{selected?.fp_valid ? "loaded" : "none"}</div>
            <div className="spec-label">Fingerprint</div>
          </div>
          <div>
            <div className="spec-value">{selfPoint ? selfPoint.band : "—"}</div>
            <div className="spec-label">vs city median</div>
          </div>
          <Corners />
        </div>

        <div className="seg" role="tablist">
          <label className="seg-opt">
            <input type="radio" name="tab" checked={tab === "use"} onChange={() => setTab("use")} />
            Consumption
          </label>
          <label className="seg-opt">
            <input type="radio" name="tab" checked={tab === "fp"} onChange={() => setTab("fp")} />
            Monthly fingerprint
          </label>
          <label className="seg-opt">
            <input type="radio" name="tab" checked={tab === "map"} onChange={() => setTab("map")} />
            Map &amp; geofence
          </label>
        </div>

        {tab === "use" ? (
          <div className="blueprint chart-panel" style={{ marginTop: 22 }}>
            <Corners />
            <div className="chart-head">
              <div className="chart-title">Volume</div>
              <div className="seg">
                {(["hourly", "daily", "weekly", "monthly"] as Range[]).map((r) => (
                  <label className="seg-opt" key={r}>
                    <input type="radio" name="range" checked={range === r} onChange={() => setRange(r)} />
                    {r}
                  </label>
                ))}
              </div>
            </div>
            {series.length ? (
              <ConsumptionChart data={series} />
            ) : (
              <p className="muted">No readings yet for this IMEI.</p>
            )}
          </div>
        ) : null}

        {tab === "fp" ? (
          <div className="blueprint chart-panel" style={{ marginTop: 22 }}>
            <Corners />
            <div className="chart-head">
              <div className="chart-title">W(t) overlay</div>
            </div>
            <p className="eq-note">Mean + 24 h sine + 12 h sine · one line per month</p>
            {months.length ? (
              <FingerprintChart months={months} />
            ) : (
              <p className="muted">No fingerprint yet. The cloud worker publishes after enough samples.</p>
            )}
          </div>
        ) : null}

        {tab === "map" ? (
          <>
            <form className="blueprint chart-panel" style={{ marginTop: 22 }} onSubmit={saveAddress}>
              <Corners />
              <div className="chart-title" style={{ marginBottom: 12 }}>
                Service address
              </div>
              <div className="field">
                <label htmlFor="address">Street</label>
                <input
                  className="input"
                  id="address"
                  value={address}
                  onChange={(e) => setAddress(e.target.value)}
                  placeholder="Street, zone, Guatemala City"
                />
              </div>
              <div className="field">
                <label htmlFor="geofence">Geofence radius (meters)</label>
                <input
                  className="input"
                  id="geofence"
                  value={geofence}
                  onChange={(e) => setGeofence(e.target.value)}
                  type="number"
                  min={50}
                  max={20000}
                />
              </div>
              <button className="btn btn-primary" type="submit">
                Geocode &amp; save
              </button>
              {msg ? <p className="muted">{msg}</p> : null}
            </form>
            {heat?.geofence ? (
              <div className="callout">
                <span className="tag tag-accent">geofence</span>
                <p>
                  {heat.geofence.peer_count <= 1
                    ? "Lab mode: only your meter is on the map, so the band is relative to yourself (normal). Neighbors appear when more IMEIs register."
                    : `${heat.geofence.peer_count} meters in your fence${
                        heat.geofence.peer_avg_liters != null
                          ? ` · neighborhood average ${heat.geofence.peer_avg_liters.toFixed(1)} L this month`
                          : ""
                      }`}
                </p>
              </div>
            ) : (
              <p className="muted">Save an address to place your meter on the map.</p>
            )}
            <div className="blueprint chart-panel">
              <Corners />
              <p className="legend">
                <span>
                  <i className="dot-below" /> below average
                </span>
                <span>
                  <i className="dot-normal" /> normal
                </span>
                <span>
                  <i className="dot-above" /> above average
                </span>
              </p>
              <HeatMapLoader
                points={heat?.points || []}
                center={heat?.geofence ? { lat: heat.geofence.lat, lon: heat.geofence.lon } : null}
                radius_m={heat?.geofence?.radius_m || Number(geofence) || 500}
              />
            </div>
          </>
        ) : null}

        <div className="site-footer">
          <span>2GWaterMeter · GPL-3.0</span>
          <span>Industry blueprint · same tokens as the Pages site</span>
        </div>
      </div>
    </>
  );
}
