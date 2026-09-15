export type SkillKind =
  | "sword"
  | "shield"
  | "rage"
  | "bow"
  | "gun"
  | "heal"
  | "bolt"
  | "poison"
  | "stealth"
  | "aoe"
  | "speed"
  | "ship"
  | "coral"
  | "trade"
  | "craft"
  | "totem"
  | "seal"
  | "buff";

export type Skill = {
  name: string;
  type: "passive" | "initiate";
  sp?: string;
  effect: string;
  icon: SkillKind;
  from?: string;
};

export type PlayClass = {
  id: string;
  name: string;
  tier: "base" | "advanced";
  from?: string;
  advancesTo: string[];
  style: string;
  weapon: string;
  blurb: string;
  portrait?: string;
  skills: Skill[];
};

const swordsmanSkills: Skill[] = [
  { name: "Concentration", type: "passive", effect: "+1 hit rate per level. You actually land the swing.", icon: "buff" },
  { name: "Taunt", type: "initiate", effect: "Force a single target to attack you. Frontliners live here.", icon: "shield" },
  { name: "Sword Mastery", type: "passive", effect: "+4 sword attack per level.", icon: "sword" },
  { name: "Will of Steel", type: "initiate", sp: "10 SP", effect: "+3 defense per level for 15 seconds.", icon: "shield" },
  { name: "Break Armor", type: "initiate", sp: "25 SP", effect: "−4 enemy defense per level for 15 seconds.", icon: "sword" },
  { name: "Illusion Slash", type: "initiate", sp: "20 SP", effect: "Ranged sword energy — close the gap without walking it.", icon: "sword" },
  { name: "Tiger Roar", type: "initiate", sp: "20 SP", effect: "AoE: −3 attack and −1% speed per level for 15 seconds.", icon: "aoe" },
  { name: "Berserk", type: "initiate", sp: "15 SP", effect: "Raise hit rate and attack speed when the fight turns ugly.", icon: "rage" },
];

const hunterSkills: Skill[] = [
  { name: "Range Mastery", type: "passive", effect: "+2 ranged attack per level.", icon: "bow" },
  { name: "Windwalk", type: "passive", effect: "+2% movement speed per level. Kite or die.", icon: "speed" },
  { name: "Dual Shot", type: "initiate", sp: "20 SP", effect: "Two hits: 1.7× damage at level 1, +15% per level.", icon: "bow" },
  { name: "Rousing", type: "initiate", sp: "35 SP", effect: "+11 attack speed at level 1, +1 per level, 15 seconds.", icon: "speed" },
  { name: "Venom Arrow", type: "initiate", effect: "Poison: 12 damage/sec at level 1, +2 per level, 20 seconds.", icon: "poison" },
  { name: "Frozen Arrow", type: "initiate", sp: "15 SP", effect: "Damage plus a slow. Peel runners off the healer.", icon: "bow" },
  { name: "Meteor Shower", type: "initiate", sp: "46 SP", effect: "AoE arrows: 60% damage at level 1, +10% per level.", icon: "aoe" },
];

const herbalistSkills: Skill[] = [
  { name: "Heal", type: "initiate", effect: "Restore HP. Scales with STA, and you earn XP for keeping people alive.", icon: "heal" },
  { name: "Spiritual Bolt", type: "initiate", effect: "Magic damage that scales with STA. You are not only a bandage.", icon: "bolt" },
  { name: "Harden", type: "initiate", effect: "+14 defense at level 1, +4 per level, 180 seconds.", icon: "shield" },
  { name: "Spiritual Fire", type: "initiate", effect: "+11% attack at level 1, +1% per level, 180 seconds.", icon: "buff" },
  { name: "Recover", type: "initiate", effect: "Strip abnormal status. The difference between a wipe and a story.", icon: "heal" },
  { name: "Tempest Boost", type: "initiate", effect: "+6% attack speed at level 1, +1% per level, 138 seconds.", icon: "speed" },
  { name: "Revival", type: "initiate", effect: "Raise a fallen ally. Needs a Revival Clover.", icon: "heal" },
  { name: "Vigor", type: "passive", effect: "+40 max SP per level. More casts before you go quiet.", icon: "buff" },
];

