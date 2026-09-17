import Image from "next/image";
import Link from "next/link";
import { PreregCounter } from "@/components/PreregCounter";
import { Shell } from "@/components/Shell";
import { features } from "@/data/features";
import { latest } from "@/data/progress";
import { playClasses } from "@/data/classes";
import { site } from "@/data/site";

const highlights = features.filter((f) =>
  ["visuals", "naval", "stalls", "portals", "combat-power", "guild-war"].includes(f.id),
);

const heroPills = [
  {
    title: "Classic PKO",
    blurb: "Navy vs Pirate · ships · trade",
    icon: (
      <svg viewBox="0 0 24 24" className="h-5 w-5" fill="none" stroke="currentColor" strokeWidth="1.6" aria-hidden>
        <path d="M3 16c2.5-1 5-1.5 9-1.5s6.5.5 9 1.5" />
        <path d="M12 4v10.5" />
        <path d="M12 6l5 2.5L12 11" />
        <path d="M4 19h16" />
      </svg>
    ),
  },
  {
    title: "Looks alive",
    blurb: "Bold outlines · rich color · real seas",
    icon: (
      <svg viewBox="0 0 24 24" className="h-5 w-5" fill="none" stroke="currentColor" strokeWidth="1.6" aria-hidden>
        <path d="M7 18l-2 2 3-1 9-9-2-2-9 9z" />
        <path d="M14 8l2 2" />
        <path d="M17 5l2 2" />
      </svg>
    ),
  },
  {
    title: "Built to last",
    blurb: "Smoother logins · fairer fights",
    icon: (
      <svg viewBox="0 0 24 24" className="h-5 w-5" fill="none" stroke="currentColor" strokeWidth="1.6" aria-hidden>
        <circle cx="12" cy="12" r="8" />
        <path d="M12 8v4l3 2" />
        <path d="M12 4v1.5M12 18.5V20M4 12h1.5M18.5 12H20" />
      </svg>
    ),
  },
] as const;

