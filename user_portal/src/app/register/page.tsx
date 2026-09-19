"use client";

import { FormEvent, useState } from "react";
import { useRouter } from "next/navigation";
import Link from "next/link";
import { Corners } from "@/components/Corners";
import { SiteNav } from "@/components/SiteNav";

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
    <>
      <SiteNav current="register" />
      <div className="wrap auth">
        <div className="kicker">New household</div>
        <h1>Register a meter</h1>
        <p className="lead">IMEI is the 15-digit id printed on the serial console at boot.</p>
        <form className="blueprint auth-card" onSubmit={onSubmit}>
          <Corners />
          <div className="field">
            <label htmlFor="email">Email</label>
            <input className="input" id="email" name="email" type="email" required />
          </div>
          <div className="field">
            <label htmlFor="password">Password (8+ characters)</label>
            <input className="input" id="password" name="password" type="password" minLength={8} required />
          </div>
          <div className="field">
            <label htmlFor="imei">Meter IMEI</label>
            <input className="input" id="imei" name="imei" required placeholder="15-digit IMEI" />
          </div>
          <div className="field">
            <label htmlFor="address">Service address (optional)</label>
            <input className="input" id="address" name="address" placeholder="Street, Guatemala City" />
          </div>
          {error ? <p className="err">{error}</p> : null}
          <button className="btn btn-primary blueprint" type="submit">
            Create account
            <Corners />
          </button>
        </form>
        <p className="muted">
          Already registered? <Link href="/">Log in</Link>
        </p>
      </div>
    </>
  );
}
