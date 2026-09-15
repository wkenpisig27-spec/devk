import type { Metadata } from "next";
import Link from "next/link";
import { Shell } from "@/components/Shell";
import { firstJobs, playClasses, secondJobsOf } from "@/data/classes";
import { site } from "@/data/site";

export const metadata: Metadata = {
  title: "PKO Class Guide — Swordsman, Hunter, Herbalist, Explorer",
  description:
    "Pirates King Online class guide for Abyss-Sea Online: first jobs, second job advancements, weapons, and how to pick Crusader, Champion, Sharpshooter, Cleric, Seal Master, or Voyager.",
  openGraph: {
    title: "PKO Class Guide · Abyss-Sea Online",
    description: "First jobs, second advancements, and how to choose your craft.",
  },
  keywords: [
    "Pirates King Online classes",
    "PKO class guide",
    "Abyss-Sea Online",
    "Swordsman Crusader Champion",
    "Hunter Sharpshooter",
    "Herbalist Cleric Seal Master",
    "Explorer Voyager",
  ],
};

export default function PkoClassGuidePage() {
  return (
    <Shell>
      <article className="mx-auto max-w-[40rem] px-4 py-16 sm:px-6">
        <p className="text-[0.7rem] font-semibold uppercase tracking-[0.28em] text-gold">
          Knowledge · Classes
        </p>
        <h1 className="title-glow font-display mt-3 text-[2.4rem] leading-none tracking-wide sm:text-[3.1rem]">
          PKO class guide
        </h1>
        <p className="mt-5 text-[17px] leading-7 text-muted">
          {site.shortName} uses the classic Pirates King Online tree: four first jobs, then an NPC
          class quest into a second job. No cash-shop class unlocks. This guide is for players
          comparing Swordsman, Hunter, Herbalist, and Explorer before launch.
        </p>

        <nav className="card-glow mt-8 rounded-xl p-4 text-sm text-muted">
          <p className="text-[0.7rem] font-semibold uppercase tracking-[0.18em] text-gold">On this page</p>
          <ul className="mt-2 space-y-1">
            {firstJobs.map((c) => (
              <li key={c.id}>
                <a href={`#${c.id}`} className="text-foam hover:text-gold">
                  {c.name}
                </a>
                {c.advancesTo.length ? ` → ${c.advancesTo.join(" / ")}` : ""}
              </li>
            ))}
          </ul>
        </nav>

        <section className="mt-12">
          <h2 className="font-display text-[1.8rem] tracking-wide text-gold-2">How jobs work</h2>
          <p className="mt-3 text-[16px] leading-7 text-muted">
            You start as a Newbie. Around level 10 you pick a first craft. Later, an NPC quest
            unlocks the second job — and you keep the first-job skills. If you want the interactive
            skill list, open the{" "}
            <Link href="/classes" className="text-gold hover:underline">
              class tree
            </Link>
            .
          </p>
        </section>

        {firstJobs.map((job) => {
          const next = secondJobsOf(job);
          return (
            <section key={job.id} id={job.id} className="mt-14 scroll-mt-24">
              <h2 className="font-display text-[1.8rem] tracking-wide text-gold-2">{job.name}</h2>
              <p className="mt-1 text-xs uppercase tracking-[0.16em] text-gold">{job.weapon}</p>
              <p className="mt-3 text-[16px] leading-7 text-muted">{job.blurb}</p>
              <p className="mt-2 text-sm text-foam/80">{job.style}</p>

              <h3 className="mt-6 font-display text-lg tracking-wide text-foam">Second jobs</h3>
              <ul className="mt-3 space-y-4">
                {next.map((adv) => (
                  <li key={adv.id} className="rounded-xl border border-line/80 bg-panel/40 p-4">
                    <p className="font-display text-lg text-gold-2">{adv.name}</p>
                    <p className="mt-1 text-xs uppercase tracking-[0.16em] text-gold">{adv.weapon}</p>
                    <p className="mt-2 text-sm leading-6 text-muted">{adv.blurb}</p>
                  </li>
                ))}
              </ul>
            </section>
          );
        })}

        <section className="mt-14">
          <h2 className="font-display text-[1.8rem] tracking-wide text-gold-2">Quick picks</h2>
          <ul className="mt-4 space-y-2 text-[16px] leading-7 text-muted">
            <li>
              <strong className="text-foam">Want frontline?</strong> Swordsman → Champion (tank) or
              Crusader (speed / dual blades).
            </li>
            <li>
              <strong className="text-foam">Want range?</strong> Hunter → Sharpshooter (guns,
              silence, headshot).
            </li>
            <li>
              <strong className="text-foam">Want support or control?</strong> Herbalist → Cleric
              (party) or Seal Master (debuffs).
            </li>
            <li>
              <strong className="text-foam">Want the sea?</strong> Explorer → Voyager (coral arts,
              ship fights).
            </li>
          </ul>
        </section>

        <section className="card-glow mt-14 rounded-xl p-6">
          <h2 className="font-display text-xl tracking-wide text-gold-2">Ready to sail?</h2>
          <p className="mt-2 text-sm leading-6 text-muted">
            Reserve your username and tell us which first job you lean toward — it helps us balance
            launch events.
          </p>
          <div className="mt-5 flex flex-wrap gap-3">
            <Link href="/prereg" className="btn-crimson" data-track="cta_prereg_from_guide">
              Pre-register
            </Link>
            <Link href="/classes" className="btn-ghost">
              Open class tree
            </Link>
          </div>
          <p className="mt-4 text-xs text-muted">
            Covering {playClasses.length} classes in the live Abyss-Sea / PKO set.
          </p>
        </section>
      </article>
    </Shell>
  );
}