const explorerSkills: Skill[] = [
  { name: "Diligence", type: "passive", effect: "+2 SP recovery at level 1, +1 per level.", icon: "buff" },
  { name: "Current", type: "passive", effect: "+6% ship movement speed at level 1. The sea is the map.", icon: "ship" },
  { name: "Lightning Bolt", type: "initiate", effect: "Thunder Coral strike. Scales with STA.", icon: "coral" },
  { name: "Conch Armor", type: "passive", effect: "+8 ship defense at level 1, +3 per level.", icon: "shield" },
  { name: "Tornado", type: "initiate", effect: "Wind Coral knockup, 3.5 seconds at level 1.", icon: "aoe" },
  { name: "Alga Entanglement", type: "initiate", effect: "Bind plus 5 damage/sec for 6 seconds at level 1.", icon: "coral" },
];

function tagged(skills: Skill[], from: string): Skill[] {
  return skills.map((s) => ({ ...s, from }));
}

export const playClasses: PlayClass[] = [
  {
    id: "swordsman",
    name: "Swordsman",
    tier: "base",
    advancesTo: ["Crusader", "Champion"],
    style: "Close-range melee and the front of every party.",
    weapon: "Sword and shield, or dual blades",
    blurb:
      "You stand in the teeth of the pack. Taunt, break armor, and keep the healer breathing. At the next class quest you choose speed (Crusader) or raw power (Champion).",
    portrait: "/classes/swordsman.png",
    skills: swordsmanSkills,
  },
  {
    id: "hunter",
    name: "Hunter",
    tier: "base",
    advancesTo: ["Sharpshooter"],
    style: "Long range, high burst, thin as paper.",
    weapon: "Bow or gun",
    blurb:
      "You never wanted to be hugged. Kite, poison, and rain arrows. The Sharpshooter quest is the gun path — silence, cripple, and a headshot that ignores armor.",
    portrait: "/classes/hunter.png",
    skills: hunterSkills,
  },
  {
    id: "herbalist",
    name: "Herbalist",
    tier: "base",
    advancesTo: ["Cleric", "Seal Master"],
    style: "Heals, bolts, and the buffs that win long fights.",
    weapon: "Staff",
    blurb:
      "STA is your weapon. Keep HP up, strip poison, and revive when someone gets greedy. Cleric doubles down on the party. Seal Master makes the other party wish they stayed home.",
    portrait: "/classes/herbalist.png",
    skills: herbalistSkills,
  },
  {
    id: "explorer",
    name: "Explorer",
    tier: "base",
    advancesTo: ["Voyager"],
    style: "Coral elements and the open sea.",
    weapon: "Coral",
    blurb:
      "Land is a rumor. Lightning, wind, and bind from the deck. Voyager unlocks the full coral set: fog, whirlpool, curtains of thunder.",
    portrait: "/classes/explorer.png",
    skills: explorerSkills,
  },
  {
    id: "crusader",
    name: "Crusader",
    tier: "advanced",
    from: "Swordsman",
    advancesTo: [],
    style: "Dual swords, stealth, poison — speed over bulk.",
    weapon: "Twin blades",
    blurb:
      "You stop being a wall and become a knife. Stealth in, poison, knock them down, vanish. Still a Swordsman underneath — the old kit does not go away.",
    portrait: "/classes/crusader.png",
    skills: [
      ...tagged(swordsmanSkills, "Swordsman"),
      { name: "Dual Sword Mastery", type: "passive", effect: "+8% left-hand attack per level.", icon: "sword", from: "Crusader" },
      { name: "Deftness", type: "passive", effect: "+3 dodge per level. You are not there when they swing.", icon: "speed", from: "Crusader" },
      { name: "Blood Frenzy", type: "passive", effect: "−10% dual-weapon cooldown at level 1, −2% per level.", icon: "rage", from: "Crusader" },
      { name: "Stealth", type: "initiate", effect: "Go invisible. Drains SP while you hunt.", icon: "stealth", from: "Crusader" },
      { name: "Poison Dart", type: "initiate", effect: "12 damage/sec at level 1, +2 per level, 17 seconds.", icon: "poison", from: "Crusader" },
      { name: "Shadow Slash", type: "initiate", effect: "Melee plus knockout: 110% attack at level 1, +10% per level.", icon: "sword", from: "Crusader" },
    ],
  },
  {
    id: "champion",
    name: "Champion",
    tier: "advanced",
    from: "Swordsman",
    advancesTo: [],
    style: "Greatsword, totems, and the whole pack on you.",
    weapon: "Greatsword",
    blurb:
      "You plant a totem and become the boss fight. More HP, more roar, a strike that asks for that totem and pays 3.5×.",
    portrait: "/classes/champion.png",
    skills: [
      ...tagged(swordsmanSkills, "Swordsman"),
      { name: "Greatsword Mastery", type: "passive", effect: "+7 greatsword attack per level.", icon: "sword", from: "Champion" },
      { name: "Roar", type: "initiate", effect: "AoE taunt. Several monsters, one problem: you.", icon: "aoe", from: "Champion" },
      { name: "Strengthen", type: "passive", effect: "+20 max HP per level.", icon: "buff", from: "Champion" },
      { name: "Blood Bull", type: "passive", effect: "Totem: +10% HP and defense at level 1, +2% per level.", icon: "totem", from: "Champion" },
      { name: "Mighty Strike", type: "initiate", sp: "8 SP", effect: "1.25× damage at level 1, +5% per level.", icon: "sword", from: "Champion" },
      { name: "Howl", type: "initiate", effect: "AoE: 1.05× damage at level 1, +5% per level.", icon: "aoe", from: "Champion" },
      { name: "Primal Rage", type: "initiate", sp: "53 SP", effect: "Needs the totem. 3.5× damage at level 1. The button.", icon: "rage", from: "Champion" },
    ],
  },
  {
    id: "sharpshooter",
    name: "Sharpshooter",
    tier: "advanced",
    from: "Hunter",
    advancesTo: [],
    style: "Guns, silence, and a shot that skips armor.",
    weapon: "Firearm",
    blurb:
      "The bow was practice. Cripple their feet, shut their skills, melt the ground, then Headshot for max damage plus a chunk of their life.",
    portrait: "/classes/sharpshooter.png",
    skills: [
      ...tagged(hunterSkills, "Hunter"),
      { name: "Cripple", type: "initiate", effect: "−20% dodge, −50% speed, 5 seconds. They do not walk away.", icon: "gun", from: "Sharpshooter" },
      { name: "Firegun Mastery", type: "passive", effect: "+10 max and +6 min gun attack per level.", icon: "gun", from: "Sharpshooter" },
      { name: "Enfeeble", type: "initiate", effect: "−20% attack and 5 seconds of silence.", icon: "seal", from: "Sharpshooter" },
      { name: "Magma Bullet", type: "initiate", effect: "AoE fire: 33 damage/sec on the ground for 10 seconds.", icon: "aoe", from: "Sharpshooter" },
      { name: "Headshot", type: "initiate", effect: "Ignore defense. Max damage plus 5–10% of their max HP.", icon: "gun", from: "Sharpshooter" },
    ],
  },
  {
    id: "cleric",
    name: "Cleric",
    tier: "advanced",
    from: "Herbalist",
    advancesTo: [],
    style: "Party shields, auras, and a second chance.",
    weapon: "Staff",
    blurb:
      "You turn a scramble into a formation. Energy Shield eats hits as SP. Healing Spring covers the pile. Crystalline Blessing says not today.",
    portrait: "/classes/cleric.png",
    skills: [
      ...tagged(herbalistSkills, "Herbalist"),
      { name: "Divine Grace", type: "passive", effect: "+2 SP recovery per level.", icon: "buff", from: "Cleric" },
      { name: "True Sight", type: "initiate", effect: "Reveal stealthed units in an area. Crusaders hate this.", icon: "bolt", from: "Cleric" },
      { name: "Tornado Swirl", type: "initiate", effect: "+6% crit and berserk rate at level 1, 33 seconds.", icon: "buff", from: "Cleric" },
      { name: "Angelic Shield", type: "initiate", effect: "+3% defense at level 1, +3% per level, 33 seconds.", icon: "shield", from: "Cleric" },
      { name: "Energy Shield", type: "initiate", effect: "Convert incoming HP damage into SP damage.", icon: "shield", from: "Cleric" },
      { name: "Healing Spring", type: "initiate", effect: "AoE HP regen aura, 17 seconds. Plant it and don't move the pile.", icon: "heal", from: "Cleric" },
      { name: "Crystalline Blessing", type: "initiate", effect: "The target becomes immune to attacks. Spend it like gold.", icon: "buff", from: "Cleric" },
    ],
  },
  {
    id: "seal-master",
    name: "Seal Master",
    tier: "advanced",
    from: "Herbalist",
    advancesTo: [],
    style: "Debuffs, silence, and the other crew's worst minute.",
    weapon: "Seals",
    blurb:
      "You do not heal the fight — you cancel it. Defense down, no auto-attacks, no skills, stuck in the mire. Intense Magic makes your own bolts meaner.",
    portrait: "/classes/seal-master.png",
    skills: [
      ...tagged(herbalistSkills, "Herbalist"),
      { name: "Cursed Fire", type: "initiate", effect: "AoE: −12% enemy defense at level 1, −2% per level.", icon: "aoe", from: "Seal Master" },
      { name: "Shadow Insignia", type: "initiate", effect: "Stop normal attacks for 6 seconds at level 1.", icon: "seal", from: "Seal Master" },
      { name: "Abyss Mire", type: "initiate", effect: "AoE: −30% enemy speed for 20 seconds at level 1.", icon: "seal", from: "Seal Master" },
      { name: "Seal of Elder", type: "initiate", effect: "Block skill usage for 10.5 seconds at level 1.", icon: "seal", from: "Seal Master" },
      { name: "Intense Magic", type: "initiate", effect: "Boost magical damage. High levels can refund the cast.", icon: "bolt", from: "Seal Master" },
    ],
  },
  {
    id: "voyager",
    name: "Voyager",
    tier: "advanced",
    from: "Explorer",
    advancesTo: [],
    style: "Fog, whirlpool, and every coral at once.",
    weapon: "Coral arts",
    blurb:
      "Ship-to-ship is your raid. Line damage, thunderstorms, a tailwind for allies, fog that guts their attack, a whirlpool that steals their speed.",
    portrait: "/classes/voyager.png",
    skills: [
      ...tagged(explorerSkills, "Explorer"),
      { name: "Conch Ray", type: "initiate", effect: "Strike Coral: a line of damage across the water.", icon: "coral", from: "Voyager" },
      { name: "Lightning Curtain", type: "initiate", effect: "Thunder Coral: an AoE thunderstorm on their deck.", icon: "aoe", from: "Voyager" },
      { name: "Tail Wind", type: "initiate", effect: "Wind Coral: boost allied ship speed.", icon: "speed", from: "Voyager" },
      { name: "Fog", type: "initiate", effect: "Fog Coral: −attack on enemy ships in the cloud.", icon: "stealth", from: "Voyager" },
      { name: "Whirlpool", type: "initiate", effect: "Cut enemy ship movement in the swirl.", icon: "ship", from: "Voyager" },
    ],
  },
];

export const firstJobs = playClasses.filter((c) => c.tier === "base");

export function ownSkills(c: PlayClass): Skill[] {
  return c.skills.filter((s) => !s.from || s.from === c.name);
}

export function secondJobsOf(first: PlayClass): PlayClass[] {
  return playClasses.filter((c) => c.from === first.name);
}
