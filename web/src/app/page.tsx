import Link from "next/link";
import { Shell } from "@/components/Shell";
import { features } from "@/data/features";
import { latest } from "@/data/progress";
import { playClasses } from "@/data/classes";
import { site } from "@/data/site";

const highlights = features.filter((f) =>
  ["visuals", "naval", "stalls", "portals", "combat-power", "guild-war"].includes(f.id),
);

export default function HomePage() {
  return (
    <Shell>
      <section className="mx-auto max-w-[40rem] px-4 pb-16 pt-16 sm:px-6 sm:pt-20">
        <p className="text-[0.7rem] font-semibold uppercase tracking-[0.28em] text-gold">
          Set sail for Ascaron
        </p>
        <h1 className="title-glow font-display mt-3 text-[2.6rem] leading-[1.05] tracking-wide sm:text-[3.4rem]">
          The pirate MMO you remember, worth sailing again.
        </h1>
        <p className="mt-5 text-[17px] leading-7 text-muted">{site.description}</p>
        <div className="mt-8 flex flex-wrap gap-3">
          <Link href="/prereg" className="btn-crimson">
            Pre-register
          </Link>
          <Link href="/progress" className="btn-ghost">
            See what&apos;s new
          </Link>
        </div>
      </section>

      <section className="mx-auto max-w-[1120px] px-4 sm:px-6">
        <div className="grid gap-3 sm:grid-cols-3">
          {[
            ["Classic PKO", "Navy vs Pirate · ships · trade"],
            ["Looks alive", "Bold outlines · rich color · real seas"],
            ["Built to last", "Smoother logins · fairer fights"],
          ].map(([k, v]) => (
            <div key={k} className="card-glow rounded-xl px-5 py-4">
              <dt className="text-[0.7rem] font-semibold uppercase tracking-[0.2em] text-gold">{k}</dt>
              <dd className="mt-2 text-sm text-muted">{v}</dd>
            </div>
          ))}
        </div>
      </section>

      <section className="mx-auto mt-20 max-w-[1120px] px-4 sm:px-6">
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
          <Link href="/prereg" className="btn-crimson mt-6">
            Pre-register
          </Link>
        </div>
      </section>
    </Shell>
  );
}
