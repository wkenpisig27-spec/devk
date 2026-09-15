export const site = {
  name: "Abyss-Sea Online",
  shortName: "Abyss-Sea",
  tagline: "The classic pirate MMORPG, rebuilt for a new generation.",
  description:
    "Abyss-Sea Online is Pirates King Online brought back to life — Navy or Pirate, land or sea, classes, ships, and trade — looking sharper, running smoother, and waiting for your crew.",
  discord: "https://discord.com/invite/NHmFsuMpS5",
  engine: "devk",
} as const;

export const nav = [
  { href: "/features", label: "Features" },
  { href: "/progress", label: "News" },
  { href: "/classes", label: "Classes" },
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
    title: "Join the Discord.",
    body: "That is how we know you are a real sailor, and how you hear the opening bell. No mailing list. No extra forms.",
  },
  {
    n: "2",
    title: "Claim your name with the crew.",
    body: "Tell us the username you want to log in with. When account creation opens, that name is the one we hold for you — not a waitlist we forget.",
  },
  {
    n: "3",
    title: "Log in on opening day.",
    body: "There is nothing extra to create on the day. Your account is waiting, and the starter kit is already tied to it.",
  },
  {
    n: "4",
    title: "Claim the kit in Argent City.",
    body: "Harbourmaster Sella holds it. Claim it on the character you mean to keep — the kit is bound, so that choice is final.",
  },
] as const;
