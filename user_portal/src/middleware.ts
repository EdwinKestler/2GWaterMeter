import { NextResponse } from "next/server";
import type { NextRequest } from "next/server";

export function middleware(req: NextRequest) {
  const session = req.cookies.get("waterbox_session")?.value;
  const path = req.nextUrl.pathname;
  const protectedPath = path.startsWith("/app");
  if (protectedPath && !session) {
    return NextResponse.redirect(new URL("/", req.url));
  }
  if ((path === "/" || path === "/register") && session) {
    return NextResponse.redirect(new URL("/app", req.url));
  }
  return NextResponse.next();
}

export const config = { matcher: ["/", "/register", "/app/:path*"] };
