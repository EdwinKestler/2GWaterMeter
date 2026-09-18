import { NextResponse } from "next/server";
import { pool, ensureMeter, ensurePortalSchema } from "@/lib/db";
import { hashPassword, normalizeImei, setSession } from "@/lib/auth";

export async function POST(req: Request) {
  const body = await req.json();
  const email = String(body.email || "").trim().toLowerCase();
  const password = String(body.password || "");
  const imei = normalizeImei(String(body.imei || ""));
  const address = String(body.address || "").trim();
  if (!email.includes("@") || password.length < 8 || imei.length < 8) {
    return NextResponse.json({ error: "Need email, 8+ char password, and IMEI" }, { status: 400 });
  }
  await ensurePortalSchema();
  await ensureMeter(imei);
  const hash = await hashPassword(password);
  try {
    const user = await pool.query(
      `INSERT INTO portal_users (email, password_hash) VALUES ($1, $2) RETURNING id, email`,
      [email, hash]
    );
    const userId = user.rows[0].id as number;
    await pool.query(
      `INSERT INTO portal_meters (user_id, imei, label, address) VALUES ($1, $2, $3, $4)`,
      [userId, imei, imei, address || null]
    );
    await setSession({ userId, email });
    return NextResponse.json({ ok: true });
  } catch (err: unknown) {
    const msg = err instanceof Error ? err.message : "register failed";
    if (msg.includes("portal_users_email_key") || msg.includes("unique")) {
      return NextResponse.json({ error: "Email already registered" }, { status: 409 });
    }
    return NextResponse.json({ error: msg }, { status: 400 });
  }
}
