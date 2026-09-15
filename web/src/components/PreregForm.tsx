"use client";

import { useMemo, useState, type FormEvent } from "react";
import { factionOptions, firstJobOptions, intentOptions, site } from "@/data/site";
import { getStoredReferral, track } from "@/components/Analytics";
import { PreregCounter } from "@/components/PreregCounter";

type Success = {
  referralCode: string;
  username: string;
  count: number;
};

export function PreregForm() {
  const [email, setEmail] = useState("");
  const [username, setUsername] = useState("");
  const [firstJob, setFirstJob] = useState("swordsman");
  const [intent, setIntent] = useState("yes");
  const [faction, setFaction] = useState("undecided");
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<Success | null>(null);

  const shareUrl = useMemo(() => {
    if (!success) return "";
    return `${site.url}/prereg?ref=${success.referralCode}`;
  }, [success]);

  async function onSubmit(e: FormEvent) {
    e.preventDefault();
    setBusy(true);
    setError(null);
    try {
      const res = await fetch("/api/prereg", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          email,
          username,
          firstJob,
          intent,
          faction,
          referredBy: getStoredReferral(),
        }),
      });
      const data = (await res.json()) as Success & { error?: string };
      if (!res.ok) {
        setError(data.error || "Could not save. Try again.");
        return;
      }
      setSuccess({
        referralCode: data.referralCode,
        username: data.username,
        count: data.count,
      });
      track("prereg_success", { firstJob, intent });
    } catch {
      setError("Network error. Try again.");
    } finally {
      setBusy(false);
    }
  }

  if (success) {
    return (
      <div className="card-glow mt-10 rounded-2xl p-6">
        <p className="text-[0.7rem] font-semibold uppercase tracking-[0.22em] text-gold">You are in</p>
        <h3 className="font-display mt-2 text-2xl tracking-wide text-gold-2">
          {success.username} is reserved
        </h3>
        <p className="mt-3 text-[16px] leading-7 text-muted">
          We saved your spot. Join Discord so you hear the opening bell, then share your link so
          crewmates can sail with you.
        </p>
        <p className="mt-4 text-sm text-foam">
          Your referral code:{" "}
          <span className="font-semibold text-gold">{success.referralCode}</span>
        </p>
        <div className="mt-3 break-all rounded-lg border border-line bg-ink-deep px-3 py-2 text-sm text-muted">
          {shareUrl}
        </div>
        <div className="mt-6 flex flex-wrap gap-3">
          <a
            href={site.discord}
            target="_blank"
            rel="noreferrer"
            className="btn-crimson"
            data-track="cta_discord_after_prereg"
          >
            Join Discord
          </a>
          <button
            type="button"
            className="btn-ghost"
            onClick={async () => {
              try {
                await navigator.clipboard.writeText(shareUrl);
                track("referral_copy");
              } catch {
                /* ignore */
              }
            }}
          >
            Copy invite link
          </button>
        </div>
        <p className="mt-4 text-sm text-muted">
          Fleet size now: <span className="text-gold">{success.count.toLocaleString()}</span>
        </p>
      </div>
    );
  }

  return (
    <form onSubmit={onSubmit} className="card-glow mt-10 space-y-5 rounded-2xl p-6">
      <div className="flex flex-wrap items-end justify-between gap-3">
        <div>
          <p className="text-[0.7rem] font-semibold uppercase tracking-[0.22em] text-gold">
            Claim your spot
          </p>
          <h3 className="font-display mt-1 text-xl tracking-wide text-foam">Pre-register</h3>
        </div>
        <PreregCounter />
      </div>

      <label className="block">
        <span className="text-xs uppercase tracking-[0.16em] text-gold">Email</span>
        <input
          required
          type="email"
          value={email}
          onChange={(e) => setEmail(e.target.value)}
          className="mt-1.5 w-full rounded-lg border border-line bg-ink-deep px-3 py-2.5 text-foam outline-none focus:border-gold/50"
          placeholder="you@crew.mail"
          autoComplete="email"
        />
      </label>

      <label className="block">
        <span className="text-xs uppercase tracking-[0.16em] text-gold">Desired username</span>
        <input
          required
          value={username}
          onChange={(e) => setUsername(e.target.value)}
          className="mt-1.5 w-full rounded-lg border border-line bg-ink-deep px-3 py-2.5 text-foam outline-none focus:border-gold/50"
          placeholder="CaptainName"
          minLength={3}
          maxLength={16}
          pattern="[A-Za-z][A-Za-z0-9_]{2,15}"
          title="3–16 characters, start with a letter"
          autoComplete="username"
        />
      </label>

      <fieldset>
        <legend className="text-xs uppercase tracking-[0.16em] text-gold">First job</legend>
        <div className="mt-2 grid grid-cols-2 gap-2">
          {firstJobOptions.map((job) => (
            <label
              key={job.id}
              className={`cursor-pointer rounded-lg border px-3 py-2 text-sm ${
                firstJob === job.id
                  ? "border-gold/50 bg-gold/10 text-foam"
                  : "border-line text-muted hover:border-gold/30"
              }`}
            >
              <input
                type="radio"
                className="sr-only"
                name="firstJob"
                value={job.id}
                checked={firstJob === job.id}
                onChange={() => setFirstJob(job.id)}
              />
              {job.label}
            </label>
          ))}
        </div>
      </fieldset>

      <label className="block">
        <span className="text-xs uppercase tracking-[0.16em] text-gold">Will you play at launch?</span>
        <select
          value={intent}
          onChange={(e) => setIntent(e.target.value)}
          className="mt-1.5 w-full rounded-lg border border-line bg-ink-deep px-3 py-2.5 text-foam outline-none focus:border-gold/50"
        >
          {intentOptions.map((o) => (
            <option key={o.id} value={o.id}>
              {o.label}
            </option>
          ))}
        </select>
      </label>

      <label className="block">
        <span className="text-xs uppercase tracking-[0.16em] text-gold">Faction lean</span>
        <select
          value={faction}
          onChange={(e) => setFaction(e.target.value)}
          className="mt-1.5 w-full rounded-lg border border-line bg-ink-deep px-3 py-2.5 text-foam outline-none focus:border-gold/50"
        >
          {factionOptions.map((o) => (
            <option key={o.id} value={o.id}>
              {o.label}
            </option>
          ))}
        </select>
      </label>

      {error ? <p className="text-sm text-crimson-2">{error}</p> : null}

      <button type="submit" disabled={busy} className="btn-crimson w-full sm:w-auto" data-track="cta_prereg_submit">
        {busy ? "Saving…" : "Reserve my name"}
      </button>
      <p className="text-xs leading-5 text-muted">
        We use this to hold your username and email you when accounts open. Join Discord after —
        that is where the crew lives.
      </p>
    </form>
  );
}
