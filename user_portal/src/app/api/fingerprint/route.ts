import { NextResponse } from "next/server";
import { getSession } from "@/lib/auth";
import { fingerprintCurve, fingerprintsByMonth, ownsMeter } from "@/lib/queries";

export async function GET(req: Request) {
  const session = await getSession();
  if (!session) {
    return NextResponse.json({ error: "auth" }, { status: 401 });
  }
  const imei = new URL(req.url).searchParams.get("imei") || "";
  if (!(await ownsMeter(session.userId, imei))) {
    return NextResponse.json({ error: "forbidden" }, { status: 403 });
  }
  const rows = await fingerprintsByMonth(imei);
  const months = rows.map((row) => ({
    month: row.month,
    n: row.n_samples,
    curve: fingerprintCurve(row)
  }));
  return NextResponse.json({ months });
}
