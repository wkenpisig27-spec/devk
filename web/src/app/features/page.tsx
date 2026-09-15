import type { Metadata } from "next";
import { Shell } from "@/components/Shell";
import { features, pillars } from "@/data/features";

export const metadata: Metadata = {
  title: "Features",
  description: "What you can play in Abyss-Sea Online — classes, ships, stalls, and a world that stays up.",
};

const statusLabel = {
  live: "Ready",
  "in-progress": "In the works",
  coming: "Coming soon",
};

export default function FeaturesPage() {
  return (
    <Shell>
      <div className="mx-auto max-w-[40rem] px-4 py-16 sm:px-6">
        <p className="text-[0.7rem] font-semibold uppercase tracking-[0.28em] text-gold">Play</p>
        <h1 className="title-glow font-display mt-3 text-[2.6rem] leading-none tracking-wide sm:text-[3.4rem]">
          Why you&apos;ll want a boat
        </h1>
        <p className="mt-5 text-[17px] leading-7 text-muted">
          Classic PKO first. Then the extras that make a crew stay: looks, stalls, parties,
          and a world that does not fall over on a Friday night.
        </p>

        {pillars.map((pillar) => {
          const group = features.filter((f) => f.pillar === pillar.id);
          return (
            <section key={pillar.id} className="mt-16">
              <h2 className="font-display text-[1.7rem] tracking-wide text-gold-2">{pillar.label}</h2>
              <div className="mt-5 grid gap-4">
                {group.map((f) => (
                  <article
                    key={f.id}
                    className="card-glow rounded-xl p-5"
                  >
                    <div className="flex items-center justify-between gap-3">
                      <h3 className="font-display text-xl text-foam">{f.title}</h3>
                      <span className="shrink-0 text-[11px] uppercase tracking-[0.16em] text-sea">
                        {statusLabel[f.status]}
                      </span>
                    </div>
                    <p className="mt-2 text-sm font-medium text-foam/80">{f.blurb}</p>
                      <p className="mt-2 text-sm leading-6 text-muted">{f.body}</p>
                  </article>
                ))}
              </div>
            </section>
          );
        })}
      </div>
    </Shell>
  );
}