export default function HomePage() {
  return (
    <Shell>
      <section className="relative isolate -mt-[4.25rem] min-h-[100svh] overflow-hidden">
        <Image
          src="/img/banner-v2.png"
          alt="Abyss-Sea crew on deck at sunset, sailing toward Ascaron"
          fill
          priority
          unoptimized
          sizes="100vw"
          className="hero-banner-img object-cover object-[70%_42%] sm:object-[74%_40%]"
        />
        {/* Text-only scrim — keep the crew side bright so rim light reads */}
        <div className="hero-scrim-x absolute inset-0" aria-hidden />
        <div className="hero-scrim-y absolute inset-0" aria-hidden />
        <div className="hero-warm-glow absolute inset-0" aria-hidden />

        <div className="relative z-10 mx-auto flex min-h-[100svh] max-w-[1120px] flex-col justify-between px-4 pb-10 pt-[6.5rem] sm:px-6 sm:pb-12 sm:pt-[7.25rem]">
          <div className="hero-copy max-w-xl">
            <p className="text-[0.7rem] font-semibold uppercase tracking-[0.28em] text-gold">
              Set sail for Ascaron
            </p>
            <h1 className="font-display mt-3 text-[2.45rem] leading-[1.08] tracking-wide text-white drop-shadow-[0_2px_28px_rgba(0,0,0,0.75)] sm:text-[3.35rem]">
              The pirate MMO you remember,{" "}
              <span className="text-gold-2">worth sailing again.</span>
            </h1>
            <p className="mt-5 max-w-lg text-[17px] leading-7 text-foam drop-shadow-[0_1px_12px_rgba(0,0,0,0.65)]">
              {site.description}
            </p>
            <div className="mt-8 flex flex-wrap gap-3">
              <Link href="/prereg" className="btn-crimson" data-track="cta_prereg_home">
                Pre-register
              </Link>
              <Link href="/progress" className="btn-ghost !border-white/25 !bg-black/25 backdrop-blur-sm">
                See what&apos;s new
              </Link>
            </div>
            <div className="mt-4">
              <PreregCounter />
            </div>
          </div>

          <ul className="hero-pills mt-12 grid gap-3 sm:mt-16 sm:grid-cols-3">
            {heroPills.map((pill) => (
              <li
                key={pill.title}
                className="rounded-xl border border-gold/25 bg-[#1a100e]/72 px-5 py-4 shadow-[0_12px_40px_rgba(0,0,0,0.35)] backdrop-blur-md"
              >
                <div className="flex items-start gap-3">
                  <span className="mt-0.5 text-gold">{pill.icon}</span>
                  <div>
                    <p className="text-[0.7rem] font-semibold uppercase tracking-[0.2em] text-gold">
                      {pill.title}
                    </p>
                    <p className="mt-1.5 text-sm text-muted">{pill.blurb}</p>
                  </div>
                </div>
              </li>
            ))}
          </ul>
        </div>
      </section>

      <section className="mx-auto mt-16 max-w-[1120px] px-4 sm:mt-20 sm:px-6">
        <div className="flex items-end justify-between gap-4">
          <div>
            <h2 className="font-display text-[2rem] tracking-wide text-gold-2">What you play</h2>
            <p className="mt-2 max-w-xl text-[17px] leading-7 text-muted">
              The PKO loop you came for — plus the quality-of-life that makes a crew want to stay.
            </p>
          </div>
          <Link href="/features" className="hidden text-sm text-gold sm:inline">
            All features →
          </Link>
        </div>
        <div className="mt-8 grid gap-4 sm:grid-cols-2 lg:grid-cols-3">
          {highlights.map((f) => (
            <article key={f.id} className="card-glow rounded-xl p-5">
              <h3 className="font-display text-lg tracking-wide text-foam">{f.title}</h3>
              <p className="mt-2 text-sm leading-6 text-muted">{f.blurb}</p>
            </article>
          ))}
        </div>
      </section>

      <section className="mx-auto mt-20 max-w-[40rem] px-4 sm:px-6">
        <div className="flex items-end justify-between gap-4">
          <h2 className="font-display text-[2rem] tracking-wide text-gold-2">Latest news</h2>
          <Link href="/progress" className="text-sm text-gold">
            Full log →
          </Link>
        </div>
        <ol className="mt-8 space-y-5">
          {latest.map((item) => (
            <li key={item.title} className="border-l border-gold/30 pl-4">
              <time className="text-xs uppercase tracking-[0.16em] text-gold">{item.date}</time>
              <p className="mt-1 font-semibold text-foam">{item.title}</p>
              <p className="mt-1 text-sm leading-6 text-muted">{item.summary}</p>
            </li>
          ))}
        </ol>
      </section>

      <section className="mx-auto mt-20 max-w-[40rem] px-4 sm:px-6">
        <h2 className="font-display text-[2rem] tracking-wide text-gold-2">Pick a flag. Pick a craft.</h2>
        <p className="mt-2 text-[17px] leading-7 text-muted">
          Newbie at 10, then the tree opens. Advanced classes wait behind quests, not cash shops.
        </p>
        <div className="mt-6 flex flex-wrap gap-2">
          {playClasses
            .filter((c) => c.tier === "base")
            .map((c) => (
              <span
                key={c.name}
                className="rounded-lg border border-line bg-panel/80 px-3 py-1.5 text-sm text-foam shadow-[0_0_16px_rgba(230,184,94,0.08)]"
              >
                {c.name}
              </span>
            ))}
        </div>
        <Link href="/classes" className="mt-5 inline-block text-sm text-gold">
          See the full class tree →
        </Link>
        <p className="mt-2 text-sm text-muted">
          New to PKO?{" "}
          <Link href="/guides/pko-classes" className="text-gold hover:underline">
            Read the class guide
          </Link>
          .
        </p>
      </section>

      <section className="mx-auto mt-20 max-w-[40rem] px-4 pb-8 sm:px-6">
        <div className="card-glow rounded-xl p-8">
          <h2 className="font-display text-[1.7rem] tracking-wide text-gold-2">
            Come sail while we rebuild.
          </h2>
          <p className="mt-3 text-[17px] leading-7 text-muted">
            Argue Crusader vs Champion, hunt with a pickup party, and tell us what still feels
            off.
          </p>
          <Link href="/prereg" className="btn-crimson mt-6" data-track="cta_prereg_home_bottom">
            Pre-register
          </Link>
        </div>
      </section>
    </Shell>
  );
}
