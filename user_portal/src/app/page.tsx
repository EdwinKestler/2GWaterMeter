"use client";

import { FormEvent, useState } from "react";
import { useRouter } from "next/navigation";
import Link from "next/link";

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
    <div className="auth">
      <div className="kicker">2GWaterMeter</div>
      <h1>Household portal</h1>
      <p className="muted">Sign in with email or meter IMEI.</p>
      <form className="card" onSubmit={onSubmit}>
        <label>Email or IMEI</label>
        <input name="identifier" required autoComplete="username" />
        <label>Password</label>
        <input name="password" type="password" required autoComplete="current-password" />
        {error ? <p className="err">{error}</p> : null}
        <div style={{ marginTop: 16 }}>
          <button type="submit">Log in</button>
        </div>
      </form>
      <p className="muted">
        New meter? <Link href="/register">Register with IMEI</Link>
      </p>
    </div>
  );
}
