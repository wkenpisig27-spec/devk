import type { Metadata, Viewport } from "next";
import { Suspense } from "react";
import { Chakra_Petch, Russo_One } from "next/font/google";
import "./globals.css";
import { Analytics } from "@/components/Analytics";
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
  metadataBase: new URL(site.url),
  title: {
    default: `${site.name} — Sail Ascaron again`,
    template: `%s · ${site.shortName}`,
  },
  description: site.description,
  applicationName: site.name,
  keywords: [
    "Abyss-Sea Online",
    "Pirates King Online",
    "PKO private server",
    "Tales of Pirates",
    "MMORPG",
    "pirate MMO",
  ],
  authors: [{ name: "Abyss-Sea community" }],
  alternates: {
    canonical: "/",
  },
  openGraph: {
    type: "website",
    locale: "en_US",
    url: site.url,
    siteName: site.name,
    title: `${site.name} — Sail Ascaron again`,
    description: site.description,
  },
  twitter: {
    card: "summary_large_image",
    title: `${site.name} — Sail Ascaron again`,
    description: site.description,
  },
  robots: {
    index: true,
    follow: true,
  },
};

const jsonLd = {
  "@context": "https://schema.org",
  "@type": "VideoGame",
  name: site.name,
  alternateName: ["Abyss-Sea", "PKO", "Pirates King Online fan server"],
  description: site.description,
  url: site.url,
  genre: ["MMORPG", "Adventure"],
  gamePlatform: "PC",
  applicationCategory: "GameApplication",
  offers: {
    "@type": "Offer",
    price: "0",
    priceCurrency: "USD",
    availability: "https://schema.org/PreOrder",
  },
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body className={`${chakra.variable} ${russo.variable} font-sans antialiased`}>
        <script
          type="application/ld+json"
          dangerouslySetInnerHTML={{ __html: JSON.stringify(jsonLd) }}
        />
        {children}
        <Suspense fallback={null}>
          <Analytics />
        </Suspense>
      </body>
    </html>
  );
}
