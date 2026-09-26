"use client";

import { useEffect, useMemo, useState, useTransition, type CSSProperties } from "react";
import {
  aniimoRoster,
  partyPresets,
  partnerPool,
  upgradePriority,
  type AniimoBuild,
  type PartyPreset,
} from "@/data/irisalisParty";

const SLOT_LABELS = ["Lead · Irisalis", "Partner 2", "Partner 3", "Partner 4"] as const;
const STORAGE_KEY = "irisalis-party-v1";

function isSavedParty(value: unknown): value is {
  presetId: string;
  slots: [string, string, string, string];
} {
  if (!value || typeof value !== "object") return false;
  const slots = (value as { slots?: unknown }).slots;
  const presetId = (value as { presetId?: unknown }).presetId;
  if (typeof presetId !== "string" || !Array.isArray(slots) || slots.length !== 4) return false;
  if (slots[0] !== "irisalis") return false;
  const partners = slots.slice(1);
  if (partners.some((id) => typeof id !== "string" || (id !== "" && !aniimoRoster[id]))) return false;
  const filled = partners.filter((id) => id !== "");
  return new Set(filled).size === filled.length;
}

function roleTone(role: AniimoBuild["role"]) {
  switch (role) {
    case "DPS":
      return "tone-dps";
    case "Break":
      return "tone-break";
    case "Heal":
      return "tone-heal";
    case "Regen":
      return "tone-regen";
    default:
      return "tone-support";
  }
}

function AniimoCard({
  aniimo,
  slotIndex,
  selected,
  onSelect,
  locked,
}: {
  aniimo: AniimoBuild | null;
  slotIndex: number;
  selected: boolean;
  onSelect: () => void;
  locked?: boolean;
}) {
  return (
    <button
      type="button"
      onClick={onSelect}
      className={`party-slot ${selected ? "is-selected" : ""} ${locked ? "is-locked" : ""}`}
      style={aniimo ? ({ "--slot-accent": aniimo.accent } as CSSProperties) : undefined}
      aria-pressed={selected}
    >
      <span className="slot-index">{SLOT_LABELS[slotIndex]}</span>
      {aniimo ? (
        <>
          <span className="slot-name">{aniimo.name}</span>
          <span className={`slot-role ${roleTone(aniimo.role)}`}>{aniimo.role}</span>
          <span className="slot-meta">
            No.{aniimo.number} · {aniimo.elements.join(" / ")}
          </span>
          <span className="slot-stat">{aniimo.bestStat}</span>
          {locked ? (
            <span className="slot-action slot-action-locked">Main · locked</span>
          ) : (
            <span className="slot-action">{selected ? "Picking…" : "Change partner"}</span>
          )}
        </>
      ) : (
        <span className="slot-empty">Pick a partner</span>
      )}
    </button>
  );
}

function partyNotes(party: (AniimoBuild | null)[]) {
  const roles = new Set(party.filter((a): a is AniimoBuild => !!a).map((a) => a.role));
  const notes: { ok: boolean; text: string }[] = [
    { ok: true, text: "Irisalis locked as Grass DPS" },
    {
      ok: roles.has("Break"),
      text: roles.has("Break")
        ? "Break seat opens windows for clone burst"
        : "Missing Break — her burst waits on the target’s bar",
    },
    {
      ok: roles.has("Regen") || roles.has("Heal"),
      text:
        roles.has("Regen") || roles.has("Heal")
          ? "Regen or Heal keeps Dance Power loops funded"
          : "Missing Regen or Heal — EP and HP drop between clone casts",
    },
  ];
  return notes;
}

function PartyCheck({ party }: { party: (AniimoBuild | null)[] }) {
  const notes = partyNotes(party);
  return (
    <ul className="party-check" aria-label="Party coverage">
      {notes.map((note) => (
        <li key={note.text} className={note.ok ? "is-ok" : "is-gap"}>
          {note.text}
        </li>
      ))}
    </ul>
  );
}

