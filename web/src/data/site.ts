export const site = {
  name: "Abyss-Sea Online",
  shortName: "Abyss-Sea",
  tagline: "The classic pirate MMORPG, rebuilt for a new generation.",
  description:
    "Abyss-Sea Online is Pirates King Online brought back to life — Navy or Pirate, land or sea, classes, ships, and trade — looking sharper, running smoother, and waiting for your crew.",
  discord: "https://discord.com/invite/NHmFsuMpS5",
  engine: "devk",
  /** Public site origin. Override with NEXT_PUBLIC_SITE_URL in production. */
  url: process.env.NEXT_PUBLIC_SITE_URL ?? "http://157.245.52.152",
} as const;

export const nav = [
  { href: "/features", label: "Features" },
  { href: "/progress", label: "News" },
  { href: "/classes", label: "Classes" },
  { href: "/guides/pko-classes", label: "Guide" },
] as const;

export const firstJobOptions = [
  { id: "swordsman", label: "Swordsman" },
  { id: "hunter", label: "Hunter" },
  { id: "herbalist", label: "Herbalist" },
  { id: "explorer", label: "Explorer" },
] as const;

export const intentOptions = [
  { id: "yes", label: "Yes — I will play at launch" },
  { id: "maybe", label: "Maybe — watching for now" },
  { id: "watching", label: "Just browsing" },
] as const;

export const factionOptions = [
  { id: "navy", label: "Navy" },
  { id: "pirate", label: "Pirate" },
  { id: "undecided", label: "Not sure yet" },
] as const;

export const kit = [
  {
    qty: "2×",
    name: "Party EXP Fruit",
    note: "Extra experience while you are in a crew.",
    icon: "fruit",
  },
  {
    qty: "2×",
    name: "Apple of Youth",
    note: "A little help getting started on the islands.",
    icon: "apple",
  },
  {
    qty: "10×",
    name: "HP Potion",
    note: "Keeps you standing in the first hunts.",
    icon: "hp",
  },
  {
    qty: "10×",
    name: "SP Potion",
    note: "For skills when the bag is still empty.",
    icon: "sp",
  },
] as const;

export const preregSteps = [
  {
    n: "1",
    title: "Claim your spot below.",
    body: "Email, desired username, first job, and whether you will sail at launch. That is how we hold your name and know who is serious.",
  },
  {
    n: "2",
    title: "Join the Discord.",
    body: "Same crew, patch talk, and the opening bell. Bring friends with your referral link for extra kit credit later.",
  },
  {
    n: "3",
    title: "Log in on opening day.",
    body: "Nothing extra to create. Your reserved name and starter kit are waiting.",
  },
  {
    n: "4",
    title: "Claim the kit in Argent City.",
    body: "Harbourmaster Sella holds it. Claim it on the character you mean to keep — the kit is bound, so that choice is final.",
  },
] as const;
