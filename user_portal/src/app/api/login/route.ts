import { NextResponse } from "next/server";
import { pool, ensurePortalSchema } from "@/lib/db";
import { looksLikeEmail, normalizeImei, setSession, verifyPassword } from "@/lib/auth";

export async function POST(req: Request) {
  const body = await req.json();
  const identifier = String(body.identifier || "").trim();
  const password = String(body.password || "");
  if (!identifier || !password) {
    return NextResponse.json({ error: "Missing credentials" }, { status: 400 });
  }
  await ensurePortalSchema();
  let row;
  if (looksLikeEmail(identifier)) {
    row = await pool.query(
      `SELECT id, email, password_hash FROM portal_users WHERE email = $1`,
      [identifier.toLowerCase()]
    );
  } else {
    row = await pool.query(
      `SELECT u.id, u.email, u.password_hash
       FROM portal_users u
       JOIN portal_meters pm ON pm.user_id = u.id
       WHERE pm.imei = $1`,
      [normalizeImei(identifier)]
    );
  }
  if (!row.rows[0]) {
    return NextResponse.json({ error: "Unknown user or IMEI" }, { status: 401 });
  }
  const user = row.rows[0];
  const ok = await verifyPassword(password, user.password_hash);
  if (!ok) {
    return NextResponse.json({ error: "Bad password" }, { status: 401 });
  }
  await setSession({ userId: user.id, email: user.email });
  return NextResponse.json({ ok: true });
}
