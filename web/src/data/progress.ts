export type ProgressItem = {
  date: string;
  title: string;
  summary: string;
  details: string[];
  tag: "look" | "world" | "crew" | "fair-play" | "quality";
};

export type ProgressChapter = {
  id: string;
  period: string;
  headline: string;
  intro: string;
  items: ProgressItem[];
};

export const chapters: ProgressChapter[] = [
  {
    id: "jul-2026",
    period: "July 2026",
    headline: "Ascaron starts to look alive again",
    intro:
      "The world got color, characters got a clean silhouette, and parties got tools that actually help in the field.",
    items: [
      {
        date: "2026-07-23",
        title: "Grass, glyphs, and markers you can actually see",
        summary: "The ground looks richer, chat is easier to read, and quest marks no longer vanish in bright light.",
        details: [
          "Fresh grass across the fields so the islands feel less washed-out.",
          "Outlined fonts so names, chat, and UI stay sharp in a fight.",
          "Move and quest markers stay bright instead of disappearing into the scenery.",
          "Channel tags in chat no longer crash into the first word of your message.",
        ],
        tag: "look",
      },
      {
        date: "2026-07-23",
        title: "Outlines that hug your character — not the whole forest",
        summary: "Zoom in, zoom out: your crew stays readable. Trees and rocks stay clean.",
        details: [
          "Cel outlines follow bodies, not every crate and palm on the map.",
          "Line thickness scales with the camera, so close-ups do not turn into ink blobs.",
        ],
        tag: "look",
      },
      {
        date: "2026-07-18",
        title: "A classic 2.5D look, with water that moves",
        summary: "Richer scenery, smoother edges, and an ocean that finally feels like an ocean.",
        details: [
          "The sea renders with more life — less cardboard, more tide.",
          "Colors in the world are deeper without blowing out your UI.",
          "A readability-first look inspired by the 2.5D MMOs a lot of us grew up on.",
        ],
        tag: "look",
      },
      {
        date: "2026-07-18",
        title: "Party portals and a power score that means something",
        summary: "Walk through a door only your party can see. Compare strength on a number you can trust.",
        details: [
          "Drop a portal your party can actually find — no more “I clicked it, where are you?”",
          "Changing maps with a group is less likely to strand someone on the dock.",
          "Combat power is saved with your character, ready for real rankings.",
          "Creating an account is simpler: sign up and get sailing without extra hoop-jumps.",
        ],
        tag: "crew",
      },
      {
        date: "2026-07-02",
        title: "Stalls, boats, and logouts that do not eat your stuff",
        summary: "Leaving the world — or the trade window — should not strand gold, passengers, or a half-sold stall.",
        details: [
          "Offline shops clean up properly when you return: sold items gone, gold waiting.",
          "Trades and boat rides tear down cleanly instead of leaving ghosts on the map.",
          "When the world has to restart, you get a clean kick — not a silent freeze.",
        ],
        tag: "quality",
      },
      {
        date: "2026-07-02",
        title: "The mall is back. Whispers work. Your lock still holds.",
        summary: "Shop, private chat, and item locks behave the way you expect again.",
        details: [
          "The in-game shop is open after the connection work that had it dark.",
          "Second password and item unlock agree with each other — no more “shop says no, bag says yes.”",
          "Private messages and island lookups were fixed so crews can actually find each other.",
        ],
        tag: "quality",
      },
    ],
  },
  {
    id: "jun-2026",
    period: "June 2026",
    headline: "The connection you feel, not the one you debug",
    intro:
      "Login, character select, and stepping into the world had to stay playable while we made the seas harder to knock over.",
    items: [
      {
        date: "2026-07-01",
        title: "You stay you when the harbor is packed",
        summary: "Busy logins and map changes are less likely to swap you with a ghost or drop you in the void.",
        details: [
          "Your session is tied to you — not a lucky leftover from someone who just disconnected.",
          "Entering the world after character select is more reliable.",
          "Friend and party lists come back with you instead of going blank.",
        ],
        tag: "fair-play",
      },
      {
        date: "2026-06-27",
        title: "Junk traffic hits the dock. You hit the game.",
        summary: "Bad or truncated junk is turned away. Honest login, select, and enter-world still work.",
        details: [
          "Garbage connections are refused instead of taking the harbor down with them.",
          "The path from login screen to standing in town was walked and verified.",
          "The front door stays shut to junk even when something behind the scenes is mis-set.",
        ],
        tag: "fair-play",
      },
      {
        date: "2026-06-26",
        title: "Fewer freezes when someone hammers the gate",
        summary: "Rate limits, healthier keepalives, and traffic that is harder to tamper with.",
        details: [
          "Connection floods get throttled before they ruin everyone else’s login.",
          "The client and world check in more reliably, so fewer mystery AFK kicks.",
        ],
        tag: "fair-play",
      },
      {
        date: "2026-06-26",
        title: "First pass of the new look",
        summary: "Cel shading and shadows landed — the style July later turned into a full preset.",
        details: [
          "Characters and scenes picked up that comic-adventure edge.",
          "Lighting started doing real work instead of flattening every island.",
        ],
        tag: "look",
      },
      {
        date: "2026-06-25",
        title: "Monsters hunt again. You can log in.",
        summary: "Quiet maps got their AI back, and the login door stopped eating good accounts.",
        details: [
          "Mobs aggro and path the way they should — hunts feel like hunts.",
          "A valid login gets you in instead of bouncing for no good reason.",
          "A wave of stability and anti-cheat hardening shipped with that restore.",
        ],
        tag: "world",
      },
    ],
  },
  {
    id: "earlier-2026",
    period: "Earlier 2026",
    headline: "The bones of a real private world",
    intro:
      "Before the new look, the big promises were already on the table: sell while you are offline, play on a modern client, and keep the seas running overnight.",
    items: [
      {
        date: "2026-02-12",
        title: "Offline stalls",
        summary: "Set prices, log off, and let the stand work the night shift.",
        details: [
          "A stand-in holds your shop in the world while you are away.",
          "Come back to gold in the purse and a bag that matches what sold.",
        ],
        tag: "crew",
      },
      {
        date: "2026-01-27",
        title: "Built for today’s machines — and for staying online",
        summary: "A 64-bit client, a modern graphics path, and a world that can live on a real host.",
        details: [
          "The game runs as a modern 64-bit client instead of a relic from another decade.",
          "The seas are hosted the way a live world should be: dedicated, watched, restartable.",
          "Bigger bags, guilds, news, and the economy tools players actually use are in the plan.",
        ],
        tag: "quality",
      },
      {
        date: "2026-01-06",
        title: "The new coat of paint begins",
        summary: "We left the ancient graphics path behind so the world could look like 2026 without losing PKO’s soul.",
        details: [
          "The client moved onto a graphics path that modern PCs and drivers still respect.",
          "Everything you see in the July look — water, outlines, color — stands on this switch.",
        ],
        tag: "look",
      },
    ],
  },
];

export const latest = chapters[0].items.slice(0, 4);
