"use client";

import { FormEvent, useState } from "react";
import { useRouter } from "next/navigation";
import Link from "next/link";
import { Corners } from "@/components/Corners";
import { SiteNav } from "@/components/SiteNav";

export default function LoginPage() {
  const router = useRouter();
  const [error, setError] = useState("");

  async function onSubmit(e: FormEvent<HTMLFormElement>) {
    e.preventDefault();
    setError("");
    const form = new FormData(e.currentTarget);
    const res = await fetch("/api/login", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        identifier: form.get("identifier"),
        password: form.get("password")
      })
    });
    const data = await res.json();
    if (!res.ok) {
      setError(data.error || "Login failed");
      return;
    }
    router.push("/app");
    router.refresh();
  }

  return (
    <>
      <SiteNav current="login" />
      <div className="wrap auth">
        <div className="kicker">Household portal</div>
        <h1>Sign in with email or IMEI</h1>
        <p className="lead">Same stack as the technical site: one Postgres, per-meter litres, fingerprint overlay, OSM map.</p>
        <form className="blueprint auth-card" onSubmit={onSubmit}>
          <Corners />
          <div className="field">
            <label htmlFor="identifier">Email or IMEI</label>
            <input className="input" id="identifier" name="identifier" required autoComplete="username" />
          </div>
          <div className="field">
            <label htmlFor="password">Password</label>
            <input
              className="input"
              id="password"
              name="password"
              type="password"
              required
              autoComplete="current-password"
            />
          </div>
          {error ? <p className="err">{error}</p> : null}
          <button className="btn btn-primary blueprint" type="submit">
            Log in
            <Corners />
          </button>
        </form>
        <p className="muted">
          New meter? <Link href="/register">Register with IMEI</Link>
        </p>
        <div className="site-footer">
          <span>2GWaterMeter · GPL-3.0</span>
          <span>lab · 1 household</span>
        </div>
      </div>
    </>
  );
}
