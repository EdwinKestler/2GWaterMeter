import { NextResponse } from "next/server";
import { getSession } from "@/lib/auth";
import { userMeters } from "@/lib/queries";

export async function GET() {
  const session = await getSession();
  if (!session) {
    return NextResponse.json({ error: "auth" }, { status: 401 });
  }
  const meters = await userMeters(session.userId);
  return NextResponse.json({ email: session.email, meters });
}
