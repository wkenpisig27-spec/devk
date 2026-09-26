export type AniimoRole = "DPS" | "Break" | "Heal" | "Regen" | "Support";

export type Element =
  | "Grass"
  | "Wind"
  | "Water"
  | "Ice"
  | "Earth"
  | "Dark"
  | "Fire";

export type CarriedItem = {
  name: string;
  base: string;
  core: string;
  target: "Legendary";
  why: string;
};

export type AltItem = {
  name: string;
  why: string;
};

export type BaseStats = {
  hp: number;
  atk: number;
  break: number;
  pdef: number;
  mdef: number;
  regen: number;
};

export type AniimoBuild = {
  id: string;
  name: string;
  number: string;
  role: AniimoRole;
  elements: Element[];
  stage: string;
  totalStats: number;
  bestStat: string;
  trait: string;
  traitBlurb: string;
  skills: string[];
  attributes: [string, string];
  carriedItem: CarriedItem;
  altItem?: AltItem;
  stats?: BaseStats;
  rotation?: string[];
  upgrades: string[];
  locked?: boolean;
  accent: string;
};

export type PartyPreset = {
  id: string;
  name: string;
  tagline: string;
  score: string;
  slots: [string, string, string, string];
  playstyle: string;
};

export const aniimoRoster: Record<string, AniimoBuild> = {
  irisalis: {
    id: "irisalis",
    name: "Irisalis",
    number: "013",
    role: "DPS",
    elements: ["Grass"],
    stage: "Nova · Prismana",
    totalStats: 540,
    bestStat: "ATK 130",
    trait: "Bloom Cluster",
    traitBlurb:
      "Each clone hit banks 1 Dance Power. At 9 stacks the next skill costs 50% less EP and heals nearby allies for 8% of her max HP. Once every 180s she blooms again after fainting.",
    skills: ["Irisalis Shadow", "Whirling Blossom Rain", "Florae Descent"],
    attributes: ["ATK", "REGEN"],
    stats: { hp: 90, atk: 130, break: 56, pdef: 78, mdef: 78, regen: 108 },
    rotation: [
      "Irisalis Shadow — land Iris Shot so the clone fires a Floral Gleam Beam.",
      "Florae Descent when charged. That clone stays up beside the Shadow clone.",
      "Whirling Blossom Rain to command every clone at once.",
      "Finish a basic-attack chain so the clones keep beaming.",
      "At 9 Dance Power, spend the skill that costs 50% less EP.",
    ],
    carriedItem: {
      name: "Ferocious Fang",
      base: "Damage Amp +10% (Legendary)",
      core: "+0.7 ATK per Aniimo level",
      target: "Legendary",
      why: "In-game recommended held item. Push Legendary +15 — second energy unlocks another Damage Amp that doubles if ATK Acquired Potential exceeds 15.",
    },
    altItem: {
      name: "Giant Tortoise Shell",
      why: "Durability swap when the opening is dangerous and she needs time to set clones.",
    },
    upgrades: [
      "Level Irisalis first — Resonance star stage 6 wants Lv 60, stage 7 wants Lv 65.",
      "Spend attribute points ATK → REGEN every time.",
      "Enhance Ferocious Fang to Legendary +15 (Damage Amp stack + rune slots).",
      "Resonance star-ups: Iris Dewdrop Crystals + Sprout Stones; Phenomena Crystal ×1 at stage 6, ×2 at stage 7.",
      "Bank scarce crystals for late Resonance — do not scatter them on benches.",
      "Contract / Prismatic the clone kit (Irisalis Shadow) — Magical Clone Damage is the once-per-species spend.",
    ],
    locked: true,
    accent: "#6fbf6a",
  },
  somniwing: {
    id: "somniwing",
    name: "Somniwing",
    number: "031",
    role: "Regen",
    elements: ["Wind", "Grass"],
    stage: "Nova · Prismana",
    totalStats: 538,
    bestStat: "REGEN 115",
    trait: "Energy Full",
    traitBlurb: "Raises max EP by 10 so Irisalis can keep casting clone skills.",
    skills: ["Ethereal Light Wave", "Hypnotic Spores", "Butterfly Dance"],
    attributes: ["HP", "REGEN"],
    carriedItem: {
      name: "Spirited Feather",
      base: "REGEN +10% (Legendary)",
      core: "+0.7 REGEN per Aniimo level",
      target: "Legendary",
      why: "Primary in-game held item for the field partner.",
    },
    altItem: {
      name: "Auspicious Bell",
      why: "EP-luck swap when Irisalis is starving for skill casts.",
    },
    upgrades: [
      "Attribute order: HP first, REGEN second.",
      "Enhance Spirited Feather (or Auspicious Bell) to Legendary.",
      "Prioritize Ethereal Light Wave uses for research accessory slots.",
      "Keep her leveled with Irisalis — she is the in-game field partner.",
    ],
    accent: "#7ec8e3",
  },
  shrubclaw: {
    id: "shrubclaw",
    name: "Shrubclaw",
    number: "025",
    role: "Break",
    elements: ["Earth", "Grass"],
    stage: "Nova",
    totalStats: 550,
    bestStat: "DEF 109",
    trait: "Stealth",
    traitBlurb:
      "Builds Momentum on hits (doubled while tunneling). At 6 stacks, BREAK efficiency +30% for 20s.",
    skills: ["Charged Blow", "The Luminus Gust", "Quake Impact"],
    attributes: ["BREAK", "P.DEF"],
    carriedItem: {
      name: "Gargantuan Horn",
      base: "BREAK +10% (Legendary)",
      core: "+0.7 BREAK per Aniimo level",
      target: "Legendary",
      why: "Standard Break held item. Double payoff if BREAK Acquired Potential exceeds 15.",
    },
    upgrades: [
      "Attribute order: BREAK → P.DEF.",
      "Legendary Gargantuan Horn before luxury cosmetics.",
      "Tunnel to stack Momentum faster into the Break window for Irisalis.",
      "Research Charged Blow uses for accessory slots.",
    ],
    accent: "#8b7355",
  },
  coraliz: {
    id: "coraliz",
    name: "Coraliz",
    number: "085",
    role: "Regen",
    elements: ["Earth", "Water"],
    stage: "Nova",
    totalStats: 562,
    bestStat: "REGEN 108",
    trait: "Overcharged Coral",
    traitBlurb:
      "On battle entry, restores EP per Earth ally — strong with Shrubclaw on the squad.",
    skills: ["Coral Trap", "Coral Impact", "Coral Prison"],
    attributes: ["REGEN", "BREAK"],
    carriedItem: {
      name: "Spirited Feather",
      base: "REGEN +10% (Legendary)",
      core: "+0.7 REGEN per Aniimo level",
      target: "Legendary",
      why: "Keeps EP flowing so Irisalis never stalls Dance Power loops.",
    },
    upgrades: [
      "Attribute order: REGEN → BREAK.",
      "Legendary Spirited Feather.",
      "Pair with an Earth Breaker (Shrubclaw) to juice Overcharged Coral.",
      "Note: file-complete in 1.0.0.7; availability may vary by patch.",
    ],
    accent: "#d4786a",
  },
  helmut: {
    id: "helmut",
    name: "Helmut",
    number: "062",
    role: "Break",
    elements: ["Dark"],
    stage: "Lumin → evolve",
    totalStats: 339,
    bestStat: "DEF 68",
    trait: "Guardbreak Resonance",
    traitBlurb: "Scales BREAK from its own stats while Ballistic Guard soaks hits.",
    skills: ["Ballistic Guard", "Guardbreak Slam"],
    attributes: ["BREAK", "DEF"],
    carriedItem: {
      name: "Gargantuan Horn",
      base: "BREAK +10% (Legendary)",
      core: "+0.7 BREAK per Aniimo level",
      target: "Legendary",
      why: "In-game Break item. Evolve Helmut toward Pawney/Rookey for Nova power.",
    },
    upgrades: [
      "Evolve out of Lumin when materials allow — base stats jump hard.",
      "Legendary Gargantuan Horn.",
      "Keep Ballistic Guard up, then Guardbreak Slam into Irisalis burst.",
      "Attribute into BREAK and DEF so the shield stays sticky.",
    ],
    accent: "#6b5b95",
  },
  glacy: {
    id: "glacy",
    name: "Glacy",
    number: "016",
    role: "Heal",
    elements: ["Water", "Ice"],
    stage: "Nova",
    totalStats: 520,
    bestStat: "HP 118",
    trait: "Water Spirit",
    traitBlurb: "On water terrain, all skills cost 10% less EP.",
    skills: ["Healing Water", "Glimmer Shot"],
    attributes: ["HP", "M.DEF"],
    carriedItem: {
      name: "Miraculous Fleece",
      base: "Enhanced Healing +10% (Legendary)",
      core: "Overflow healing transfers to lowest-HP ally",
      target: "Legendary",
      why: "Best pure heal item. Spirited Feather is the REGEN fallback.",
    },
    upgrades: [
      "Attribute order: HP → M.DEF.",
      "Legendary Miraculous Fleece (or Spirited Feather).",
      "Keep Healing Water ready for clone-window mistakes.",
      "Research heal casts for accessory slots.",
    ],
    accent: "#5aa9e6",
  },
  panpanta: {
    id: "panpanta",
    name: "Panpanta",
    number: "053",
    role: "Break",
    elements: ["Water"],
    stage: "Nova",
    totalStats: 545,
    bestStat: "M.DEF 106",
    trait: "Appeal",
    traitBlurb:
      "With an opposite-sex Susuta-family mate, BREAK efficiency +30% for 15s on entry.",
    skills: ["Splash Shield", "Hydro Roll", "Water Cannon"],
    attributes: ["BREAK", "M.DEF"],
    carriedItem: {
      name: "Gargantuan Horn",
      base: "BREAK +10% (Legendary)",
      core: "+0.7 BREAK per Aniimo level",
      target: "Legendary",
      why: "Break amplifier while Splash Shield reflects extra hits for Irisalis.",
    },
    upgrades: [
      "Attribute order: BREAK → M.DEF.",
      "Legendary Gargantuan Horn.",
      "Open with Splash Shield, then Hydro Roll near water for BREAK efficiency.",
      "Bring Piopiota (same family) if you want the Appeal window.",
    ],
    accent: "#4a90c8",
  },
  piopiota: {
    id: "piopiota",
    name: "Piopiota",
    number: "054",
    role: "Support",
    elements: ["Water"],
    stage: "Nova",
    totalStats: 530,
    bestStat: "Support kit",
    trait: "Susuta lineage",
    traitBlurb: "Same-family Support that enables Panpanta Appeal and adds water pressure.",
    skills: ["Water Cannon", "Surge"],
    attributes: ["HP", "REGEN"],
    carriedItem: {
      name: "Spirited Feather",
      base: "REGEN +10% (Legendary)",
      core: "+0.7 REGEN per Aniimo level",
      target: "Legendary",
      why: "Keeps the water relay casting while Irisalis banks Dance Power.",
    },
    upgrades: [
      "Attribute into sustain (HP / REGEN) so the Support seat stays alive.",
      "Legendary Spirited Feather.",
      "Sync with Panpanta for Appeal uptime.",
      "Level with the rest of the squad — Support dies first if underleveled.",
    ],
    accent: "#3d7ea6",
  },
  leafy: {
    id: "leafy",
    name: "Leafy",
    number: "017",
    role: "Regen",
    elements: ["Grass", "Water"],
    stage: "Nova",
    totalStats: 530,
    bestStat: "HP 120",
    trait: "Power of Nature",
    traitBlurb:
      "Off-field, plants grass under a teammate after they cast 3 skills — free disco terrain for Irisalis.",
    skills: ["Nature sustain kit", "Grass field support"],
    attributes: ["HP", "REGEN"],
    carriedItem: {
      name: "Spirited Feather",
      base: "REGEN +10% (Legendary)",
      core: "+0.7 REGEN per Aniimo level",
      target: "Legendary",
      why: "Classic Irisal-line partner item. Grass patches buff Irisalis damage.",
    },
    upgrades: [
      "Attribute order: HP → REGEN.",
      "Legendary Spirited Feather.",
      "Let her sit off-field to seed grass for Irisalis Disco windows.",
      "Strong budget pick if you already raised Leafy for Irisal.",
    ],
    accent: "#5fad56",
  },
  turbo: {
    id: "turbo",
    name: "Turbo",
    number: "019",
    role: "Support",
    elements: ["Wind"],
    stage: "Nova",
    totalStats: 520,
    bestStat: "Haste 109",
    trait: "Shrouded in Mist",
    traitBlurb: "After 6 basics, enters mist — shields and tempo for the blossom carry.",
    skills: ["Cloudwalk utility", "Shield / tempo support"],
    attributes: ["M.DEF", "HP"],
    carriedItem: {
      name: "Auspicious Bell",
      base: "REGEN +10% (Legendary)",
      core: "20% chance for +5 EP on skill cast",
      target: "Legendary",
      why: "EP luck feeds Irisalis. Spirited Feather is the safe REGEN twin.",
    },
    upgrades: [
      "Attribute order: M.DEF → HP.",
      "Legendary Auspicious Bell or Spirited Feather.",
      "Use mist windows to cover Irisalis while clones fire.",
      "Research basic-attack → mist cycles for accessory slots.",
    ],
    accent: "#9ec9c4",
  },
};