function loadoutText(party: (AniimoBuild | null)[]) {
  const lines = party.map((aniimo, i) => {
    if (!aniimo) return `${SLOT_LABELS[i]}: empty`;
    return [
      `${SLOT_LABELS[i]}: ${aniimo.name} (${aniimo.role})`,
      `  Item: ${aniimo.carriedItem.name} → ${aniimo.carriedItem.target}`,
      `  Attr: ${aniimo.attributes[0]} → ${aniimo.attributes[1]}`,
      `  First upgrade: ${aniimo.upgrades[0]}`,
    ].join("\n");
  });
  return ["Irisalis 4-Aniimo party", ...lines].join("\n\n");
}

function PartyLoadout({ party }: { party: (AniimoBuild | null)[] }) {
  const filled = party.filter((a): a is AniimoBuild => !!a);
  if (filled.length === 0) return null;

  return (
    <section className="loadout-board" aria-labelledby="loadout-heading">
      <div className="upgrade-intro">
        <p className="detail-kicker">Party gear</p>
        <h2 id="loadout-heading">Carried items at a glance</h2>
        <p>Every seat’s Legendary target, attribute order, and first upgrade cue — including Irisalis.</p>
        <CopyLoadout party={party} />
      </div>
      <div className="loadout-grid">
        {party.map((aniimo, i) =>
          aniimo ? (
            <article
              key={aniimo.id}
              className="loadout-card"
              style={{ "--slot-accent": aniimo.accent } as CSSProperties}
            >
              <p className="loadout-seat">{SLOT_LABELS[i]}</p>
              <h3>{aniimo.name}</h3>
              <p className={`slot-role ${roleTone(aniimo.role)}`}>{aniimo.role}</p>
              <p className="loadout-gear">{aniimo.carriedItem.name}</p>
              <p className="loadout-meta">{aniimo.carriedItem.base}</p>
              <p className="loadout-attrs">
                Attr · {aniimo.attributes[0]} → {aniimo.attributes[1]}
              </p>
              <p className="loadout-upgrade">{aniimo.upgrades[0]}</p>
            </article>
          ) : (
            <article key={`empty-${i}`} className="loadout-card is-empty">
              <p className="loadout-seat">{SLOT_LABELS[i]}</p>
              <h3>Empty</h3>
              <p className="loadout-meta">Pick a partner to fill this seat.</p>
            </article>
          ),
        )}
      </div>
    </section>
  );
}

function CopyLoadout({ party }: { party: (AniimoBuild | null)[] }) {
  const [copied, setCopied] = useState(false);

  async function copy() {
    try {
      await navigator.clipboard.writeText(loadoutText(party));
      setCopied(true);
      window.setTimeout(() => setCopied(false), 1600);
    } catch {
      setCopied(false);
    }
  }

  return (
    <button type="button" className="copy-loadout" onClick={copy}>
      {copied ? "Copied" : "Copy party & gear"}
    </button>
  );
}

