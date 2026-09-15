import type { Metadata } from "next";
import { Shell } from "@/components/Shell";
import { chapters } from "@/data/progress";

export const metadata: Metadata = {
  title: "What's new",
  description:
    "What’s new in Abyss-Sea Online — looks, crews, stalls, and a world that stays worth sailing.",
};

const tagLabel: Record<string, string> = {
  look: "Look",
  world: "Adventure",
  crew: "Crew",
  "fair-play": "Fair play",
  quality: "Feel",
};

const tagColor: Record<string, string> = {
  look: "text-sea",
  world: "text-gold",
  crew: "text-emerald-300",
  "fair-play": "text-crimson",
  quality: "text-foam/70",
};

export default function ProgressPage() {
  return (
    <Shell>
      <div className="mx-auto max-w-[40rem] px-4 py-16 sm:px-6">
        <p className="text-[0.7rem] font-semibold uppercase tracking-[0.28em] text-gold">Voyage log</p>
        <h1 className="title-glow font-display mt-3 text-[2.6rem] leading-none tracking-wide sm:text-[3.4rem]">What&apos;s new</h1>
        <p className="mt-5 text-[17px] leading-7 text-muted">
          Patch notes written for sailors. What changed in the world you walk, the fights
          you take, and the crew you sail with — not a developer diary.
        </p>

        <div className="mt-14 space-y-20">
          {chapters.map((chapter) => (
            <section key={chapter.id}>
              <p className="text-xs uppercase tracking-[0.25em] text-sea">{chapter.period}</p>
              <h2 className="font-display mt-2 text-[1.7rem] tracking-wide text-gold-2 sm:text-[2rem]">
                {chapter.headline}
              </h2>
              <p className="mt-3 text-foam/65">{chapter.intro}</p>
              <ol className="mt-8 space-y-8">
                {chapter.items.map((item) => (
                  <li
                    key={`${item.date}-${item.title}`}
                    className="relative border-l border-gold/30 pl-6"
                  >
                    <span className="absolute -left-[5px] top-1.5 h-2.5 w-2.5 rounded-full bg-gold" />
                    <div className="flex flex-wrap items-center gap-3">
                      <time className="text-xs text-foam/50">{item.date}</time>
                      <span
                        className={`text-[11px] uppercase tracking-[0.18em] ${tagColor[item.tag]}`}
                      >
                        {tagLabel[item.tag]}
                      </span>
                    </div>
                    <h3 className="mt-2 text-xl text-foam">{item.title}</h3>
                    <p className="mt-1 text-foam/70">{item.summary}</p>
                    <ul className="mt-3 list-disc space-y-1 pl-5 text-sm text-foam/55">
                      {item.details.map((d) => (
                        <li key={d}>{d}</li>
                      ))}
                    </ul>
                  </li>
                ))}
              </ol>
            </section>
          ))}
        </div>
      </div>
    </Shell>
  );
}
