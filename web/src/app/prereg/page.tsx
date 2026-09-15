import type { Metadata } from "next";
import { KitIcon } from "@/components/KitIcon";
import { Shell } from "@/components/Shell";
import { kit, preregSteps, site } from "@/data/site";

export const metadata: Metadata = {
  title: "Pre-register",
  description:
    "Pre-register for Abyss-Sea Online. Your name waits on opening day, with a starter kit tied to it.",
};

export default function PreregPage() {
  return (
    <Shell>
      <article className="mx-auto max-w-[40rem] px-4 py-16 sm:px-6">
        <p className="text-[0.7rem] font-semibold uppercase tracking-[0.28em] text-gold">
          Before the opening
        </p>
        <h1 className="title-glow font-display mt-3 text-[2.6rem] leading-none tracking-wide sm:text-[3.4rem]">
          Pre-register
        </h1>
        <p className="mt-5 text-[17px] leading-7 text-muted">
          Make your account with the crew now. When the seas open, it is already waiting —
          not a form we keep in a drawer — with a starter kit tied to it.
        </p>

        <h2 className="font-display mt-14 text-[2rem] tracking-wide text-gold-2">What you get</h2>
        <p className="mt-2 text-sm text-muted">Held for you until you claim it in game.</p>

        <div className="card-glow mt-6 grid grid-cols-2 gap-3 rounded-xl p-3 sm:grid-cols-4">
          {kit.map((item) => (
            <div
              key={item.name}
              className="flex flex-col items-center rounded-lg bg-ink-deep/80 px-2 py-4 text-center ring-1 ring-gold/10"
            >
              <KitIcon name={item.icon} />
              <p className="mt-2 text-[11px] leading-snug text-muted">
                {item.qty} {item.name}
              </p>
            </div>
          ))}
        </div>

        <ul className="mt-6 space-y-2.5 text-[17px] leading-7 text-foam">
          {kit.map((item) => (
            <li key={item.name} className="flex gap-2">
              <span className="mt-1 text-gold" aria-hidden>
                ✓
              </span>
              <span>
                <strong className="font-semibold text-foam">
                  {item.qty} {item.name}
                </strong>
                {item.note ? <span className="text-muted"> — {item.note}</span> : null}
              </span>
            </li>
          ))}
        </ul>

        <aside className="card-glow mt-8 rounded-xl border-l-4 border-l-gold px-5 py-4">
          <p className="text-[0.7rem] font-semibold uppercase tracking-[0.2em] text-gold">
            One gift per person
          </p>
          <p className="mt-2 text-[15px] leading-6 text-foam/90">
            Not one per Discord alt — <strong>one per person</strong>. The kit is{" "}
            <strong>bound</strong>: it cannot be traded, sold, or dropped, so it stays on
            whoever claims it. Pick the character you mean to keep.
          </p>
        </aside>

        <h2 className="font-display mt-14 text-[2rem] tracking-wide text-gold-2">How it works</h2>
        <ol className="mt-6 space-y-5">
          {preregSteps.map((step) => (
            <li key={step.n} className="flex gap-4 text-[17px] leading-7">
              <span className="flex h-8 w-8 shrink-0 items-center justify-center rounded-full border border-gold/50 bg-gold/10 font-display text-sm text-gold shadow-[0_0_12px_rgba(230,184,94,0.28)]">
                {step.n}
              </span>
              <p className="text-muted">
                <strong className="font-semibold text-foam">{step.title}</strong> {step.body}
              </p>
            </li>
          ))}
        </ol>

        <a href={site.discord} target="_blank" rel="noreferrer" className="btn-crimson mt-10">
          Join Discord to pre-register
        </a>

        <p className="mt-8 text-sm text-muted">
          <a href="/" className="text-gold hover:underline">
            ← Back to the site
          </a>
          <span className="mx-2 text-line">·</span>
          <a href={site.discord} target="_blank" rel="noreferrer" className="text-gold hover:underline">
            Ask on Discord
          </a>
        </p>
      </article>
    </Shell>
  );
}
