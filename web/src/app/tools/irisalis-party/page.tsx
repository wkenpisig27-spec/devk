import type { Metadata } from "next";
import { Fraunces, Figtree } from "next/font/google";
import { IrisalisPartyBuilder } from "@/components/IrisalisPartyBuilder";
import "./irisalis.css";

const fraunces = Fraunces({
  subsets: ["latin"],
  variable: "--font-iris-display",
});

const figtree = Figtree({
  subsets: ["latin"],
  variable: "--font-iris-sans",
  weight: ["400", "500", "600", "700"],
});

export const metadata: Metadata = {
  title: "Irisalis Party Builder — 4 Aniimo Gear & Upgrades",
  description:
    "Build a 4-Aniimo party around Irisalis: recommended partners, carried items, attribute order, and upgrade priorities for the Legendary Grass DPS.",
  openGraph: {
    title: "Irisalis 4-Aniimo Party Builder",
    description:
      "Pick Bloom Core, Guarded Blossom, Tidal Relay, or Meadow Line — with gear and upgrades for Irisalis and every seat.",
  },
  keywords: [
    "Irisalis",
    "Aniimo",
    "party builder",
    "Ferocious Fang",
    "Somniwing",
    "Grass DPS",
  ],
  robots: {
    index: true,
    follow: true,
  },
};

export default function IrisalisPartyPage() {
  return (
    <div className={`${fraunces.variable} ${figtree.variable} iris-root`}>
      <div className="iris-atmosphere" aria-hidden />
      <div className="iris-petals" aria-hidden />

      <header className="iris-hero">
        <p className="iris-brand">Irisalis</p>
        <h1 className="iris-headline">Four seats. One bloom.</h1>
        <p className="iris-lede">
          Lock Irisalis as lead, then fill Break, Heal, and Regen around her — with every carried
          item and upgrade priority spelled out.
        </p>
        <div className="iris-cta-row">
          <a className="iris-cta" href="#builder">
            Open party builder
          </a>
          <a className="iris-cta iris-cta-ghost" href="#upgrades">
            Irisalis upgrades
          </a>
        </div>
      </header>

      <main id="builder" className="iris-main">
        <IrisalisPartyBuilder />
      </main>

      <footer className="iris-foot">
        <p>
          Data drawn from in-game build panels and community Aniilog reads (build 1.0.0.7 era). Patch
          numbers can shift — treat held-item targets and Resonance costs as priorities, not
          absolutes.
        </p>
      </footer>
    </div>
  );
}
