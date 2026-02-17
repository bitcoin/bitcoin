import type { Metadata } from "next";
import "./globals.css";
import RootProvider from "./rootProvider";

export const metadata: Metadata = {
  title: "Onchain Agent",
  description: "AI-powered blockchain agent built with OnchainKit",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body>
        <RootProvider>{children}</RootProvider>
      </body>
    </html>
  );
}