export const partyPresets: PartyPreset[] = [
  {
    id: "bloom-core",
    name: "Bloom Core",
    tagline: "In-game partner spine",
    score: "A · 70",
    slots: ["irisalis", "somniwing", "shrubclaw", "coraliz"],
    playstyle:
      "Irisalis leads. Somniwing is the official field partner for EP. Shrubclaw cracks Break, Coraliz refunds EP for every Earth ally.",
  },
  {
    id: "guarded-blossom",
    name: "Guarded Blossom",
    tagline: "Shield · heal · regen",
    score: "S blueprint",
    slots: ["irisalis", "helmut", "glacy", "somniwing"],
    playstyle:
      "Helmut Ballistic Guard soaks, Glacy tops the party, Somniwing keeps EP high while Irisalis clones melt the target.",
  },
  {
    id: "tidal-relay",
    name: "Tidal Relay",
    tagline: "Water Break + Support",
    score: "S blueprint",
    slots: ["irisalis", "panpanta", "piopiota", "somniwing"],
    playstyle:
      "Panpanta Splash Shield + Hydro Roll opens Break. Piopiota enables Appeal. Somniwing fuels Irisalis skill loops.",
  },
  {
    id: "meadow-line",
    name: "Meadow Line",
    tagline: "Irisal graduates",
    score: "Budget",
    slots: ["irisalis", "leafy", "turbo", "somniwing"],
    playstyle:
      "If you already invested in the Iris → Irisal path, Leafy seeds grass and Turbo covers mist shields while Somniwing regenerates.",
  },
];

