export type Feature = {
  id: string;
  title: string;
  blurb: string;
  body: string;
  status: "live" | "in-progress" | "coming";
  pillar: "world" | "combat" | "social" | "quality" | "trust";
};

export const features: Feature[] = [
  {
    id: "ascaron",
    title: "The world of Ascaron",
    blurb: "Navy in Argent. Pirates in Shaitan. Open water in between.",
    body: "Two flags, harbor towns, and a world built for sailing, hunting, and guild war. The fantasy is the one you remember — brought forward so it still feels right on a modern PC.",
    status: "live",
    pillar: "world",
  },
  {
    id: "classes",
    title: "Every craft, every path",
    blurb: "Newbie to Crusader, Champion, Sharpshooter, Cleric, Seal Master, Voyager, and more.",
    body: "Start green, pick your first craft at 10, then earn the advanced class through quests — not a cash shop. Land fighters, sea casters, merchants, and captains all have a real place in the world.",
    status: "live",
    pillar: "combat",
  },
  {
    id: "naval",
    title: "Ships, coral, and coin",
    blurb: "Fog the fleet, duel on the waves, and chase harbor prices.",
    body: "This was never only a land grind. Weave whirlpools as a Voyager, lead from the helm as a Captain, and live on the price boards of every port. Fishing, gathering, and woodcutting still pay if you put in the hours.",
    status: "live",
    pillar: "world",
  },
  {
    id: "visuals",
    title: "A world that pops",
    blurb: "Bold outlines, richer color, sharp text, and seas that actually look like water.",
    body: "Characters read like a classic 2.5D adventure: clean silhouettes when you zoom, scenery with real color, fonts you can read in a fight, and an ocean that moves. Trees and rocks stay clean — no muddy black edges swallowing the map.",
    status: "live",
    pillar: "quality",
  },
  {
    id: "stalls",
    title: "Sell while you sleep",
    blurb: "Plant a stall, log off, and wake up to gold.",
    body: "Your shop keeps running after you leave. Other players buy from a stand-in at your spot. When you return, sold goods are gone, gold is waiting, and your bag is ready to restock.",
    status: "live",
    pillar: "social",
  },
  {
    id: "portals",
    title: "Party portals",
    blurb: "Open a door only your party can see — and walk through together.",
    body: "No more screaming coordinates in chat while half the crew clicks the wrong rock. Drop a portal your party can actually use for hunts, map hops, and those “meet me in ten seconds” moments.",
    status: "live",
    pillar: "social",
  },
  {
    id: "combat-power",
    title: "Combat power",
    blurb: "One number that shows how hard you hit. Built for real rankings.",
    body: "Gear, stats, and class add up to a score you can trust. Compare crews, chase the board, and know whether that duel is courage or a bad idea.",
    status: "live",
    pillar: "combat",
  },
  {
    id: "inventory",
    title: "Room for the haul",
    blurb: "Bigger bags for fishers, crafters, and anyone who hates the warehouse trip.",
    body: "The old pack filled up before the fun started. Extra slots mean you can hunt, stall, and trade without living in storage.",
    status: "live",
    pillar: "quality",
  },
  {
    id: "launcher",
    title: "Play in one click",
    blurb: "Updates, repairs, and news sit next to the Play button.",
    body: "Launch, patch, and go. If a file goes missing, repair it from the same window. You should not need a forum thread to stay on the current client.",
    status: "live",
    pillar: "quality",
  },
  {
    id: "linux",
    title: "Seas that stay up",
    blurb: "A world built to run all night — not a box under someone’s desk.",
    body: "The game lives on dedicated servers meant to stay online. Map changes, logins, and busy harbors are built to keep your session instead of dropping you mid-sail.",
    status: "live",
    pillar: "trust",
  },
  {
    id: "network",
    title: "Cleaner fights, safer logins",
    blurb: "Less rubber-banding. Fewer mystery disconnects. Cheaper tricks get less room.",
    body: "Connections are tighter, logins are checked properly, and junk traffic is turned away before it ruins your evening. You still play the same game — it just holds together when the harbor is packed.",
    status: "live",
    pillar: "trust",
  },
  {
    id: "proxy",
    title: "Guarded gates",
    blurb: "Real players get in. Floods and bots get the plank.",
    body: "We are putting extra watch on the front door so login stays smooth when someone tries to knock the world over. Honest crews should feel nothing except a world that stays reachable.",
    status: "in-progress",
    pillar: "trust",
  },
  {
    id: "autopath",
    title: "Click the quest. Walk there.",
    blurb: "Mission text, NPC talk, and the map should all send you to the same place.",
    body: "No more guessing which red line is the actual destination. We are making auto-path consistent so a clicked objective takes you there — from the quest log and from conversation.",
    status: "in-progress",
    pillar: "quality",
  },
  {
    id: "guild-war",
    title: "Guilds, fame, and flags",
    blurb: "Crew up. Hold ground. Make cities remember your name.",
    body: "Guilds, friends, marriages, and fame still matter. Navy and Pirate standing changes who talks to you and which doors open. Territory is the long game.",
    status: "live",
    pillar: "social",
  },
  {
    id: "pets",
    title: "Fairies, rebirth, events",
    blurb: "Pets at your side, a second life, and seas that change with the season.",
    body: "The extras that made PKO feel alive are staying — not stripped out for a “clean” server. Same loop you loved, with fewer crashes and a client that looks awake.",
    status: "coming",
    pillar: "world",
  },
];

export const pillars = [
  { id: "world", label: "The adventure" },
  { id: "combat", label: "How you fight" },
  { id: "social", label: "Your crew" },
  { id: "quality", label: "How it feels" },
  { id: "trust", label: "A world you can trust" },
] as const;
