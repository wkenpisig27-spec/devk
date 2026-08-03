# Equipment V2 (Family Gear)

## ID layout

```
id = 10000 + tierIndex*100 + (familyId-1)*12 + slotIndex
```

| TierIndex | Level | ID base |
|----------:|------:|--------:|
| 0 | 10 | 10000 |
| 1 | 20 | 10100 |
| 2 | 30 | 10200 |
| 3 | 40 | 10300 |
| 4 | 50 | 10400 |
| 5 | 60 | 10500 |
| 6 | 70 | 10600 |

Families (set ID): 1 Guardian, 2 Royal, 3 Wind, 4 Hunter, 5 Shadow, 6 Sanctum

Slots 0-11: Greatsword, Sword, Bow, Gun, Staff, Dagger, Cap, Body, Gloves, Boots, Necklace, Ring

Total: 6 families × 7 tiers × 12 slots = **504** items (`10000`–`10671`)

## Regenerate

```bash
python server/resource/equipment_v2/generate_equipment.py
```

## Runtime

- `server/addons/equipment_families.lua` — family affix pools + set bonuses
- `server/addons/equips.lua` — rarity rolls (family-aware for 10000+)
- `AttrRecheck` calls `ApplyFamilySetBonuses`
- `CItemRecord::sSetID` loaded from ItemInfo set ID column

## Test (GM)

```
/giveset Guardian,40
/familyset Royal,70
/givesetjob Wind,40,crusader
```

Jobs for `/givesetjob`: `champion`, `crusader`, `sharpshooter`, `cleric`, `seal`, `voyager`, `all`

Give a full T40 Guardian set, equip, confirm 2/4/6 set bonuses after `AttrRecheck`.

## Client tooltips

Family gear (`10000+`) shows rarity-colored titles, family taglines, live set progress (2/4/6), and a rolled-affix section that encourages chasing Mythic (5/5) rolls.
Rebuild the **client** after pulling `UIItemCommand` changes.
