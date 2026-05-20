# Pirates King Online — Gameplay Reference

> Sourced from piratekings.online/database (scraped 2026-05-13) combined with
> server-side Lua scripts in `server/resource/script/`.  
> Use this document to verify devk's gameplay faithfully reproduces the original PKO experience.

---

## Table of Contents

1. [Game Overview](#game-overview)
2. [Character & Stats](#character--stats)
3. [Class System](#class-system)
4. [Skill System](#skill-system)
5. [Combat System](#combat-system)
6. [Status Effects](#status-effects)
7. [Quest System](#quest-system)
8. [World & Maps](#world--maps)
9. [NPC System](#npc-system)
10. [Monster Roster](#monster-roster)
11. [Naval & Ship System](#naval--ship-system)
12. [Economy & Trade](#economy--trade)
13. [Pet / Fairy System](#pet--fairy-system)
14. [Equipment & Crafting](#equipment--crafting)
15. [Guild & PvP](#guild--pvp)
16. [Rebirth System](#rebirth-system)
17. [Special Events](#special-events)

---

## Game Overview

Pirates King Online (PKO) is a 3D maritime MMORPG set in the fantasy world of Ascaron, split between the lawful **Navy** faction (based in Argent City) and the rebellious **Pirate** faction (based in Shaitan City). Players sail the open seas, battle monsters, advance through a branching class system, and engage in guild warfare for territorial control.

Key pillars:
- **Land combat** — level-based PvE against monsters and PvP
- **Naval combat** — ship-to-ship battles with coral skills
- **Trade/Commerce** — buy low, sell high across harbors for profit
- **Life skills** — fishing, gathering, woodcutting as supplemental income
- **Social systems** — guilds, parties, marriages, factions

---

## Character & Stats

### Primary Stats

| Stat | Abbrev | Affects |
|------|--------|---------|
| Strength | STR | Physical attack, carry weight |
| Agility | AGI | Movement speed, dodge |
| Dexterity | DEX | Attack speed, hit rate, physical damage |
| Constitution | CON | Max HP, defense |
| Stamina | STA | Max SP, HP/SP recovery, magic healing power |
| Luck | LUK | Critical hit rate, magic factor |

### Derived Stats (Newbie baseline formulas)

| Stat | Formula |
|------|---------|
| Max HP | `CON × 3 + Level × 15` |
| Max SP | `STA × 1 + Level × 3` |
| Physical ATK | `STR × 1.5 + DEX × 0.4` |
| Defense | `CON × 0.1` |
| Hit Rate | `DEX × 0.6` |
| Dodge/Flee | `AGI × 0.6` |
| Critical Rate | `LUK × 0.31` |
| Magic Factor | `LUK × 0.39` |

Each advanced class has unique multiplier coefficients for these formulas (defined in `server/resource/script/calculate/AttrCalculate.lua`).

### Damage Formula

```
Damage = Base + (ATK_Stat × Coefficient) ± 10% random variance
```

Physical resistance reduces incoming damage as a flat percentage. Defense reduces it as a flat value.

---

## Class System

### Tier 1 — Newbie (Levels 0–10)

All characters begin as **Newbie**. At level 10, they automatically advance to a Tier 1 class based on their choice at creation.

### Tier 1 Classes

| Class | Combat Style | Weapon | Advancement Options |
|-------|-------------|--------|---------------------|
| **Swordsman** | Close-range melee, tank | Sword + Shield or Dual Swords | Crusader, Champion |
| **Hunter** | Long-range, fragile | Bow or Gun | Sharpshooter |
| **Herbalist** | Support/healer, magic | Staff | Cleric, Seal Master |
| **Explorer** | Naval/coral elemental | Coral items | Voyager |
| **Sailor** | Life/support | — | Captain |
| **Merchant** | Trade | — | Upstart |
| **Artisan** | Crafting | — | Engineer |

> Note: Sailor, Merchant, and Artisan are life/trade classes. Their advancement and skill data is not listed on piratekings.online/database but they are present in the game.

### Tier 2 — Advanced Classes (via NPC class quest)

| Base Class | → Advanced Class | Specialty |
|-----------|-----------------|-----------|
| Swordsman | → **Crusader** | Dual swords, stealth, poison — speed/agility focus |
| Swordsman | → **Champion** | Greatsword, AoE, totem — power/tanking focus |
| Hunter | → **Sharpshooter** | Gun mastery, crowd control, silence/cripple |
| Herbalist | → **Cleric** | Group buffs, revive, energy shield, healing AoE |
| Herbalist | → **Seal Master** | Debuffs, silence, AoE defense reduction |
| Explorer | → **Voyager** | Multi-coral naval skills, fog, whirlpool |
| Sailor | → **Captain** | Naval leadership |
| Merchant | → **Upstart** | Enhanced trade |
| Artisan | → **Engineer** | Advanced crafting |

### Faction Alignment

- **Navy** — Argent City & Thundoria — favors law and order
- **Pirate** — Shaitan City — favors freedom and plunder
- Faction affects NPC availability, certain quests, and social standing (Fame system)

---

## Skill System

Skills are divided into **Passive** (always active) and **Initiate** (activated, costs SP).

### Swordsman Skills (shared by Crusader & Champion)

| Skill | Type | SP Cost | Effect |
|-------|------|---------|--------|
| Concentration | Passive | — | +1 hit rate per level |
| Taunt | Initiate | — | Force single target to attack you |
| Sword Mastery | Passive | — | +4 attack per level (sword) |
| Will of Steel | Initiate | 10 | +3 defense per level, 15s |
| Break Armor | Initiate | 25 | -4 enemy defense per level, 15s |
| Illusion Slash | Initiate | 20 | Ranged sword energy attack |
| Tiger Roar | Initiate | 20 | AoE: -3 atk / -1% spd per level, 15s |
| Berserk | Initiate | 15 | Increase hit rate + attack speed |

### Crusader Skills (2nd class)

| Skill | Type | Effect |
|-------|------|--------|
| Dual Sword Mastery | Passive | +8% left-hand attack per level |
| Deftness | Passive | +3 dodge per level |
| Blood Frenzy | Passive | -10% dual-weapon cooldown (Lv1), -2%/level |
| Stealth | Initiate | Become invisible, drains SP over time |
| Poison Dart | Initiate | Poison: 12 dmg/sec (Lv1), +2/level, 17s |
| Shadow Slash | Initiate | Melee + knockout: 110% atk (Lv1), +10%/level |

### Champion Skills (2nd class)

| Skill | Type | Effect |
|-------|------|--------|
| Greatsword Mastery | Passive | +7 attack per level (greatsword) |
| Roar | Initiate | AoE Taunt — lures multiple enemies |
| Strengthen | Passive | +20 max HP per level |
| Blood Bull | Passive | Totem: +10% HP+DEF (Lv1), +2%/level |
| Mighty Strike | Initiate | 1.25× damage (Lv1), +5%/level, 8 SP |
| Howl | Initiate | AoE: 1.05× damage (Lv1), +5%/level |
| Primal Rage | Initiate | Requires totem. 3.5× damage (Lv1), 53 SP |

### Hunter Skills (shared by Sharpshooter)

| Skill | Type | Effect |
|-------|------|--------|
| Range Mastery | Passive | +2 ranged attack per level |
| Windwalk | Passive | +2% movement speed per level |
| Dual Shot | Initiate | 2 attacks: 1.7× (Lv1), +15%/level, 20 SP |
| Rousing | Initiate | +11 attack speed (Lv1), +1/level, 15s, 35 SP |
| Venom Arrow | Initiate | Poison: 12 dmg/sec (Lv1), +2/level, 20s |
| Frozen Arrow | Initiate | Damage + slow, 15 SP |
| Meteor Shower | Initiate | AoE arrows: 60% dmg (Lv1), +10%/level, 46 SP |

### Sharpshooter Skills (2nd class)

| Skill | Type | Effect |
|-------|------|--------|
| Cripple | Initiate | -20% dodge, -50% speed, 5s |
| Firegun Mastery | Passive | +10 max / +6 min gun attack per level |
| Enfeeble | Initiate | -20% attack + 5s silence |
| Magma Bullet | Initiate | AoE fire: 33 dmg/sec on ground for 10s |
| Headshot | Initiate | Ignore defense, max damage + 5–10% max HP |

### Herbalist Skills (shared by Cleric & Seal Master)

| Skill | Type | Effect |
|-------|------|--------|
| Heal | Initiate | Restore HP (scales with STA), earns XP |
| Spiritual Bolt | Initiate | Magic damage (scales with STA) |
| Harden | Initiate | +14 defense (Lv1), +4/level, 180s |
| Spiritual Fire | Initiate | +11% attack (Lv1), +1%/level, 180s |
| Recover | Initiate | Remove abnormal status effects |
| Tempest Boost | Initiate | +6% attack speed (Lv1), +1%/level, 138s |
| Revival | Initiate | Revive dead character (requires Revival Clover) |
| Vigor | Passive | +40 max SP per level |

### Cleric Skills (2nd class)

| Skill | Type | Effect |
|-------|------|--------|
| Divine Grace | Passive | +2 SP recovery per level |
| True Sight | Initiate | Reveal invisible units in AoE |
| Tornado Swirl | Initiate | +6% crit/berserk rate (Lv1), 33s |
| Angelic Shield | Initiate | +3% defense (Lv1), +3%/level, 33s |
| Energy Shield | Initiate | Convert HP damage to SP damage |
| Healing Spring | Initiate | AoE HP regen aura, 17s |
| Crystalline Blessing | Initiate | Target becomes immune to attacks |

### Seal Master Skills (2nd class)

| Skill | Type | Effect |
|-------|------|--------|
| Cursed Fire | Initiate | AoE: -12% enemy defense (Lv1), -2%/level |
| Shadow Insignia | Initiate | Prevent normal attacks, 6s (Lv1) |
| Abyss Mire | Initiate | AoE: -30% enemy speed, 20s (Lv1) |
| Seal of Elder | Initiate | Prevent skill usage, 10.5s (Lv1) |
| Intense Magic | Initiate | Boost magical damage; chance of free cast at high level |

### Explorer Skills (shared by Voyager)

| Skill | Type | Effect |
|-------|------|--------|
| Diligence | Passive | +2 SP recovery (Lv1), +1/level |
| Current | Passive | +6% ship movement speed (Lv1) |
| Lightning Bolt | Initiate | Thunder Coral strike (scales with STA) |
| Conch Armor | Passive | +8 ship defense (Lv1), +3/level |
| Tornado | Initiate | Wind Coral knockup, 3.5s (Lv1) |
| Alga Entanglement | Initiate | Bind + DoT: 5 dmg/sec, 6s (Lv1) |

### Voyager Skills (2nd class)

| Skill | Type | Effect |
|-------|------|--------|
| Conch Ray | Initiate | Strike Coral: line damage |
| Lightning Curtain | Initiate | Thunder Coral: AoE thunderstorm |
| Tail Wind | Initiate | Wind Coral: boost ally ship speed |
| Fog | Initiate | Fog Coral: -attack to enemy ships in AoE |
| Whirlpool | Initiate | -movement speed of enemy ships in AoE |

---

## Combat System

### Attack Resolution

1. **Hit check** — attacker Hit Rate vs defender Dodge
2. **Damage** — attacker ATK - defender DEF, reduced by Physical Resistance %
3. **Critical** — rolled against Critical Hit Rate; multiplies damage
4. **Status effects** — applied per skill on successful hit

### Aggro / Behavior Types

| Behavior | Description |
|----------|-------------|
| Hostile | Attacks players on sight within Chase Range |
| Non-hostile | Does not attack unless attacked first |
| Immovable | Stationary (chests, objects) |
| Guard | Patrols/guards fixed area |

Chase Range and Movement Speed are per-monster values (stored in monster data). For example:
- **Armored Crab** (Lv15): 480 HP, 48 ATK, 50 DEF, Chase Range 1500, Non-hostile
- **Ancient Behemoth** (Lv100 Boss): 1,506,000 HP, 2284 ATK, 414 DEF, Chase Range 1500, Hostile

### Monster AI States

`Idle → Patrol → Chase (vision 400–800 units) → Combat → Flee`

Monster AI is defined in `server/resource/script/ai/ai.lua` with 7 AI type constants:

| AI Type | Constant | Behavior |
|---------|----------|---------|
| None | AI_NONE (0) | No AI |
| Non-aggressive | AI_N_ATK (1) | Non-hostile, attacks if attacked |
| Flee | AI_FLEE (2) | Runs from player |
| Follow host | AI_MOVETOHOST (4) | Follows summoner |
| Ranged attack | AI_R_ATK (5) | Attacks from range |
| Aggressive | AI_ATK (10) | Attacks on sight |
| Aggressive + flee | AI_ATK_FLEE (11) | Attacks, flees when low HP |

---

## Status Effects

21 status effect types exist in the game (defined in `server/resource/script/calculate/SkillStateType.lua`):

| Effect | Description |
|--------|-------------|
| Cut (Bleed) | Damage over time |
| Fervor Arrow | Attack buff |
| Frost Arrow | Speed debuff |
| Skyrocket | Knockback |
| Murrain | Poison damage |
| Giddy | Confusion |
| Freeze | Immobilize |
| Sleep | Prevent action |
| Bind | Prevent movement |
| Frost | Chill/slow |
| Beat Back | Knockback variant |
| Unbeatable | Invincibility |
| Toxin | Poison variant |
| Rebound | Damage reflect |
| Avatar | Summon effect |
| Titan | Power amplification |
| Blindness | -Hit rate |
| Change | Transform |
| Float | Levitate |
| Call | Summon |
| Shield | Protection buff |

---

## Quest System

### Quest Types

| Type | Description |
|------|-------------|
| **Common** | Standard quest, usually part of a chain |
| **World** | Global story quests, non-repeatable, gated by Records |
| **Random** | Daily/repeatable kill, collect, or delivery quests |

### Quest Structure

Each quest consists of:
- **Start Conditions** — required level, items, or prior quest Records
- **Start Actions** — give items, set mission ID, display dialog
- **Objectives** — `MIS_NEED_KILL`, `MIS_NEED_ITEM`, `MIS_NEED_SEND`, `MIS_NEED_CONVOY`, `MIS_NEED_EXPLORE`, `MIS_NEED_DESP`
- **Finish Conditions** — have required mission ID, check Record
- **Finish Actions** — reward XP, Gold, Fame, items; set Record flag; remove collected items

### Quest Progression Tracking

- **Records** — permanent flags set by `SetRecord(id)` / checked by `IsRecord(cha, id)`. Used for story gates and one-time completions.
- **Flags** — per-quest state flags set via `SetFlag` / `HasFlag`
- **Mission IDs** — active quest tracking. A quest can require "does not have mission X" (cannot retake) or "requires mission record Y" (prerequisite chain)

### Quest Scale

The database contains **1,141 quests** including:
- Storyline chains (e.g., Lone Tower chain: 10+ sequential quests with NPC dialogs)
- Collection quests requiring monster drops (e.g., "50x Wailing Warrior Carcass + 50x Skeleton of Sorrow Warrior")
- Exploration quests directing players to specific map coordinates
- Time-gated daily quests using `QuestFunc()` + `OS.date()` date tracking

### Example Quest Chain (Lone Tower)
```
小秘的梦想 (id:1214)
  → 男佣的不满 (id:1214 record prerequisite)
  → 奇怪的女佣 (id:1215) — go to Lone Tower Floor 2 (151,134)
  → 严肃的守护者 (id:1219) — go to Lone Tower Floor 4 (261,70)
  → 一份人情又一份人情 (id:1220) — collect 50x Black Bobcat Wing + 50x Corrupted Angel Halo
```

---

## World & Maps

### Main Cities

| City | Internal Name | Faction | Description |
|------|--------------|---------|-------------|
| Argent City | garner | Navy | Primary Navy hub, starter zone |
| Shaitan City | jialebi | Pirate | Primary Pirate hub |
| Thundoria Castle | leiting2 | Neutral | Mountain fortress city |
| Icicle City | binglang2 | Neutral | Ice city |

### Major Zones

| Zone | Level Range | Type |
|------|-------------|------|
| Magical Ocean | 20–40 | Open sea, trade routes |
| Deep Blue | 40–60 | Mid-tier grinding |
| Garden of Edel | 40–60 | Jungle zone |
| Eastern Goaf | 50–70 | Underground caves |
| Lone Tower | 50–70 | Tower dungeon (quest hub) |
| Dark Swamp | 60–80 | Swamp enemies |

### Dungeons

| Dungeon | Internal Name | Tier |
|---------|--------------|------|
| Forsaken City | abandonedcity (1–3) | Mid |
| Demonic World | puzzleworld (1–2) | High |
| Abaddon | hell (1–5) | End-game |
| Black Dragon Lair | heilong (1–2) | End-game |

### PvP Zones

| Zone | Type |
|------|------|
| PK Arena | teampk / PKmap — open and team PvP |
| Sacred War | guildwar01–04 — territory control |

### Safe Havens (12+)

Abandon Mine, Rockery, Andes Forest, Valhalla, Solace, Chaldea, Oasis, Babul, Icicle, Atlantis, Skeleton, Icespire

---

## NPC System

### NPC Categories

| Type | Examples | Function |
|------|---------|---------|
| Quest Givers | Various named NPCs | Start/complete quests |
| Merchants | Blacksmith Goldie, Tailor Granny Nila, Grocer Jimberry | Buy/sell items |
| Bankers | Monica (Argent), Macurdo | Item storage, gold deposit/withdraw |
| Teleporters | Harbor operators | Instant travel between cities/havens |
| Harbor Operators | Per-harbor NPCs | Berthing, Repair, Cargo Trade, Salvage, Supply |
| Class Trainers | Per-class NPCs | Skill/job advancement |
| Innkeepers | Various | Set spawn point |

The database lists **677 NPCs** across all maps. Key NPCs include:
- **Xmas Village** (07xmas map) — seasonal event NPCs including Gift Merchants (Crispin, Felix, Forrest, Rolo), Grocery Keeper, and quest NPCs
- **Argent City** (garner) — main service hub with bankers, class trainers, teleporters
- Standard maps each have at least: harbor operator, merchant, teleporter

### NPC Data Format

Each NPC entry in the database records:
- Display name
- Map name (internal)
- Friendly map label (e.g., "Thundoria Castle")
- World coordinates (X, Y)

---

## Monster Roster

The database lists **1,042 monster entries** spanning all content tiers.

### Monster Stat Fields

| Field | Description |
|-------|-------------|
| Level | Monster level (0 for objects/chests) |
| Behavior | Hostile / Non-hostile / Immovable / Guard |
| HP | Total health points |
| Min Attack | Minimum physical damage |
| Physical Resistance | % damage reduction |
| Defense | Flat damage reduction |
| Hit Rate | Accuracy stat |
| Dodge | Evasion stat |
| Critical Hit Rate | Crit chance |
| HP Recovery | Passive HP regen per tick |
| SP Recovery | Passive SP regen |
| Attack Speed | Milliseconds per attack |
| Chase Range | Detection radius |
| Movement Speed | Movement units per second |
| STR / AGI / DEX / CON / STA | Base stats |
| Experience | XP awarded on kill |
| Drop Table | Items with % drop rates |

### Stat Examples by Tier

| Monster | Level | HP | Min ATK | DEF | Type |
|---------|-------|----|---------|-----|------|
| Armored Crab | 15 | 480 | 48 | 50 | Non-hostile |
| Angel Squirt | ~25 | — | — | — | Standard |
| Ancient Siren | ~60 | — | — | — | Standard |
| Ancient Behemoth | 100 | 1,506,000 | 2,284 | 414 | Boss |

### Monster Categories (from naming patterns)

- **Crabs, Turtles, Sea creatures** — coastal zones, low-mid level
- **Sirens, Mermaids** — ocean zones
- **Tribal Villagers, Shamans** — jungle/forest zones
- **Undead (Skeletons, Wailing Warriors)** — dungeon zones
- **Demons, Abyss Lords** — end-game dungeons (Abaddon)
- **Abyss Lords** — Lv80–100 named bosses: Phantom Baron, Demon Flame, Evil Beast, Tyran, Phoenix, Despair, Drakan, Asura, Hardin, Kara (Supreme)
- **Behemoths** — world bosses (Ancient Behemoth Lv100, Lv1,506,000 HP)
- **Objects** — Abandoned Chests (drop class-specific loot), Token Chests (Maze Tokens)

### Boss Encounter Notes

World bosses like the **Ancient Behemoth** have:
- `Private Space: 500` — large aggro bubble around boss
- `Chase Range: 1500` — long pursuit range
- Faster attack speed (1500ms) compared to normal mobs (2000ms)
- Multiple distributed spawn positions with long respawn timers (20–145 min typical)

---

## Naval & Ship System

### Ship Progression

- Ships level **1–100** on a separate XP track (exponential curve, independent of character level)
- Sailing XP is earned from sea combat, cargo runs, and exploration

### Harbors (15 named locations)

Argent, Thundoria, Shaitan, Icicle, Zephyr, Glacier, Outlaw, Chill Harbor + additional haven harbors

**Harbor services at each location:**
- **Berthing** — dock/undock ship
- **Repair** — restore ship HP
- **Cargo Trade** — buy/sell cargo goods
- **Salvage** — recover sunken ship items
- **Supply** — replenish consumables

### Naval Combat

Naval combat uses **Coral items** activated by Explorer/Voyager class skills:

| Coral Type | Skill Example | Effect |
|-----------|--------------|--------|
| Thunder Coral | Lightning Bolt | Ranged electrical strike |
| Wind Coral | Tornado | Knockup / speed boost |
| Strike Coral | Conch Ray | Line damage |
| Fog Coral | Fog | -attack AoE |

Ship stats (HP, defense, speed) scale with ship level and are buffed by Conch Armor (passive) and Current (speed).

---

## Economy & Trade

### Currency

| Currency | Use |
|----------|-----|
| **Gold (G)** | Primary currency — earned from quests, monster drops, trading |
| **Crystals (IMP)** | Premium currency — in-game mall purchases |

### Commerce System

Players can purchase **cargo goods** cheaply at one harbor city and sell them at higher prices elsewhere. **Commerce Permits** (Low/Mid/High tier) reduce trade tax, improving profit margins. This is the primary gold-making activity for Merchant/Sailor class players.

### Player Markets

- **Offline Stalls** — set up a personal shop that persists while offline
- **Direct Trade** — face-to-face player-to-player trading
- **NPC Shops** — fixed-price vendor NPCs in each city

### In-Game Mall (IMP Shop)

Categories: Fairy, Leveling aids, Equipment, Forging, Tickets, Apparels, Grocery, Mounts, Special items

Mall mechanics:
- Per-item stock limits
- 30-second cooldown after 3 failed purchases
- Purchase history logged per player

---

## Pet / Fairy System

### Fairy Types (10+)

Life, Darkness, Virtue, Kudos, Faith, Valor, Hope, Woe, Love, Heart, and others.

Fairies are obtained from eggs (purchased from the mall or dropped by monsters).

### Fairy Progression

| Action | Item Required | Effect |
|--------|--------------|--------|
| Train stats | Fruits (STR/AGI/DEX/CON/STA variants) | Increase specific fairy stat |
| Feed | Rations | Maintain fairy hunger/loyalty |
| Skill usage | — | Offensive (Protection, Berserk, Magic) and healing (Recovery, Meditation) skills |

### Skill Tiers

`Novice → Standard → [higher tiers]`

### Fairy Marriage / Breeding

Two fairies can be combined (marriage/breeding) with fruits influencing the offspring's stat distribution.

---

## Equipment & Crafting

### Equipment Slots (34 total)

Head, Face, Body, Gloves, Shoes, Neck, 2× Weapons, Hands, 4× Jewelry, Wings, Cloak, Fairy, Mount, Rear, 5× Cosmetic appearance slots, special weapon type slots.

### Item Attribute Types (49+)

**Coefficient stats:** STR, AGI, DEX, CON, STA, LUK, ASPD (attack speed)  
**Direct value bonuses:** +HP, +ATK, +DEF, etc.  
**Special attributes:** Refinement level, Fusion ID, Durability

### Item Types

| Type ID | Category |
|---------|---------|
| 0 | Weapons |
| 1 | Armor / Defense |
| 2 | Other (consumables, misc) |
| 3 | Synthesis materials |
| 4+ | Gems |

### Forging System

The forging system (`server/resource/script/calculate/forge.lua`) supports:

**Gem Combining:**
- Formula: 2× same-type/level gems + scroll → 1× higher tier gem
- Success rates: Lv0=100%, Lv1=90%, Lv2=80%... Lv7=30%, Lv8=20% (varies by gem type)
- Type 49 gems degrade faster; Type 50 gems start at 100% and degrade slower
- **Bonus Fruits** can be used to increase success rate (+10% per fruit level)

**Equipment Enhancement:**
- **Refine** — increases weapon damage or armor defense
- **Socket** — add gem slots to equipment
- **Upgrade / Transcendence** — further power enhancement tiers

### Class-Specific Loot Chests

Abandoned Chests in dungeons drop class-specific equipment:
- Abandoned Chest 1: Yal Runestone (20%), Lum Runestone (20%), Beautiful Chest (10%), Mystic Chest (20%)
- Abandoned Chest 2: Skeletar Chest of Swordsman (15%), Hunter (15%), Explorer (15%)
- Abandoned Chest 3: Incantation Chest of Champion (15%), Crusader (15%), Sharpshooter (15%)

---

## Guild & PvP

### Guild Requirements

- Character level: 40+
- Gold cost + item requirements
- Guild leader must purchase and place Guild Stone

### Guild Features

- Guild treasury (gold pooling)
- Guild bank log
- Online member tracking
- **Sacred War** participation (territory PvP)

### PvP Systems

| System | Map | Type |
|--------|-----|------|
| Open PvP | PKmap | Free-for-all |
| Team PvP | teampk | Team battle |
| Sacred War | guildwar01–04 | Territory control — guilds fight for map zones |
| Sea PvP | Open ocean | Ship-to-ship battles |

### Fame System

Fame is tied to faction standing (Navy vs Pirate). Earned through PvP kills, quests, and faction missions. Affects NPC dialog options and certain faction-gated content.

---

## Rebirth System

After reaching maximum level, players can **Rebirth** — resetting their level in exchange for permanent power bonuses.

### Rebirth Benefits

**All rebirthed classes gain:**

| Skill | Effect |
|-------|--------|
| Rebirth Mystic Power (Passive) | +5.5% damage and damage reduction at Rebirth Lv1; +0.5% per additional rebirth level |

### Rebirth Class Skills

Each advanced class gains a unique rebirth-exclusive skill:

| Class | Rebirth Skill | Effect |
|-------|--------------|--------|
| Crusader | Ethereal Slash | Burst sword energy to all surrounding enemies (+30 dmg/level) |
| Champion | Beast Legion Smash | Greatsword AoE, reduces enemy speed (+30 dmg/level) |
| Sharpshooter | Red Thunder Cannon | Gun: damages all targets in a line |
| Cleric | Holy Judgement | Staff: holy AoE judgment on surrounding targets |
| Seal Master | Devil Curse | Staff: summon devil for DoT + DEF/SPD/ASPD/ATK debuff |
| Voyager | Super Consciousness | Dagger: summon objects to hit surrounding enemies (+30 dmg/level) |

---

## Special Events

### Christmas Event (07xmas map — Xmas Village)

A dedicated seasonal map (`07xmas` / `xMas Arena`) populated with:
- Gift Merchants: Crispin, Felix, Forrest, Rolo (at 91–175, 250–256)
- Grocery Keeper (194, 252)
- Quest NPCs: Captain Kalimi, Colonel Karee, Grandmother Michia, Melancholy Elder
- Environmental NPCs: Christmas Tree (144, 160), Christmastide Bonfire (249, 229)
- Player NPCs: Latte Boy, Graybeard Noot, Amateur Dancer, and others

### Time-Based Events

Events use a date formula: `NowDateNum = Year × 1,000,000 + Month × 10,000 + Day × 100 + Hour`

Daily and weekly quests are capped via `QuestFunc()` + `OS.date()` checks. Hourly `TE_GAMETIME` triggers can spawn event bosses or change world state.

### Boss Hunt Events

Special boss spawns in high-level zones (e.g., Ancient Behemoth in Abandon Mine 2) trigger timed hunts. Distributed respawn positions prevent camping a single spawn point.

### Maze Token System

Special Token Chests (Abandoned Tokens Chest 1–3) spawn in dungeon content and drop **Maze Token Fragments** (25% drop rate each). Fragments can be combined into **Maze Tokens** — a secondary currency used for special rewards.

---

## Data Sources

| Source | Coverage |
|--------|---------|
| piratekings.online/database/quest/ | 1,141 quests |
| piratekings.online/database/monster/ | 1,042 monsters |
| piratekings.online/database/npc/ | 677 NPCs |
| piratekings.online/database/class/ | 10 classes (all skill trees) |
| server/resource/script/calculate/ | Formulas: AttrCalculate.lua, forge.lua, exp_and_level.lua |
| server/resource/script/ai/ | AI behavior: ai.lua, ai_base.lua, ai_sdk.lua |
| server/resource/script/MisScript/ | NPC definitions, quest scripts |
| server/resource/script/monster/ | Spawn definitions |

Full raw reference data is in: `C:\Users\Ken\.copilot\session-state\79b7511f-...\files\pko-reference\`
