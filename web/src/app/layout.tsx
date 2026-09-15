import type { Metadata, Viewport } from "next";
import { Chakra_Petch, Russo_One } from "next/font/google";
import "./globals.css";
import { site } from "@/data/site";

export const viewport: Viewport = {
  themeColor: "#120c0c",
};

const chakra = Chakra_Petch({
  subsets: ["latin"],
  variable: "--font-chakra",
  weight: ["400", "500", "600", "700"],
});

const russo = Russo_One({
  subsets: ["latin"],
  variable: "--font-russo",
  weight: "400",
});

export const metadata: Metadata = {
  title: {
    default: `${site.name} — Sail Ascaron again`,
    template: `%s · ${site.shortName}`,
  },
  description: site.description,
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body className={`${chakra.variable} ${russo.variable} font-sans antialiased`}>
        {children}
      </body>
    </html>
  );
}
