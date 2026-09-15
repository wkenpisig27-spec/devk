"use client";

import { useMemo, useState } from "react";
import {
  firstJobs,
  ownSkills,
  secondJobsOf,
  type PlayClass,
  type Skill,
} from "@/data/classes";
import { SkillIcon } from "@/components/SkillIcon";

type Filter = "all" | "passive" | "initiate";

function ClassPortrait({ src, className }: { src?: string; className?: string }) {
  if (!src) return null;
  return (
    <img
      src={src}
      alt=""
      width={256}
      height={256}
      className={className ?? "mx-auto block h-auto w-full min-w-0 object-contain"}
    />
  );
}

function SkillGrid({
  cls,
  skills,
  active,
  onPick,
}: {
  cls: PlayClass;
  skills: Skill[];
  active?: Skill;
  onPick: (name: string) => void;
}) {
  return (
    <ul className="mt-4 grid grid-cols-2 gap-2 sm:grid-cols-3">
      {skills.map((s) => {
        const on = active?.name === s.name && (active.from ?? cls.name) === (s.from ?? cls.name);
        return (
          <li key={`${cls.id}-${s.name}`}>
            <button
              type="button"
              onClick={() => onPick(s.name)}
              className={`flex w-full items-center gap-2 rounded-lg px-2 py-2 text-left transition ${
                on ? "bg-gold/15 ring-1 ring-gold/50" : "border border-line/80 hover:border-gold/40"
              }`}
            >
              <span className="flex h-8 w-8 shrink-0 items-center justify-center overflow-hidden rounded-md border border-line bg-ink-deep">
                <SkillIcon name={s.name} kind={s.icon} />
              </span>
              <span>
                <span className="block text-sm font-medium text-foam">{s.name}</span>
                <span className="block text-[10px] uppercase tracking-wider text-muted">{s.type}</span>
              </span>
            </button>
          </li>
        );
      })}
    </ul>
  );
}

function SkillDetail({ skill, cls }: { skill?: Skill; cls: PlayClass }) {
  if (!skill) return null;
  return (
    <div className="mt-6 rounded-xl border border-gold/25 bg-ink-deep/70 p-4">
      <div className="flex items-start gap-3">
        <span className="flex h-12 w-12 items-center justify-center overflow-hidden rounded-lg border border-gold/20 bg-ink-deep">
          <SkillIcon name={skill.name} kind={skill.icon} />
        </span>
        <div>
          <p className="font-display text-lg tracking-wide text-foam">{skill.name}</p>
          <p className="text-[11px] uppercase tracking-[0.16em] text-gold">
            {skill.type}
            {skill.sp ? ` · ${skill.sp}` : ""}
            {` · ${skill.from ?? cls.name}`}
          </p>
        </div>
      </div>
      <p className="mt-3 text-sm leading-6 text-muted">{skill.effect}</p>
    </div>
  );
}

