"use client";

import { useEffect, useMemo, useState, type CSSProperties, type ReactNode } from "react";
import type { AniimoBuild, AniimoRole, Element } from "@/data/irisalisParty";
import {
  ANIIMO_ELEMENTS,
  ANIIMO_ROLES,
  LIBRARY_KEY,
  blankCustom,
  blankFromCatalog,
  isAniimoBuild,
  libraryRows,
  lines,
  mergeBuilds,
  parseLibraryFile,
  recordsToSave,
  seedBuilds,
  seedIds,
  type LibraryRow,
} from "@/lib/aniimoLibrary";

type StatusFilter = "all" | "built" | "open";

function roleTone(role: AniimoRole) {
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

function readSaved(): AniimoBuild[] {
  try {
    const raw = localStorage.getItem(LIBRARY_KEY);
    if (!raw) return [];
    const data = JSON.parse(raw) as { builds?: unknown };
    if (!Array.isArray(data.builds)) return [];
    return data.builds.filter(isAniimoBuild);
  } catch {
    return [];
  }
}

function writeSaved(builds: AniimoBuild[]) {
  localStorage.setItem(LIBRARY_KEY, JSON.stringify({ version: 1, builds: recordsToSave(builds) }));
}

function BuildSheet({ build }: { build: AniimoBuild }) {
  return (
    <div className="detail-panel" style={{ "--slot-accent": build.accent } as CSSProperties}>
      <div className="detail-head">
        <p className="detail-kicker">
          No.{build.number || "—"}
          {build.stage ? ` · ${build.stage}` : ""}
        </p>
        <h3>{build.name}</h3>
        {build.traitBlurb ? <p className="detail-blurb">{build.traitBlurb}</p> : null}
      </div>

      <dl className="detail-grid">
        <div>
          <dt>Role</dt>
          <dd className={roleTone(build.role)}>{build.role}</dd>
        </div>
        <div>
          <dt>Elements</dt>
          <dd>{build.elements.length ? build.elements.join(" · ") : "—"}</dd>
        </div>
        <div>
          <dt>Total stats</dt>
          <dd>{build.totalStats || "—"}</dd>
        </div>
        <div>
          <dt>Trait</dt>
          <dd>{build.trait || "—"}</dd>
        </div>
      </dl>

      {build.stats ? (
        <section className="detail-block">
          <h4>Base stats</h4>
          <ul className="stat-bars">
            {(
              [
                ["HP", build.stats.hp],
                ["ATK", build.stats.atk],
                ["BREAK", build.stats.break],
                ["P.DEF", build.stats.pdef],
                ["M.DEF", build.stats.mdef],
                ["REGEN", build.stats.regen],
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
      ) : null}

      {build.rotation && build.rotation.length > 0 ? (
        <section className="detail-block">
          <h4>Rotation</h4>
          <ol className="rotation-list">
            {build.rotation.map((step) => (
              <li key={step}>{step}</li>
            ))}
          </ol>
        </section>
      ) : null}

      {build.skills.length > 0 ? (
        <section className="detail-block">
          <h4>Skills to run</h4>
          <ul>
            {build.skills.map((skill) => (
              <li key={skill}>{skill}</li>
            ))}
          </ul>
        </section>
      ) : null}

      <section className="detail-block">
        <h4>Attributes to raise</h4>
        <ol className="attr-list">
          <li>
            <span>1st</span> {build.attributes[0] || "—"}
          </li>
          <li>
            <span>2nd</span> {build.attributes[1] || "—"}
          </li>
        </ol>
      </section>

      <section className="detail-block gear-block">
        <h4>Carried item</h4>
        <p className="gear-name">{build.carriedItem.name || "Not set"}</p>
        {build.carriedItem.base ? <p className="gear-line">{build.carriedItem.base}</p> : null}
        {build.carriedItem.core ? <p className="gear-line">{build.carriedItem.core}</p> : null}
        {build.carriedItem.why ? <p className="gear-why">{build.carriedItem.why}</p> : null}
        <p className="gear-target">Upgrade target · {build.carriedItem.target || "Legendary"}</p>
        {build.altItem?.name ? (
          <p className="gear-alt">
            Alt · {build.altItem.name}
            {build.altItem.why ? `. ${build.altItem.why}` : ""}
          </p>
        ) : null}
      </section>

      {build.upgrades.length > 0 ? (
        <section className="detail-block">
          <h4>What to upgrade</h4>
          <ul className="upgrade-list">
            {build.upgrades.map((step) => (
              <li key={step}>{step}</li>
            ))}
          </ul>
        </section>
      ) : null}

      {build.id === "irisalis" ? (
        <p className="sheet-link">
          <a href="/tools/irisalis-party">Open the 4-Aniimo party builder</a>
        </p>
      ) : null}
    </div>
  );
}

function Field({
  label,
  children,
}: {
  label: string;
  children: ReactNode;
}) {
  return (
    <label className="build-field">
      <span>{label}</span>
      {children}
    </label>
  );
}

function BuildForm({
  draft,
  onChange,
  onSave,
  onCancel,
  error,
}: {
  draft: AniimoBuild;
  onChange: (next: AniimoBuild) => void;
  onSave: () => void;
  onCancel: () => void;
  error: string;
}) {
  function patch(partial: Partial<AniimoBuild>) {
    onChange({ ...draft, ...partial });
  }

  function toggleElement(element: Element) {
    const elements = draft.elements.includes(element)
      ? draft.elements.filter((item) => item !== element)
      : [...draft.elements, element];
    patch({ elements });
  }

  return (
    <form
      className="build-form detail-panel"
      style={{ "--slot-accent": draft.accent } as CSSProperties}
      onSubmit={(event) => {
        event.preventDefault();
        onSave();
      }}
    >
      <div className="detail-head">
        <p className="detail-kicker">Build sheet</p>
        <h3>{draft.name.trim() || "New Aniimo"}</h3>
      </div>

      <div className="form-grid">
        <Field label="Name">
          <input
            value={draft.name}
            onChange={(event) => patch({ name: event.target.value })}
            required
          />
        </Field>
        <Field label="Number">
          <input
            value={draft.number}
            onChange={(event) => patch({ number: event.target.value })}
            placeholder="013"
          />
        </Field>
        <Field label="Role">
          <select
            value={draft.role}
            onChange={(event) => patch({ role: event.target.value as AniimoRole })}
          >
            {ANIIMO_ROLES.map((role) => (
              <option key={role} value={role}>
                {role}
              </option>
            ))}
          </select>
        </Field>
        <Field label="Stage">
          <input
            value={draft.stage}
            onChange={(event) => patch({ stage: event.target.value })}
            placeholder="Nova"
          />
        </Field>
        <Field label="Trait">
          <input value={draft.trait} onChange={(event) => patch({ trait: event.target.value })} />
        </Field>
        <Field label="Best stat">
          <input
            value={draft.bestStat}
            onChange={(event) => patch({ bestStat: event.target.value })}
            placeholder="ATK 130"
          />
        </Field>
        <Field label="1st attribute">
          <input
            value={draft.attributes[0]}
            onChange={(event) => patch({ attributes: [event.target.value, draft.attributes[1]] })}
          />
        </Field>
        <Field label="2nd attribute">
          <input
            value={draft.attributes[1]}
            onChange={(event) => patch({ attributes: [draft.attributes[0], event.target.value] })}
          />
        </Field>
      </div>

      <fieldset className="element-set">
        <legend>Elements</legend>
        <div className="element-picks">
          {ANIIMO_ELEMENTS.map((element) => (
            <button
              key={element}
              type="button"
              className={draft.elements.includes(element) ? "is-on" : ""}
              onClick={() => toggleElement(element)}
              aria-pressed={draft.elements.includes(element)}
            >
              {element}
            </button>
          ))}
        </div>
      </fieldset>

      <Field label="Trait note">
        <textarea
          rows={3}
          value={draft.traitBlurb}
          onChange={(event) => patch({ traitBlurb: event.target.value })}
        />
      </Field>
      <Field label="Skills — one per line">
        <textarea
          rows={3}
          value={draft.skills.join("\n")}
          onChange={(event) => patch({ skills: lines(event.target.value) })}
        />
      </Field>

      <div className="form-grid">
        <Field label="Carried item">
          <input
            value={draft.carriedItem.name}
            onChange={(event) =>
              patch({ carriedItem: { ...draft.carriedItem, name: event.target.value } })
            }
            required
          />
        </Field>
        <Field label="Upgrade target">
          <input
            value={draft.carriedItem.target}
            onChange={(event) =>
              patch({ carriedItem: { ...draft.carriedItem, target: event.target.value } })
            }
          />
        </Field>
        <Field label="Item base">
          <input
            value={draft.carriedItem.base}
            onChange={(event) =>
              patch({ carriedItem: { ...draft.carriedItem, base: event.target.value } })
            }
            placeholder="Damage Amp +10% (Legendary)"
          />
        </Field>
        <Field label="Item core">
          <input
            value={draft.carriedItem.core}
            onChange={(event) =>
              patch({ carriedItem: { ...draft.carriedItem, core: event.target.value } })
            }
            placeholder="+0.7 ATK per level"
          />
        </Field>
      </div>
      <Field label="Why this item">
        <textarea
          rows={2}
          value={draft.carriedItem.why}
          onChange={(event) =>
            patch({ carriedItem: { ...draft.carriedItem, why: event.target.value } })
          }
        />
      </Field>
      <div className="form-grid">
        <Field label="Alt item">
          <input
            value={draft.altItem?.name ?? ""}
            onChange={(event) =>
              patch({
                altItem: { name: event.target.value, why: draft.altItem?.why ?? "" },
              })
            }
          />
        </Field>
        <Field label="Alt note">
          <input
            value={draft.altItem?.why ?? ""}
            onChange={(event) =>
              patch({
                altItem: { name: draft.altItem?.name ?? "", why: event.target.value },
              })
            }
          />
        </Field>
      </div>
      <Field label="What to upgrade — one per line">
        <textarea
          rows={4}
          value={draft.upgrades.join("\n")}
          onChange={(event) => patch({ upgrades: lines(event.target.value) })}
        />
      </Field>
      <Field label="Rotation — one step per line">
        <textarea
          rows={3}
          value={(draft.rotation ?? []).join("\n")}
          onChange={(event) => {
            const rotation = lines(event.target.value);
            patch({ rotation: rotation.length ? rotation : undefined });
          }}
        />
      </Field>

      {error ? <p className="form-error">{error}</p> : null}
      <div className="form-actions">
        <button type="submit" className="iris-cta">
          Save build
        </button>
        <button type="button" className="iris-cta iris-cta-ghost" onClick={onCancel}>
          Cancel
        </button>
      </div>
    </form>
  );
}

export function AniimoBuildLibrary() {
  const [builds, setBuilds] = useState<AniimoBuild[]>(() => mergeBuilds(null));
  const [ready, setReady] = useState(false);
  const [query, setQuery] = useState("");
  const [status, setStatus] = useState<StatusFilter>("all");
  const [role, setRole] = useState<AniimoRole | "all">("all");
  const [selectedKey, setSelectedKey] = useState("013-Irisalis");
  const [draft, setDraft] = useState<AniimoBuild | null>(null);
  const [error, setError] = useState("");
  const [notice, setNotice] = useState("");

  useEffect(() => {
    setBuilds(mergeBuilds(readSaved()));
    setReady(true);
  }, []);

  useEffect(() => {
    if (!ready) return;
    writeSaved(builds);
  }, [builds, ready]);

  const rows = useMemo(() => libraryRows(builds), [builds]);
  const filtered = useMemo(() => {
    const needle = query.trim().toLowerCase();
    return rows.filter((row) => {
      if (status === "built" && !row.build) return false;
      if (status === "open" && row.build) return false;
      if (role !== "all" && row.role !== role) return false;
      if (!needle) return true;
      const hay = [
        row.name,
        row.number,
        row.role ?? "",
        row.build?.carriedItem.name ?? "",
        row.build?.elements.join(" ") ?? "",
      ]
        .join(" ")
        .toLowerCase();
      return hay.includes(needle);
    });
  }, [rows, query, status, role]);

  const selected = rows.find((row) => row.key === selectedKey) ?? filtered[0] ?? rows[0];
  const seeds = seedIds();
  const builtCount = rows.filter((row) => row.build).length;

  function selectRow(row: LibraryRow) {
    if (draft && !window.confirm("Leave this sheet without saving?")) return;
    setDraft(null);
    setError("");
    setSelectedKey(row.key);
  }

  function startEdit(build: AniimoBuild) {
    setDraft(structuredClone(build));
    setError("");
  }

  function startFromRow(row: LibraryRow) {
    const taken = new Set(builds.map((build) => build.id));
    const next = row.catalog ? blankFromCatalog(row.catalog, taken) : blankCustom(taken);
    if (!row.catalog) next.name = "";
    setDraft(next);
    setError("");
    setSelectedKey(row.key);
  }

  function saveDraft() {
    if (!draft) return;
    const name = draft.name.trim();
    if (!name) {
      setError("Name the Aniimo before saving.");
      return;
    }
    if (!draft.carriedItem.name.trim()) {
      setError("Add a carried item. Use the same slot as the Irisalis sheet.");
      return;
    }
    const next = { ...draft, name, carriedItem: { ...draft.carriedItem, name: draft.carriedItem.name.trim() } };
    if (next.altItem && !next.altItem.name.trim()) delete next.altItem;
    setBuilds((prev) => {
      const index = prev.findIndex((build) => build.id === next.id);
      if (index === -1) return [...prev, next];
      const copy = [...prev];
      copy[index] = next;
      return copy;
    });
    setDraft(null);
    setError("");
    setNotice(`Saved ${name}.`);
    const match = libraryRows([...builds.filter((build) => build.id !== next.id), next]).find(
      (row) => row.build?.id === next.id,
    );
    if (match) setSelectedKey(match.key);
  }

  function resetSelected() {
    if (!selected?.build || !seeds.has(selected.build.id)) return;
    if (!window.confirm(`Restore the original ${selected.build.name} sheet?`)) return;
    const seed = seedBuilds().find((build) => build.id === selected.build?.id);
    if (!seed) return;
    setBuilds((prev) => prev.map((build) => (build.id === seed.id ? seed : build)));
    setDraft(null);
    setNotice(`Restored ${selected.name}.`);
  }

  function deleteSelected() {
    if (!selected?.build || seeds.has(selected.build.id)) return;
    if (!window.confirm(`Delete the ${selected.build.name} build?`)) return;
    setBuilds((prev) => prev.filter((build) => build.id !== selected.build?.id));
    setDraft(null);
    setNotice(`Deleted ${selected.name}.`);
  }

  function exportBuilds() {
    const payload = JSON.stringify({ version: 1, builds }, null, 2);
    const blob = new Blob([payload], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = "aniimo-builds.json";
    link.click();
    URL.revokeObjectURL(url);
  }

  async function importFile(file: File) {
    try {
      const { builds: incoming, skipped } = parseLibraryFile(await file.text());
      setBuilds((prev) => mergeBuilds([...recordsToSave(prev), ...incoming]));
      setNotice(
        `Imported ${incoming.length} build${incoming.length === 1 ? "" : "s"}${skipped ? ` (${skipped} skipped)` : ""}.`,
      );
    } catch {
      setNotice("That file is not an Aniimo build export.");
    }
  }

  return (
    <div className="lib-app">
      <div className="lib-toolbar">
        <input
          className="lib-search"
          value={query}
          onChange={(event) => setQuery(event.target.value)}
          placeholder="Search name, number, role, or item"
          aria-label="Search Aniimo"
        />
        <div className="lib-filters" role="group" aria-label="Build status">
          {(
            [
              ["all", "All"],
              ["built", "Has build"],
              ["open", "Needs build"],
            ] as const
          ).map(([id, label]) => (
            <button
              key={id}
              type="button"
              className={status === id ? "is-on" : ""}
              onClick={() => setStatus(id)}
            >
              {label}
            </button>
          ))}
        </div>
        <select
          aria-label="Filter by role"
          value={role}
          onChange={(event) => setRole(event.target.value as AniimoRole | "all")}
        >
          <option value="all">Every role</option>
          {ANIIMO_ROLES.map((item) => (
            <option key={item} value={item}>
              {item}
            </option>
          ))}
        </select>
        <button
          type="button"
          className="iris-cta"
          onClick={() => {
            const taken = new Set(builds.map((build) => build.id));
            const next = blankCustom(taken);
            next.name = "";
            setDraft(next);
            setSelectedKey("custom-new");
            setError("");
          }}
        >
          New build
        </button>
      </div>

      <p className="lib-count">
        {builtCount} builds saved · {rows.length - builtCount} Aniilog entries still open
        {notice ? ` · ${notice}` : ""}
      </p>

      <div className="lib-shell">
        <div className="lib-list" role="listbox" aria-label="Aniimo">
          {filtered.map((row) => (
            <button
              key={row.key}
              type="button"
              role="option"
              aria-selected={selected?.key === row.key}
              className={`lib-row ${selected?.key === row.key ? "is-selected" : ""} ${row.build ? "has-build" : ""}`}
              onClick={() => selectRow(row)}
            >
              <span className="lib-no">No.{row.number || "—"}</span>
              <span className="lib-name">{row.name}</span>
              {row.role ? <span className={`slot-role ${roleTone(row.role)}`}>{row.role}</span> : null}
              <span className="lib-state">{row.build ? row.build.carriedItem.name || "Build" : "Add build"}</span>
            </button>
          ))}
          {filtered.length === 0 ? <p className="lib-empty">No Aniimo match that search.</p> : null}
        </div>

        <div className="lib-detail">
          {draft ? (
            <BuildForm
              draft={draft}
              onChange={setDraft}
              onSave={saveDraft}
              onCancel={() => {
                setDraft(null);
                setError("");
              }}
              error={error}
            />
          ) : selected?.build ? (
            <>
              <div className="lib-actions">
                <button type="button" className="iris-cta" onClick={() => startEdit(selected.build!)}>
                  Edit build
                </button>
                {seeds.has(selected.build.id) ? (
                  <button type="button" className="iris-cta iris-cta-ghost" onClick={resetSelected}>
                    Restore original
                  </button>
                ) : (
                  <button type="button" className="iris-cta iris-cta-ghost" onClick={deleteSelected}>
                    Delete
                  </button>
                )}
              </div>
              <BuildSheet build={selected.build} />
            </>
          ) : selected ? (
            <div className="detail-panel empty-sheet">
              <p className="detail-kicker">No.{selected.number}</p>
              <h3>{selected.name}</h3>
              <p className="detail-blurb">
                No gear sheet yet. Start one in the same shape as Irisalis: carried item, attribute
                order, and what to upgrade.
              </p>
              <button type="button" className="iris-cta" onClick={() => startFromRow(selected)}>
                Write {selected.name}&apos;s build
              </button>
            </div>
          ) : null}
        </div>
      </div>

      <div className="lib-io">
        <button type="button" className="iris-cta iris-cta-ghost" onClick={exportBuilds}>
          Export JSON
        </button>
        <label className="iris-cta iris-cta-ghost">
          Import JSON
          <input
            type="file"
            accept="application/json,.json"
            hidden
            onChange={(event) => {
              const file = event.target.files?.[0];
              if (file) void importFile(file);
              event.target.value = "";
            }}
          />
        </label>
      </div>
    </div>
  );
}
