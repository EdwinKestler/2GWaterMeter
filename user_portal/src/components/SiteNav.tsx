import Link from "next/link";
import { ReactNode } from "react";

export function SiteNav({
  current,
  status,
  children
}: {
  current?: "login" | "register" | "app";
  status?: string;
  children?: ReactNode;
}) {
  return (
    <nav className="nav site-nav">
      <Link className="nav-brand" href={current === "app" ? "/app" : "/"}>
        2GWATERMETER
      </Link>
      {current !== "app" ? (
        <div className="nav-links">
          <Link href="/" aria-current={current === "login" ? "page" : undefined}>
            Portal
          </Link>
          <Link href="/register" aria-current={current === "register" ? "page" : undefined}>
            Register
          </Link>
        </div>
      ) : (
        <div className="nav-links" />
      )}
      <span className="nav-status">{status || "lab · online"}</span>
      {children}
    </nav>
  );
}