function DetailPanel({ aniimo }: { aniimo: AniimoBuild }) {
  return (
    <div className="detail-panel" style={{ "--slot-accent": aniimo.accent } as CSSProperties}>
      <div className="detail-head">
        <p className="detail-kicker">
          No.{aniimo.number} · {aniimo.stage}
        </p>
        <h3>{aniimo.name}</h3>
        <p className="detail-blurb">{aniimo.traitBlurb}</p>
      </div>

      <dl className="detail-grid">
        <div>
          <dt>Role</dt>
          <dd className={roleTone(aniimo.role)}>{aniimo.role}</dd>
        </div>
        <div>
          <dt>Elements</dt>
          <dd>{aniimo.elements.join(" · ")}</dd>
        </div>
        <div>
          <dt>Total stats</dt>
          <dd>{aniimo.totalStats}</dd>
        </div>
        <div>
          <dt>Trait</dt>
          <dd>{aniimo.trait}</dd>
        </div>
      </dl>

      {aniimo.stats && (
        <section className="detail-block">
          <h4>Base stats</h4>
          <ul className="stat-bars">
            {(
              [
                ["HP", aniimo.stats.hp],
                ["ATK", aniimo.stats.atk],
                ["BREAK", aniimo.stats.break],
                ["P.DEF", aniimo.stats.pdef],
                ["M.DEF", aniimo.stats.mdef],
                ["REGEN", aniimo.stats.regen],
              ] as const
            ).map(([label, value]) => (
              <li key={label}>
                <span>{label}</span>
                <span className="stat-track" aria-hidden>
                  <span style={{ width: `${Math.min(100, (value / 140) * 100)}%` }} />
                </span>
                <strong>{value}</strong>
              </li>
            ))}
          </ul>
        </section>
      )}

      {aniimo.rotation && (
        <section className="detail-block">
          <h4>Rotation</h4>
          <ol className="rotation-list">
            {aniimo.rotation.map((step) => (
              <li key={step}>{step}</li>
            ))}
          </ol>
        </section>
      )}

      <section className="detail-block">
        <h4>Skills to run</h4>
        <ul>
          {aniimo.skills.map((s) => (
            <li key={s}>{s}</li>
          ))}
        </ul>
      </section>

      <section className="detail-block">
        <h4>Attributes to raise</h4>
        <ol className="attr-list">
          <li>
            <span>1st</span> {aniimo.attributes[0]}
          </li>
          <li>
            <span>2nd</span> {aniimo.attributes[1]}
          </li>
        </ol>
      </section>

      <section className="detail-block gear-block">
        <h4>Carried item</h4>
        <p className="gear-name">{aniimo.carriedItem.name}</p>
        <p className="gear-line">{aniimo.carriedItem.base}</p>
        <p className="gear-line">{aniimo.carriedItem.core}</p>
        <p className="gear-why">{aniimo.carriedItem.why}</p>
        <p className="gear-target">Upgrade target · {aniimo.carriedItem.target}</p>
        {aniimo.altItem && (
          <p className="gear-alt">
            Alt · {aniimo.altItem.name}. {aniimo.altItem.why}
          </p>
        )}
      </section>

      <section className="detail-block">
        <h4>What to upgrade</h4>
        <ul className="upgrade-list">
          {aniimo.upgrades.map((u) => (
            <li key={u}>{u}</li>
          ))}
        </ul>
      </section>
    </div>
  );
}

