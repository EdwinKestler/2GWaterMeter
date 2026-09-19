import "@/styles/industry.css";
import "./globals.css";
import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "2GWaterMeter portal",
  description: "Household water consumption, fingerprint, and map"
};

export default function RootLayout({ children }: { children: React.ReactNode }) {
  return (
    <html lang="en">
      <body>{children}</body>
    </html>
  );
}
