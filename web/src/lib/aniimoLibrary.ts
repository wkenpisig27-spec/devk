import { aniimoCatalog, type CatalogEntry } from "@/data/aniimoCatalog";
import {
  aniimoRoster,
  type AniimoBuild,
  type AniimoRole,
  type Element,
} from "@/data/irisalisParty";

export const LIBRARY_KEY = "aniimo-build-library-v1";

export const ANIIMO_ROLES: AniimoRole[] = ["DPS", "Break", "Heal", "Regen", "Support"];

export const ANIIMO_ELEMENTS: Element[] = [
  "Grass",
  "Fire",
  "Water",
  "Wind",
  "Earth",
  "Ice",
  "Lightning",
  "Dark",
  "Light",
];

const ACCENTS = [
  "#6fbf6a",
  "#7ec8e3",
  "#8b7355",
  "#d4786a",
  "#6b5b95",
  "#5aa9e6",
  "#e8a0b0",
  "#f3d5a0",
  "#9ec9c4",
  "#c45d78",
];

export type LibraryRow = {
  key: string;
  name: string;
  number: string;
  role?: AniimoRole;
  build: AniimoBuild | null;
  catalog: CatalogEntry | null;
};

export function seedBuilds(): AniimoBuild[] {
  return Object.values(aniimoRoster);
}

export function seedIds(): Set<string> {
  return new Set(seedBuilds().map((build) => build.id));
}

export function normName(name: string) {
  return name.toLowerCase().replace(/[^a-z0-9]/g, "");
}

export function slugId(name: string) {
  const slug = name
    .toLowerCase()
    .trim()
    .replace(/[^a-z0-9]+/g, "-")
    .replace(/^-|-$/g, "");
  return slug || "aniimo";
}

export function accentFor(name: string) {
  let hash = 0;
  for (const char of name) hash = (hash * 31 + char.charCodeAt(0)) >>> 0;
  return ACCENTS[hash % ACCENTS.length];
}

function isRole(value: unknown): value is AniimoRole {
  return typeof value === "string" && ANIIMO_ROLES.includes(value as AniimoRole);
}

function isElement(value: unknown): value is Element {
  return typeof value === "string" && ANIIMO_ELEMENTS.includes(value as Element);
}

export function isAniimoBuild(value: unknown): value is AniimoBuild {
  if (!value || typeof value !== "object") return false;
  const build = value as AniimoBuild;
  return (
    typeof build.id === "string" &&
    build.id.length > 0 &&
    typeof build.name === "string" &&
    build.name.trim().length > 0 &&
    isRole(build.role) &&
    Array.isArray(build.elements) &&
    build.elements.every(isElement) &&
    Array.isArray(build.skills) &&
    Array.isArray(build.attributes) &&
    build.attributes.length === 2 &&
    typeof build.attributes[0] === "string" &&
    typeof build.attributes[1] === "string" &&
    !!build.carriedItem &&
    typeof build.carriedItem.name === "string" &&
    Array.isArray(build.upgrades)
  );
}

export function mergeBuilds(saved: AniimoBuild[] | null | undefined): AniimoBuild[] {
  const byId = new Map(seedBuilds().map((build) => [build.id, build]));
  const knownSeeds = seedIds();
  for (const build of saved ?? []) {
    if (!isAniimoBuild(build)) continue;
    byId.set(build.id, build);
  }
  const seeds = seedBuilds().map((build) => byId.get(build.id) ?? build);
  const custom = [...byId.values()]
    .filter((build) => !knownSeeds.has(build.id))
    .sort((a, b) => a.name.localeCompare(b.name));
  return [...seeds, ...custom];
}

export function recordsToSave(builds: AniimoBuild[]): AniimoBuild[] {
  const seeds = new Map(seedBuilds().map((build) => [build.id, JSON.stringify(build)]));
  return builds.filter((build) => seeds.get(build.id) !== JSON.stringify(build));
}

export function libraryRows(builds: AniimoBuild[]): LibraryRow[] {
  const byName = new Map(builds.map((build) => [normName(build.name), build]));
  const used = new Set<string>();
  const rows: LibraryRow[] = aniimoCatalog.map((entry) => {
    const build = byName.get(normName(entry.name)) ?? null;
    if (build) used.add(build.id);
    return {
      key: `${entry.number}-${entry.name}`,
      name: entry.name,
      number: build?.number || entry.number,
      role: build?.role ?? entry.role,
      build,
      catalog: entry,
    };
  });

  for (const build of builds) {
    if (used.has(build.id)) continue;
    rows.push({
      key: build.id,
      name: build.name,
      number: build.number,
      role: build.role,
      build,
      catalog: null,
    });
  }

  return rows;
}

export function blankFromCatalog(entry: CatalogEntry, taken: Set<string>): AniimoBuild {
  let id = slugId(entry.name);
  let n = 2;
  while (taken.has(id)) {
    id = `${slugId(entry.name)}-${n}`;
    n += 1;
  }
  return {
    id,
    name: entry.name,
    number: entry.number,
    role: entry.role ?? "DPS",
    elements: [],
    stage: "",
    totalStats: entry.totalStats ?? 0,
    bestStat: "",
    trait: "",
    traitBlurb: "",
    skills: [],
    attributes: ["ATK", "HP"],
    carriedItem: {
      name: "",
      base: "",
      core: "",
      target: "Legendary",
      why: "",
    },
    upgrades: [],
    accent: accentFor(entry.name),
  };
}

export function blankCustom(taken: Set<string>): AniimoBuild {
  return blankFromCatalog({ number: "", name: "New Aniimo" }, taken);
}

export function lines(value: string) {
  return value
    .split("\n")
    .map((line) => line.trim())
    .filter(Boolean);
}

export function parseLibraryFile(raw: string): { builds: AniimoBuild[]; skipped: number } {
  const data = JSON.parse(raw) as unknown;
  const list = Array.isArray(data)
    ? data
    : data && typeof data === "object" && Array.isArray((data as { builds?: unknown }).builds)
      ? (data as { builds: unknown[] }).builds
      : null;
  if (!list) throw new Error("Expected a builds array.");
  const builds = list.filter(isAniimoBuild);
  return { builds, skipped: list.length - builds.length };
}
