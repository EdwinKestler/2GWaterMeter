"use client";

import { FormEvent, useState } from "react";
import { useRouter } from "next/navigation";
import Link from "next/link";

export default function RegisterPage() {
  const router = useRouter();
  const [error, setError] = useState("");

  async function onSubmit(e: FormEvent<HTMLFormElement>) {
    e.preventDefault();
    setError("");
    const form = new FormData(e.currentTarget);
    const res = await fetch("/api/register", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        email: form.get("email"),
        password: form.get("password"),
        imei: form.get("imei"),
        address: form.get("address")
      })
    });
    const data = await res.json();
    if (!res.ok) {
      setError(data.error || "Register failed");
      return;
    }
    router.push("/app");
    router.refresh();
  }

  return (
    <div className="auth">
      <div className="kicker">2GWaterMeter</div>
      <h1>Register a meter</h1>
      <p className="muted">IMEI is the device id printed by the firmware at boot.</p>
      <form className="card" onSubmit={onSubmit}>
        <label>Email</label>
        <input name="email" type="email" required />
        <label>Password (8+ characters)</label>
        <input name="password" type="password" minLength={8} required />
        <label>Meter IMEI</label>
        <input name="imei" required placeholder="15-digit IMEI" />
        <label>Service address (optional, used for the map)</label>
        <input name="address" placeholder="Street, Guatemala City" />
        {error ? <p className="err">{error}</p> : null}
        <div style={{ marginTop: 16 }}>
          <button type="submit">Create account</button>
        </div>
      </form>
      <p className="muted">
        Already registered? <Link href="/">Log in</Link>
      </p>
    </div>
  );
}
