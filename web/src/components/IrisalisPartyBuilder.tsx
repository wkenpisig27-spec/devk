"use client";

import { useMemo, useState, useTransition, type CSSProperties } from "react";
import {
  aniimoRoster,
  partyPresets,
  partnerPool,
  upgradePriority,
  type AniimoBuild,
  type PartyPreset,
} from "@/data/irisalisParty";

const SLOT_LABELS = ["Lead · Irisalis", "Partner 2", "Partner 3", "Partner 4"] as const;

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
      disabled={locked && !!aniimo}
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
        </>
      ) : (
        <span className="slot-empty">Pick a partner</span>
      )}
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
  const [activeSlot, setActiveSlot] = useState(0);
  const [isPending, startTransition] = useTransition();

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
      setActiveSlot(0);
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