export const upgradePriority = [
  {
    title: "1 · Level Irisalis",
    body: "Push her level before dumping crystals. Resonance star stage 6 wants Lv 60; stage 7 wants Lv 65.",
  },
  {
    title: "2 · ATK then REGEN",
    body: "Every attribute point: ATK first, REGEN second. That is the in-game priority for Irisalis.",
  },
  {
    title: "3 · Ferocious Fang → Legendary +15",
    body: "Core +0.7 ATK per level. Legendary base Damage Amp +10%; at +15 the second energy adds another Damage Amp (doubles if ATK potential > 15).",
  },
  {
    title: "4 · Resonance star-ups",
    body: "Iris Dewdrop Crystals + Sprout Stones each stage. Phenomena Crystal ×1 at stage 6, ×2 at stage 7.",
  },
  {
    title: "5 · Hold scarce crystals",
    body: "Save Phenomena / late Resonance materials for Irisalis. Do not scatter them on benches.",
  },
  {
    title: "6 · Contract the clone kit",
    body: "Once-per-species spend — Irisalis Shadow Magical Clone Damage is the priority over side skills.",
  },
  {
    title: "7 · Partners after the carry",
    body: "Once Irisalis is online, Legendary held items on Somniwing and your Break seat, then Heal/Support.",
  },
] as const;

export const partnerPool = Object.values(aniimoRoster).filter((a) => !a.locked);