export function ClassExplorer() {
  const [selectedId, setSelectedId] = useState("swordsman");
  const [skillName, setSkillName] = useState<string | null>(null);
  const [skillClassId, setSkillClassId] = useState<string | null>(null);
  const [filter, setFilter] = useState<Filter>("all");

  const selected = firstJobs.find((c) => c.id === selectedId) ?? firstJobs[0];
  const nextJobs = useMemo(() => secondJobsOf(selected), [selected]);

  const firstSkills = ownSkills(selected).filter((s) => filter === "all" || s.type === filter);

  const skillOwner =
    [selected, ...nextJobs].find((c) => c.id === skillClassId) ?? selected;
  const activeSkill: Skill | undefined =
    ownSkills(skillOwner).find((s) => s.name === skillName) ?? firstSkills[0];

  function pickFirst(id: string) {
    setSelectedId(id);
    setSkillName(null);
    setSkillClassId(id);
    setFilter("all");
  }

  function pickSkill(cls: PlayClass, name: string) {
    setSkillClassId(cls.id);
    setSkillName(name);
  }

  return (
    <div className="mt-10 space-y-8">
      <div>
        <p className="text-[0.7rem] font-semibold uppercase tracking-[0.22em] text-gold">First job</p>
        <div className="mt-3 grid grid-cols-2 gap-3 sm:grid-cols-4">
          {firstJobs.map((c) => {
            const on = c.id === selected.id;
            return (
              <button
                key={c.id}
                type="button"
                onClick={() => pickFirst(c.id)}
                className={`min-w-0 overflow-visible rounded-xl p-2 text-center transition ${
                  on ? "card-glow ring-1 ring-gold/50" : "border border-line bg-panel/50 hover:border-gold/40"
                }`}
              >
                <ClassPortrait src={c.portrait} className="mx-auto block h-auto w-full max-w-[9rem] object-contain" />
                <p className="mt-2 font-display text-sm tracking-wide text-foam">{c.name}</p>
              </button>
            );
          })}
        </div>
      </div>

      <section className="card-glow grid gap-8 rounded-2xl p-6 lg:grid-cols-[minmax(0,1fr)_minmax(0,1.15fr)]">
        <div>
          <div className="flex items-start gap-4">
            <ClassPortrait src={selected.portrait} className="h-[5.5rem] w-[5.5rem] shrink-0 object-contain" />
            <div>
              <p className="text-[0.7rem] font-semibold uppercase tracking-[0.22em] text-gold">First job</p>
              <h2 className="font-display mt-2 text-[2rem] tracking-wide text-gold-2">{selected.name}</h2>
              <p className="mt-1 text-xs uppercase tracking-[0.16em] text-gold">{selected.weapon}</p>
            </div>
          </div>
          <p className="mt-4 text-[16px] leading-7 text-muted">{selected.blurb}</p>
          <p className="mt-3 text-sm text-foam/80">{selected.style}</p>
          <SkillDetail skill={skillOwner.id === selected.id ? activeSkill : firstSkills[0]} cls={selected} />
        </div>

        <div>
          <div className="flex flex-wrap items-center justify-between gap-3">
            <h3 className="font-display text-lg tracking-wide text-foam">Skills</h3>
            <div className="flex gap-1 rounded-lg border border-line p-1">
              {(["all", "passive", "initiate"] as const).map((f) => (
                <button
                  key={f}
                  type="button"
                  onClick={() => {
                    setFilter(f);
                    setSkillName(null);
                    setSkillClassId(selected.id);
                  }}
                  className={`rounded-md px-2.5 py-1 text-xs capitalize ${
                    filter === f ? "bg-gold/20 text-gold" : "text-muted hover:text-foam"
                  }`}
                >
                  {f}
                </button>
              ))}
            </div>
          </div>
          <p className="mt-1 text-xs text-muted">Click a skill to read it. Second jobs keep this kit.</p>
          <SkillGrid
            cls={selected}
            skills={firstSkills}
            active={skillOwner.id === selected.id ? activeSkill : undefined}
            onPick={(name) => pickSkill(selected, name)}
          />
        </div>
      </section>

      <section>
        <p className="text-[0.7rem] font-semibold uppercase tracking-[0.22em] text-gold">Second job</p>
        <h3 className="font-display mt-2 text-xl tracking-wide text-foam">
          {selected.name} advances at the class quest
        </h3>
        <p className="mt-2 max-w-[40rem] text-sm leading-6 text-muted">
          NPC quest — not a cash shop. You keep the first-job skills and unlock a new set.
        </p>
        <div className={`mt-5 grid gap-4 ${nextJobs.length > 1 ? "lg:grid-cols-2" : ""}`}>
          {nextJobs.map((job) => {
            const skills = ownSkills(job).filter((s) => filter === "all" || s.type === filter);
            const active = skillOwner.id === job.id ? activeSkill : undefined;
            return (
              <article key={job.id} className="card-glow rounded-2xl p-5">
                <div className="flex items-start gap-4">
                  <ClassPortrait src={job.portrait} className="h-[5.5rem] w-[5.5rem] shrink-0 object-contain" />
                  <div>
                    <h4 className="font-display text-2xl tracking-wide text-gold-2">{job.name}</h4>
                    <p className="mt-1 text-xs uppercase tracking-[0.16em] text-gold">{job.weapon}</p>
                    <p className="mt-2 text-sm text-foam/80">{job.style}</p>
                  </div>
                </div>
                <p className="mt-4 text-[15px] leading-7 text-muted">{job.blurb}</p>
                <SkillGrid cls={job} skills={skills} active={active} onPick={(name) => pickSkill(job, name)} />
                {active ? <SkillDetail skill={active} cls={job} /> : null}
              </article>
            );
          })}
        </div>
      </section>
    </div>
  );
}
