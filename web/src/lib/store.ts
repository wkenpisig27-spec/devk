import { promises as fs } from "fs";
import path from "path";

const dataDir = process.env.DATA_DIR || path.join(process.cwd(), "data");

export type PreregRecord = {
  id: string;
  email: string;
  username: string;
  firstJob: string;
  intent: string;
  faction: string;
  referralCode: string;
  referredBy: string | null;
  createdAt: string;
  userAgent?: string;
};

export type AnalyticsEvent = {
  id: string;
  name: string;
  path?: string;
  meta?: Record<string, string>;
  createdAt: string;
};

type StoreFile<T> = { items: T[] };

async function ensureDir() {
  await fs.mkdir(dataDir, { recursive: true });
}

async function readJson<T>(file: string, fallback: StoreFile<T>): Promise<StoreFile<T>> {
  await ensureDir();
  try {
    const raw = await fs.readFile(file, "utf8");
    return JSON.parse(raw) as StoreFile<T>;
  } catch {
    return fallback;
  }
}

async function writeJson<T>(file: string, data: StoreFile<T>) {
  await ensureDir();
  const tmp = `${file}.tmp`;
  await fs.writeFile(tmp, JSON.stringify(data, null, 2), "utf8");
  await fs.rename(tmp, file);
}

function preregFile() {
  return path.join(dataDir, "preregs.json");
}

function eventsFile() {
  return path.join(dataDir, "events.json");
}

function makeId() {
  return `${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`;
}

function makeReferralCode() {
  const alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
  let out = "";
  for (let i = 0; i < 6; i++) out += alphabet[Math.floor(Math.random() * alphabet.length)];
  return out;
}

export async function getPreregCount(): Promise<number> {
  const store = await readJson<PreregRecord>(preregFile(), { items: [] });
  return store.items.length;
}

export async function findPreregByEmail(email: string) {
  const store = await readJson<PreregRecord>(preregFile(), { items: [] });
  return store.items.find((p) => p.email.toLowerCase() === email.toLowerCase()) ?? null;
}

export async function findPreregByUsername(username: string) {
  const store = await readJson<PreregRecord>(preregFile(), { items: [] });
  return store.items.find((p) => p.username.toLowerCase() === username.toLowerCase()) ?? null;
}

export async function findPreregByReferral(code: string) {
  const store = await readJson<PreregRecord>(preregFile(), { items: [] });
  return store.items.find((p) => p.referralCode === code.toUpperCase()) ?? null;
}

export async function createPrereg(input: {
  email: string;
  username: string;
  firstJob: string;
  intent: string;
  faction: string;
  referredBy: string | null;
  userAgent?: string;
}): Promise<PreregRecord> {
  const store = await readJson<PreregRecord>(preregFile(), { items: [] });

  let referralCode = makeReferralCode();
  while (store.items.some((p) => p.referralCode === referralCode)) {
    referralCode = makeReferralCode();
  }

  const record: PreregRecord = {
    id: makeId(),
    email: input.email.trim().toLowerCase(),
    username: input.username.trim(),
    firstJob: input.firstJob,
    intent: input.intent,
    faction: input.faction,
    referralCode,
    referredBy: input.referredBy,
    createdAt: new Date().toISOString(),
    userAgent: input.userAgent,
  };

  store.items.push(record);
  await writeJson(preregFile(), store);
  return record;
}

export async function getPreregStats() {
  const store = await readJson<PreregRecord>(preregFile(), { items: [] });
  const byJob: Record<string, number> = {};
  const byIntent: Record<string, number> = {};
  const byFaction: Record<string, number> = {};
  let referrals = 0;

  for (const p of store.items) {
    byJob[p.firstJob] = (byJob[p.firstJob] ?? 0) + 1;
    byIntent[p.intent] = (byIntent[p.intent] ?? 0) + 1;
    byFaction[p.faction] = (byFaction[p.faction] ?? 0) + 1;
    if (p.referredBy) referrals += 1;
  }

  return {
    count: store.items.length,
    referrals,
    byJob,
    byIntent,
    byFaction,
  };
}

export async function trackEvent(input: {
  name: string;
  path?: string;
  meta?: Record<string, string>;
}) {
  const store = await readJson<AnalyticsEvent>(eventsFile(), { items: [] });
  store.items.push({
    id: makeId(),
    name: input.name,
    path: input.path,
    meta: input.meta,
    createdAt: new Date().toISOString(),
  });
  // Keep the last 20k events so the file stays small
  if (store.items.length > 20000) {
    store.items = store.items.slice(-20000);
  }
  await writeJson(eventsFile(), store);
}
