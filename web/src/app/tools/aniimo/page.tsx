import type { Metadata } from "next";
import { Fraunces, Figtree } from "next/font/google";
import { AniimoBuildLibrary } from "@/components/AniimoBuildLibrary";
import "../irisalis-party/irisalis.css";
import "./aniimo.css";

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
  title: "Aniimo Build Library — Gear & Upgrades",
  description:
    "Write an Aniimo build the same way as Irisalis: carried item, attribute order, skills, and what to upgrade. The Aniilog is listed so every creature has a sheet.",
  robots: { index: true, follow: true },
};

export default function AniimoLibraryPage() {
  return (
    <div className={`${fraunces.variable} ${figtree.variable} iris-root`}>
      <div className="iris-atmosphere" aria-hidden />
      <div className="iris-petals" aria-hidden />

      <header className="lib-hero">
        <p className="lib-kicker">Aniilog</p>
        <h1>Every Aniimo. Same build sheet.</h1>
        <p>
          Irisalis already has gear and upgrades. Use that layout for the rest of the roster — carried
          item, attributes, and what to raise — and keep the sheets in this browser.
        </p>
        <div className="iris-cta-row">
          <a className="iris-cta" href="#library">
            Open the library
          </a>
          <a className="iris-cta iris-cta-ghost" href="/tools/irisalis-party">
            Irisalis party
          </a>
        </div>
      </header>

      <main id="library" className="iris-main">
        <AniimoBuildLibrary />
      </main>

      <footer className="iris-foot">
        <p>
          The name list follows the public Aniidex. Filled sheets are the Irisalis party builds plus
          anything you save here. Gear you have not written is left blank on purpose.
        </p>
      </footer>
    </div>
  );
}
