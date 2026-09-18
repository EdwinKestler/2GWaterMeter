import { NextResponse } from "next/server";
import { getSession } from "@/lib/auth";
import { consumption, ownsMeter, type RangeKey } from "@/lib/queries";

export async function GET(req: Request) {
  const session = await getSession();
  if (!session) {
    return NextResponse.json({ error: "auth" }, { status: 401 });
  }
  const url = new URL(req.url);
  const imei = url.searchParams.get("imei") || "";
  const range = (url.searchParams.get("range") || "daily") as RangeKey;
  if (!["hourly", "daily", "weekly", "monthly"].includes(range)) {
    return NextResponse.json({ error: "bad range" }, { status: 400 });
  }
  if (!(await ownsMeter(session.userId, imei))) {
    return NextResponse.json({ error: "forbidden" }, { status: 403 });
  }
  const series = await consumption(imei, range);
  return NextResponse.json({ series });
}