export function IrisalisPartyBuilder() {
  const [presetId, setPresetId] = useState(partyPresets[0].id);
  const [slots, setSlots] = useState<[string, string, string, string]>([
    ...partyPresets[0].slots,
  ]);
  const [activeSlot, setActiveSlot] = useState(1);
  const [ready, setReady] = useState(false);
  const [isPending, startTransition] = useTransition();

  useEffect(() => {
    try {
      const saved = JSON.parse(localStorage.getItem(STORAGE_KEY) ?? "null");
      if (isSavedParty(saved)) {
        setPresetId(saved.presetId);
        setSlots(saved.slots);
      }
    } catch {
      /* ignore broken storage */
    }
    setReady(true);
  }, []);

  useEffect(() => {
    if (!ready) return;
    localStorage.setItem(STORAGE_KEY, JSON.stringify({ presetId, slots }));
  }, [presetId, slots, ready]);

  const preset = partyPresets.find((p) => p.id === presetId) ?? partyPresets[0];
  const party = useMemo(
    () => slots.map((id) => aniimoRoster[id] ?? null) as [
      AniimoBuild,
      AniimoBuild | null,
      AniimoBuild | null,
      AniimoBuild | null,
    ],
    [slots],
  );
  const active = party[activeSlot] ?? party[0];

  function applyPreset(next: PartyPreset) {
    startTransition(() => {
      setPresetId(next.id);
      setSlots([...next.slots]);
      setActiveSlot(1);
    });
  }

  function pickPartner(id: string) {
    if (activeSlot === 0) return;
    startTransition(() => {
      setSlots((prev) => {
        const next = [...prev] as [string, string, string, string];
        // Avoid duplicates in other partner seats
        for (let i = 1; i < 4; i++) {
          if (i !== activeSlot && next[i] === id) next[i] = "";
        }
        next[activeSlot] = id;
        return next;
      });
      setPresetId("custom");
    });
  }

  const availablePartners = partnerPool.filter((p) => {
    const usedElsewhere = slots.some((id, i) => i !== activeSlot && id === p.id);
    return !usedElsewhere || slots[activeSlot] === p.id;
  });

  return (
    <div className={`builder ${isPending ? "is-pending" : ""}`}>
      <section className="preset-row" aria-label="Party presets">
        {partyPresets.map((p) => (
          <button
            key={p.id}
            type="button"
            className={`preset-chip ${presetId === p.id ? "is-active" : ""}`}
            onClick={() => applyPreset(p)}
          >
            <span className="preset-score">{p.score}</span>
            <span className="preset-name">{p.name}</span>
            <span className="preset-tag">{p.tagline}</span>
          </button>
        ))}
        {presetId === "custom" && (
          <span className="preset-chip is-active is-custom">
            <span className="preset-score">Custom</span>
            <span className="preset-name">Your bloom</span>
            <span className="preset-tag">Edited seats</span>
          </span>
        )}
      </section>

      <p className="playstyle">{presetId === "custom" ? "Mix partners for your roster." : preset.playstyle}</p>

      <PartyCheck party={party} />

      <div className="party-grid" role="list">
        {party.map((aniimo, i) => (
          <AniimoCard
            key={i}
            aniimo={aniimo}
            slotIndex={i}
            selected={activeSlot === i}
            locked={i === 0}
            onSelect={() => setActiveSlot(i)}
          />
        ))}
      </div>

      <div className="seat-tabs" role="tablist" aria-label="Inspect party seat">
        {party.map((aniimo, i) => (
          <button
            key={`tab-${i}`}
            type="button"
            role="tab"
            aria-selected={activeSlot === i}
            className={`seat-tab ${activeSlot === i ? "is-active" : ""}`}
            onClick={() => setActiveSlot(i)}
          >
            Seat {i + 1}
            <span>{aniimo?.name ?? "Empty"}</span>
          </button>
        ))}
      </div>

      {activeSlot > 0 && (
        <section className="picker" aria-label="Choose partner">
          <h3>Choose seat {activeSlot + 1}</h3>
          <div className="picker-grid">
            {availablePartners.map((p) => (
              <button
                key={p.id}
                type="button"
                className={`picker-card ${slots[activeSlot] === p.id ? "is-picked" : ""}`}
                style={{ "--slot-accent": p.accent } as CSSProperties}
                onClick={() => pickPartner(p.id)}
              >
                <span className="picker-name">{p.name}</span>
                <span className={`slot-role ${roleTone(p.role)}`}>{p.role}</span>
                <span className="picker-meta">{p.elements.join(" / ")}</span>
              </button>
            ))}
          </div>
        </section>
      )}

      {active && <DetailPanel aniimo={active} />}

      <PartyLoadout party={party} />

      <section id="upgrades" className="upgrade-board" aria-labelledby="upgrade-heading">
        <div className="upgrade-intro">
          <p className="detail-kicker">Main Aniimo · Irisalis</p>
          <h2 id="upgrade-heading">Upgrade order</h2>
          <p>
            Spend on Irisalis before polishing partners. She is the T0 Grass DPS and the only seat
            that blooms twice.
          </p>
        </div>
        <ol className="upgrade-steps">
          {upgradePriority.map((step) => (
            <li key={step.title}>
              <strong>{step.title}</strong>
              <span>{step.body}</span>
            </li>
          ))}
        </ol>
      </section>
    </div>
  );
}
