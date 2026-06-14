# VEIL//WORLD - The 151 Eidolons

Version: 0.1  
Purpose: Complete roster reference for Eidolon lore, gameplay roles, empire jobs, rarity, habitats, and asset planning.

## Overview

Every Eidolon should eventually contain:

- Origin lore
- Temperament
- Combat role
- Empire role
- Active abilities
- Passive traits
- Habitat
- Rare variants
- Evolution potential
- Asset requirements
- Empire job assignments

This document starts as the master roster. Individual Eidolons can later be expanded into full design sheets.

## Asset Naming Conventions

| Type | Format | Example |
| --- | --- | --- |
| Mesh | `SM_Eidolon_{Name}` | `SM_Eidolon_EmberFox` |
| Skeleton | `SK_Eidolon_{Name}` | `SK_Eidolon_EmberFox` |
| Animation Blueprint | `ABP_Eidolon_{Name}` | `ABP_Eidolon_EmberFox` |
| Behavior Tree | `BT_Eidolon_{Name}` | `BT_Eidolon_EmberFox` |
| Icon | `T_Icon_Eidolon_{Name}` | `T_Icon_Eidolon_EmberFox` |
| VFX | `NS_Eidolon_{Name}_{Ability}` | `NS_Eidolon_EmberFox_EmberDash` |
| Sound | `SFX_Eidolon_{Name}_{Action}` | `SFX_Eidolon_EmberFox_Attack` |
| Material Instance | `MI_Eidolon_{Name}_{Variant}` | `MI_Eidolon_EmberFox_AshVariant` |

Note: if VEIL//WORLD remains a 0 A.D. mod first, these names are still useful as internal asset IDs, but final file formats and folders should match the 0 A.D. art pipeline.

## Blender MCP Generation Commands

All creature blockouts use:

```text
blender_generate_creature_blockout(body_type, size, name)
```

Supported `body_type` values:

- `quadruped`
- `biped`
- `serpent`
- `bird`

Suggested `size` range:

- `0.3` tiny
- `0.5-0.7` small
- `0.8-1.2` medium
- `1.5-2.5` large
- `3.0-5.0` titan

## The Four Primal Starters

### Starter 1 - Ember Fox

- Core Identity: balanced utility / exploration starter
- Evolution Themes: fire, lightning, spirit, volcanic, dimensional, stealth
- Empire Paths: mining, scouting, furnace support, speed courier, stealth hunting

### Starter 2 - Strayn Claw

- Core Identity: agility / combat starter
- Evolution Themes: stealth, electricity, shadow, jungle, storm, void
- Empire Paths: defense, hunting, reconnaissance, power routing, patrol systems

### Starter 3 - Aero Finch

- Core Identity: traversal / scouting starter
- Evolution Themes: wind, storm, celestial, sound, sky, gravity
- Empire Paths: scouting, communications, sky transport, aerial defense, weather stabilization

### Starter 4 - Tideborn

- Core Identity: aquatic survival / adaptability starter
- Evolution Themes: ocean, pressure, abyssal, electricity, coral, leviathan
- Empire Paths: fishing, underwater exploration, submarine assistance, ocean cargo, aquatic farming

## Fox Line: 1-20

| # | Name | Habitat | Role | Empire Job | Size | Rarity | Body Type | Lore Summary |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Ember Fox | Fractured Forest | DPS/Scout | Furnace, Resin Gathering, Heat Gen | Small | Common | Quadruped | First Aether-altered creature. Internal combustion organs generate warmth. Seeks stranded humans. |
| 2 | Ashfang | Ashfall Crater | DPS/Breaker | Ore Processing, Volcanic Scout | Medium | Uncommon | Quadruped | Volcanic evolution with superheated jaws that melt metal. Processes ore faster than furnaces. |
| 3 | Volter Fox | Stormbreak Archipelago | Support/DPS | Power Generation, Grid Stabilization | Small | Uncommon | Quadruped | Electrical mutation. Bio-generators stabilize damaged power grids for days. |
| 4 | Spirit Ember | Fractured Forest | Healer | Medical Support, Creature Nursery | Small | Rare | Quadruped | Manifests from profound emotional bonding. Healing warmth instead of destructive fire. |
| 5 | Voidtail Fox | Void Rift | Assassin/Scout | Portal Detection, Stealth Recon | Small | Rare | Quadruped | Phases partially out of reality. Nearly impossible to catch without specialized traps. |
| 6 | Frost Vulpine | Frozen Fracture | Support/Tank | Cold Storage, Ice Harvesting | Small | Uncommon | Quadruped | Ice-adapted fox found near frozen fracture zones. Preserves food and materials. |
| 7 | Mossfire Fox | Verdant Wilds | Agriculture | Plant Growth, Camp Fertility | Small | Common | Quadruped | Forest utility fox that accelerates plant growth around camps. |
| 8 | Duskflare | Fractured Forest | DPS | Night Patrol, Ambush Hunting | Small | Uncommon | Quadruped | Nocturnal predator with flame trails used to blind enemies. |
| 9 | Cinder Warden | Ashfall Crater | Tank/Defender | Base Defense, Volcanic Nest Guard | Large | Rare | Quadruped | Massive defensive fox evolution protecting volcanic nests. |
| 10 | Rift Fox | Void Rift | Scout | Portal Tracking, Dimensional Mapping | Small | Rare | Quadruped | Portal-sensitive scout capable of tracking dimensional seams. |
| 11 | Solar Vulpine | Sunscorch Plains | Support | Cave Lighting, Exploration | Small | Uncommon | Quadruped | Light-emitting fox used by explorers in dark biomes. |
| 12 | Ember Hound | Fractured Forest | DPS | Pack Combat, Coordinated Attacks | Medium | Uncommon | Quadruped | Pack-combat evolution specialized for coordinated attacks. |
| 13 | Brimstone Jackal | Scorched Desert | DPS/Survival | Desert Scouting, Heat Resistance | Medium | Uncommon | Quadruped | Desert survival predator resistant to extreme heat. |
| 14 | Thornfur Fox | Verdant Wilds | Stealth | Woodland Patrol, Camouflage | Small | Common | Quadruped | Camouflaged woodland stalker covered in thorn-like fur. |
| 15 | Echo Fang | Fracture Zones | Scout | Temporal Anomaly Detection | Small | Rare | Quadruped | Aether-sensitive fox able to detect temporal anomalies. |
| 16 | Stormtail | Stormbreak Archipelago | Mount/DPS | Fast Travel, Electrical Burst | Medium | Rare | Quadruped | Fast-travel mount evolution utilizing electrical bursts. |
| 17 | Pyroclast Fox | Deep Volcanic | Mining | Magma Tunnel Mining | Medium | Rare | Quadruped | Heavy mining fox adapted for magma tunnels. |
| 18 | Crystal Vulpine | Crystal Basin | Mining | Mineral Detection, Crystal Harvest | Small | Uncommon | Quadruped | Crystal-coated variant that resonates with mineral deposits. |
| 19 | Hollow Flame | Corrupted Zones | Wild/Hostile | None: Corrupted | Medium | Epic | Quadruped | Corrupted fox consumed by unstable Aether exposure. Cannot be tamed normally. |
| 20 | Aetherion Fox | Dimensional Nexus | Legendary | Reality Stabilization | Medium | Mythic | Quadruped | Legendary fox believed descended from the first Eidolon bloodline. |

## Cat Line: 21-40

| # | Name | Habitat | Role | Empire Job | Size | Rarity | Body Type | Lore Summary |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 21 | Strayn Claw | Urban Ruins | DPS/Scout | Defense, Hunting, Recon | Small | Common | Quadruped | Highly adaptive feline starter. Nervous system responsive to Aether frequencies. |
| 22 | Volt Lynx | Industrial Zones | DPS/Utility | Power Routing, Industrial Hunt | Medium | Uncommon | Quadruped | Electrical predator used in industrial power routing. |
| 23 | Shadow Prowler | Ruined Cities | Assassin | Stealth Ops, Night Patrol | Medium | Uncommon | Quadruped | Stealth assassin evolution operating in ruined cities. |
| 24 | Storm Saber | Windscar Cliffs | DPS | Cliff Patrol, Storm Combat | Large | Rare | Quadruped | Storm-adapted apex predator hunting along cliff regions. |
| 25 | Rift Panther | Void Rift | Assassin | Dimensional Patrol, Phase Strikes | Medium | Rare | Quadruped | Dimensional cat capable of short-range phasing. |
| 26 | Crystal Fang | Crystal Basin | Mining/DPS | Crystal Harvesting, Cave Combat | Medium | Uncommon | Quadruped | Mineral-armored hunter feeding on crystal ecosystems. |
| 27 | Ironclaw Tiger | Frontier Zones | Tank/DPS | Territorial Defense, War Mount | Large | Rare | Quadruped | Massive armored combat Eidolon used in territorial wars. |
| 28 | Ashmane Cougar | Ashfall Crater | DPS | Volcanic Patrol, Heat Combat | Medium | Uncommon | Quadruped | Volcanic feline with heat-resistant musculature. |
| 29 | Frost Saber | Frozen Fracture | DPS | Ice Patrol, Freezing Attacks | Large | Rare | Quadruped | Ice biome predator using freezing claw attacks. |
| 30 | Echo Jaguar | Fracture Zones | Scout | Distortion Tracking, Anomaly Hunt | Medium | Rare | Quadruped | Tracks dimensional distortions and unstable creatures. |
| 31 | Thorn Pouncer | Jungle | Assassin | Jungle Patrol, Ambush | Medium | Uncommon | Quadruped | Jungle ambush hunter using camouflage fur. |
| 32 | Duskrunner | Trade Routes | Scout/Mount | Caravan Escort, Night Scout | Medium | Common | Quadruped | Fast nocturnal scout favored by caravans. |
| 33 | Sunclaw Lion | Sunscorch Plains | DPS/Alpha | Territory Control, Solar Combat | Large | Rare | Quadruped | Solar-powered alpha predator with radiant attacks. |
| 34 | Arc Panther | Stormbreak | DPS | Chain Lightning Combat | Medium | Rare | Quadruped | Electrical combat evolution with chain-lightning abilities. |
| 35 | Mirecat | Toxic Swamp | Survival | Swamp Patrol, Toxin Resistance | Small | Uncommon | Quadruped | Swamp predator adapted to toxic wetlands. |
| 36 | Hollow Fang | Corrupted Zones | Wild/Hostile | None: Corrupted | Medium | Epic | Quadruped | Corrupted cat evolution consumed by Void energy. |
| 37 | Mistclaw | Fog Regions | Assassin | Fog Patrol, Illusion Ambush | Medium | Uncommon | Quadruped | Fog-dwelling ambush hunter using illusion-like movement. |
| 38 | Titan Saber | Frontier Wars | Siege | Siege Combat, Frontier Warfare | Large | Epic | Quadruped | Large siege feline used in frontier warfare. |
| 39 | Astral Lynx | Celestial Zones | Support | Celestial Event Sensing | Small | Epic | Quadruped | Rare cosmic feline capable of sensing celestial events. |
| 40 | Veil Stalker | The Veil Edge | Legendary | Reality Assassination | Medium | Mythic | Quadruped | Legendary assassin creature tied to the collapse of The Veil. |

## Bird Line: 41-60

| # | Name | Habitat | Role | Empire Job | Size | Rarity | Body Type | Lore Summary |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 41 | Aero Finch | Windscar Cliffs | Scout | Scouting, Communications | Tiny | Common | Bird | Starter aerial Eidolon adapted for scouting and communication. |
| 42 | Static Finch | Storm Regions | Signal | Communication Relay, Grid Balance | Tiny | Common | Bird | Communicates through electromagnetic pulses. |
| 43 | Driftwing | Open Skies | Transport | Long-Distance Travel, Mapping | Small | Uncommon | Bird | Glider species evolved for long-distance travel. |
| 44 | Thunder Roc | Mountain Peaks | DPS/Boss | Mountain Defense, Storm Combat | Titan | Rare | Bird | Massive storm predator feared across mountain regions. |
| 45 | Celestial Roc | High Atmosphere | Legendary | Solar Myths, Sky Sovereignty | Titan | Epic | Bird | Ancient flying Eidolon associated with solar myths. |
| 46 | Arc Talon | Storm Regions | DPS | Aerial Combat, Electric Dives | Medium | Uncommon | Bird | Fast aerial hunter utilizing electric dive strikes. |
| 47 | Tempest Seraph | Storm Eye | Support | Atmospheric Stabilization | Large | Rare | Bird | Storm-support creature stabilizing atmospheric anomalies. |
| 48 | Sky Manta | Open Skies | Transport | Air Transport, Exploration | Large | Uncommon | Bird | Air-gliding transport species used for exploration. |
| 49 | Razorwing | Windscar Cliffs | DPS | Aerial Interception, Speed Combat | Medium | Rare | Bird | Extremely fast aerial combat predator. |
| 50 | Storm Crow | Storm Regions | Recon | Storm Recon, Lightning Survival | Small | Common | Bird | Recon bird capable of surviving lightning storms. |
| 51 | Ash Vulture | Ashfall Crater | Scavenger | Volcanic Scavenging, Corpse Cleanup | Medium | Common | Bird | Scavenger species adapted to volcanic wastelands. |
| 52 | Prism Falcon | Crystal Basin | DPS/Scout | Crystal Recon, Refractive Attack | Small | Uncommon | Bird | Crystal biome hunter with refractive feathers. |
| 53 | Voidwing | Void Rift | Assassin | Dimensional Air Patrol | Medium | Rare | Bird | Corrupted sky creature phasing through dimensional tears. |
| 54 | Frostfeather | Frozen Fracture | Support | Cold Support, Polar Scouting | Small | Uncommon | Bird | Cold-resistant support bird from polar fractures. |
| 55 | Glowtail Owl | Caves/Night | Scout | Night Scouting, Cave Navigation | Small | Common | Bird | Bioluminescent nocturnal scout. |
| 56 | Echo Raven | Fracture Zones | Signal | Portal Frequency Detection | Small | Uncommon | Bird | Signal-interference bird reacting to portal frequencies. |
| 57 | Sunhawk | Sunscorch Plains | DPS | Solar Dive Attacks, Day Patrol | Medium | Rare | Bird | Solar predator using radiant dive attacks. |
| 58 | Cyclone Harrier | Storm Regions | DPS/Control | Vortex Generation, Wind Combat | Medium | Rare | Bird | Wind-combat species capable of generating vortexes. |
| 59 | Nimbus Crane | Wetlands/Sky | Support | Weather Balancing, Rain Control | Medium | Uncommon | Bird | Weather-balancing support creature. |
| 60 | World Roc | Global Migration | Legendary | Storm Reshaping, World Events | Titan | Mythic | Bird | Titan-class sky beast whose migrations reshape storms. |

## Fish Line: 61-80

| # | Name | Habitat | Role | Empire Job | Size | Rarity | Body Type | Lore Summary |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 61 | Tideborn | Coastal Shallows | Utility | Fishing, Shallow Exploration | Small | Common | Serpent | Starter aquatic species adapted to shallow dimensional tides. |
| 62 | Mirefin | Marshes/Coast | DPS/Mount | Underwater Mount, Amphibious Hunt | Medium | Uncommon | Serpent | Amphibious aquatic hunter and underwater mount. |
| 63 | Abyss Ray | Deep Ocean | Scout | Deep-Sea Scouting, Bioluminescent Comms | Medium | Uncommon | Serpent | Deep-ocean scout with bioluminescent communication. |
| 64 | Tide Hydra | Dimensional Trenches | DPS/Boss | Ocean Defense, Trench Guard | Large | Rare | Serpent | Multi-headed ocean predator near dimensional trenches. |
| 65 | Leviathan Spawn | Abyssal Depths | DPS | Deep Combat, Pressure Resistance | Large | Epic | Serpent | Juvenile stage of world-scale sea entities. |
| 66 | Coral Serpent | Reef Systems | Support | Reef Protection, Coral Farming | Medium | Uncommon | Serpent | Reef guardian species protecting coral ecosystems. |
| 67 | Glowjaw Eel | Underwater Caves | Utility | Underwater Power, Colony Lighting | Small | Common | Serpent | Electrical aquatic species powering underwater colonies. |
| 68 | Deepmaw | Abyssal Depths | DPS | Abyss Hunting, Unstable Organism Cleanup | Large | Rare | Serpent | Abyss predator consuming unstable marine organisms. |
| 69 | Stormfin Shark | Storm Seas | DPS | Storm Sea Combat, Aggressive Patrol | Large | Rare | Serpent | Aggressive combat species active during superstorms. |
| 70 | Brine Drake | High-Pressure Zones | DPS/Mount | Deep Exploration, Pressure Mount | Large | Rare | Serpent | Aquatic dragon evolution for high-pressure zones. |
| 71 | Void Koi | Dimensional Pools | Legendary | Timeline Phasing, Dimensional Fish | Small | Epic | Serpent | Believed to phase between timelines. |
| 72 | Crystal Eel | Crystal Caves | Mining | Energy Mineral Harvesting | Small | Uncommon | Serpent | Resonance creature feeding on energy-rich minerals. |
| 73 | Tide Crawler | Shorelines | Scavenger | Shore Scavenging, Tidal Harvest | Small | Common | Quadruped | Amphibious shoreline scavenger. |
| 74 | Reef Grazer | Coral Reefs | Agriculture | Aquatic Farming, Reef Maintenance | Small | Common | Serpent | Passive aquatic farming species. |
| 75 | Saltback Croc | Toxic Swamp | Tank/DPS | Swamp Apex, Toxin Resistance | Large | Uncommon | Quadruped | Swamp apex predator resistant to toxins. |
| 76 | Abyss Lantern | Deep Ocean | Utility | Deep-Sea Navigation, Lighting | Small | Uncommon | Serpent | Bioluminescent deep-sea navigation creature. |
| 77 | Pearl Manta | Open Ocean | Transport | Underwater Cargo, Ocean Transport | Large | Rare | Serpent | Large transport species for underwater cargo. |
| 78 | Ocean Titan | Ocean Settlements | Mount/Base | Ocean Settlement Support | Titan | Epic | Serpent | Massive aquatic mount supporting ocean settlements. |
| 79 | Umbra Leviathan | Void Ocean | Legendary/Boss | World Event: Void Sea God | Titan | Mythic | Serpent | Void-corrupted sea god feared by sailors. |
| 80 | World Leviathan | Planetary Tides | Legendary | World Event: Planetary Tides | Titan | Mythic | Serpent | Ancient ocean entity tied to planetary tides. |

## Labor / Industrial Line: 81-100

| # | Name | Habitat | Role | Empire Job | Size | Rarity | Body Type | Lore Summary |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 81 | Rootback | Verdant Wilds | Agriculture | Mobile Farm, Ecosystem Carrier | Titan | Rare | Quadruped | Living ecosystem turtle carrying mobile farms. |
| 82 | Titan Beetle | Mining Colonies | Mining | Heavy Excavation, Tunnel Boring | Large | Uncommon | Quadruped | Heavy excavation creature used in mining colonies. |
| 83 | Ironhide Mammoth | Construction Sites | Construction | Megastructure Moving, Heavy Lift | Titan | Rare | Quadruped | Construction beast capable of moving megastructures. |
| 84 | Magma Crawler | Molten Regions | Industrial | Furnace Living, Lava Processing | Large | Uncommon | Quadruped | Industrial furnace creature living in molten regions. |
| 85 | Quarry Tortoise | Mining Zones | Mining | Stone Processing, Large-Scale Mining | Titan | Uncommon | Quadruped | Stone-processing Eidolon used in large-scale mining. |
| 86 | Furnace Boar | Industrial Zones | Industrial | Heat Refinery, Smelting | Large | Uncommon | Quadruped | Heat-generating refinery creature. |
| 87 | Steamjaw Rhino | Trade Routes | Transport | Cargo Hauling, Industrial Labor | Large | Uncommon | Quadruped | Cargo-hauling industrial labor beast. |
| 88 | Bronzeback Ape | Construction | Construction | Advanced Building, Tool Use | Medium | Rare | Biped | Advanced construction helper with tool intelligence. |
| 89 | Gearjaw Croc | Machine Zones | Industrial | Machine Integration, Repair | Large | Rare | Quadruped | Machine-integrated industrial predator. |
| 90 | Dustmaw Worm | Underground | Tunneling | Massive Tunneling, Terrain Reshape | Titan | Rare | Serpent | Massive tunneling species reshaping terrain. |
| 91 | Rivet Hound | Workshops | Repair | Mechanical Repair, Maintenance | Small | Common | Quadruped | Mechanical repair companion. |
| 92 | Pulse Ram | Power Grids | Power | Energy Transfer, Grid Support | Medium | Uncommon | Quadruped | Energy-transfer creature used in power grids. |
| 93 | Ironvine Serpent | Underground | Transport | Pipeline Maintenance, Underground Transport | Large | Uncommon | Serpent | Pipeline maintenance and underground transport. |
| 94 | Shellback Grazer | Trade Routes | Transport | Mobile Storage, Massive Cargo | Titan | Uncommon | Quadruped | Mobile storage creature carrying massive cargo. |
| 95 | Static Ape | Generator Zones | Power | Generator Boost, Machine Efficiency | Medium | Uncommon | Biped | Generator support creature boosting machine efficiency. |
| 96 | Ember Ant | Harvest Zones | Gathering | Swarm Harvesting, Resource Collection | Tiny | Common | Quadruped | Swarm-based industrial harvesting species. |
| 97 | Obsidian Tusk | Volcanic Mines | Mining | Volcanic Excavation, Titan Mining | Titan | Rare | Quadruped | Volcanic excavation titan. |
| 98 | Stonehorn Yak | Mountain Passes | Transport | Mountain Cargo, Trade Caravans | Large | Common | Quadruped | Mountain cargo species used in trade caravans. |
| 99 | Volt Ox | Infrastructure | Power | Electrical Infrastructure Support | Large | Uncommon | Quadruped | Electrical infrastructure support beast. |
| 100 | Titan Drone Wyrm | Aerial Freight | Transport | Late-Game Aerial Freight | Titan | Epic | Serpent | Late-game aerial freight creature. |

## Forest / Wildland Line: 101-120

| # | Name | Habitat | Role | Empire Job | Size | Rarity | Body Type | Lore Summary |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 101 | Hollow Wolf | Fractured Forest | DPS | Pack Combat, Forest Patrol | Medium | Common | Quadruped | Pack predator mutated by early fractures. |
| 102 | Vine Panther | Jungle | Assassin | Jungle Stealth, Plant Camouflage | Medium | Uncommon | Quadruped | Stealth jungle hunter blending with plant life. |
| 103 | Mossfang Bear | Deep Forest | Tank | Forest Defense, Guardian | Large | Rare | Quadruped | Massive defensive forest guardian. |
| 104 | Thornclaw Raptor | Forest Edge | DPS | Fast Ambush, Perimeter Patrol | Medium | Uncommon | Quadruped | Fast ambush predator. |
| 105 | Root Mother | Ancient Groves | Legendary | Ecosystem Control, World Event | Titan | Epic | Quadruped | Ancient living plant entity controlling ecosystems. |
| 106 | Barkhide Elk | Deep Forest | Mount | Forest Mount, Heavy Transport | Large | Uncommon | Quadruped | Heavy mount species adapted for forests. |
| 107 | Spirit Lynx | Sacred Groves | Support | Emotional Tracking, Mystic Scout | Small | Rare | Quadruped | Mystic tracker sensitive to emotional energy. |
| 108 | Bloom Serpent | Jungle | DPS | Poison Combat, Floral Ambush | Medium | Uncommon | Serpent | Poisonous floral predator. |
| 109 | Canopy Manta | Forest Canopy | Transport | Tree-Gliding, Canopy Scout | Medium | Uncommon | Bird | Tree-gliding aerial forest creature. |
| 110 | Verdant Titan | Ancient Forest | Legendary | Ecosystem Stabilization, World Event | Titan | Epic | Quadruped | Titan-class ecosystem stabilizer. |
| 111 | Glow Antler | Sacred Groves | Healer | Forest Restoration, Healing | Medium | Rare | Quadruped | Healing support creature tied to forest restoration. |
| 112 | Bramble Boar | Forest Edge | Tank | Territorial Defense, Aggressive Guard | Large | Uncommon | Quadruped | Aggressive territorial defense beast. |
| 113 | Whisper Hound | All Forests | Scout | Invisible Threat Detection | Medium | Rare | Quadruped | Detection creature sensing invisible threats. |
| 114 | Petal Drake | Jungle Canopy | Scout | Fast Floral Scout, Aerial Recon | Small | Uncommon | Bird | Fast floral aerial scout. |
| 115 | Bloom Ape | Verdant Wilds | Agriculture | Agricultural Helper, Planting | Medium | Common | Biped | Agricultural helper species. |
| 116 | Razorvine Stalker | Dense Jungle | Assassin | Vegetation Ambush, Silent Kill | Medium | Rare | Quadruped | Ambush predator hidden within vegetation. |
| 117 | Spirit Elk | Sacred Groves | Mount | Sacred Mount, Ceremonial | Large | Epic | Quadruped | Rare sacred mount creature. |
| 118 | Thornback Rhino | Forest Border | Siege | Siege Defense, Structure Breaking | Large | Rare | Quadruped | Siege-capable defense creature. |
| 119 | Echo Owl | Night Forest | Scout | Nocturnal Anomaly Tracking | Small | Uncommon | Bird | Nocturnal anomaly tracker. |
| 120 | Wildroot Colossus | World Tree | Legendary | Forest Ecosystem Embodiment | Titan | Mythic | Quadruped | Ancient titan embodying forest ecosystems. |

## Storm / Sky Line: 121-130

| # | Name | Habitat | Role | Empire Job | Size | Rarity | Body Type | Lore Summary |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 121 | Floatwhale | High Atmosphere | Transport/Base | Airborne Ecosystem, Sky Settlement | Titan | Epic | Serpent | Massive floating sky creature supporting airborne ecosystems. |
| 122 | Cyclone Serpent | Storm Corridors | DPS | Wind Storm Generation, Aerial Combat | Large | Rare | Serpent | Aerial predator generating wind storms. |
| 123 | Static Falcon | Storm Regions | Courier | High-Speed Courier, Message Delivery | Small | Uncommon | Bird | High-speed courier species. |
| 124 | Gale Wolf | Windscar Cliffs | DPS | Wind-Enhanced Pursuit, Pack Hunt | Medium | Uncommon | Quadruped | Wind-enhanced pursuit predator. |
| 125 | Thunder Ape | Storm Peaks | Tank/DPS | Storm Combat, Heavy Assault | Large | Rare | Biped | Storm-powered heavy combat beast. |
| 126 | Nimbus Tortoise | Sky Islands | Base/Support | Floating Fortress, Sky Settlement | Titan | Epic | Quadruped | Floating fortress creature supporting sky settlements. |
| 127 | Arc Moth | Storm Regions | Utility | Electrical Energy Harvesting | Small | Common | Bird | Electrical energy harvesting species. |
| 128 | Skyfang Lion | Cloud Peaks | DPS/Alpha | Sky Apex Predator, Territory Control | Large | Rare | Quadruped | Sky apex predator. |
| 129 | Tempest Whale | Global Migration | Legendary | Atmospheric Migration, World Event | Titan | Epic | Serpent | Atmospheric migration titan. |
| 130 | Skybreaker Wyvern | Storm Eye | Raid Boss | Raid: Flying Predator | Titan | Mythic | Bird | Raid-level flying predator. |

## Volcanic / Desert Line: 131-140

| # | Name | Habitat | Role | Empire Job | Size | Rarity | Body Type | Lore Summary |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 131 | Ash Drake | Ashfall Crater | DPS | Fire Combat, Volcanic Hunting | Large | Uncommon | Quadruped | Fire-breathing volcanic hunter. |
| 132 | Ember Titan | Deep Volcanic | Boss | Regional Boss: Magma Infused | Titan | Epic | Quadruped | Massive magma-infused regional threat. |
| 133 | Lava Crawler | Magma Crust | Swarm | Swarm Combat, Lava Surface | Small | Common | Quadruped | Swarm creature living beneath magma crust. |
| 134 | Magma Serpent | Volcanic Tunnels | DPS | Tunnel Combat, Volcanic Predator | Large | Rare | Serpent | Tunnel-dwelling volcanic predator. |
| 135 | Obsidian Rhino | Volcanic Plains | Tank | Heavy Armor, Heat Resistance | Large | Rare | Quadruped | Heavy armored heat-resistant beast. |
| 136 | Sand Strider | Scorched Desert | Mount | High-Speed Desert Travel | Medium | Uncommon | Quadruped | High-speed desert traversal mount. |
| 137 | Dune Manta | Desert Thermals | Transport | Gliding Desert Transport | Large | Uncommon | Bird | Gliding desert creature using thermal currents. |
| 138 | Scorch Hound | Volcanic Edge | DPS | Aggressive Fire Combat | Medium | Common | Quadruped | Aggressive fire predator. |
| 139 | Molten Beetle | Volcanic Vents | Industrial | Heat Processing, Industrial Insect | Small | Common | Quadruped | Heat-processing industrial insect. |
| 140 | Phoenix Wyrm | Volcanic Core | Legendary | Legendary: Rebirth Entity | Titan | Mythic | Serpent | Legendary rebirth entity tied to volcanic cycles. |

## Void / Dimensional Line: 141-151

| # | Name | Habitat | Role | Empire Job | Size | Rarity | Body Type | Lore Summary |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 141 | Echo Beast | Dimensional Nexus | Companion | Primary Story Companion, Dimensional Bond | Medium | Rare | Quadruped | Primary dimensional companion species tied to the main story. |
| 142 | Veil Serpent | Dimensional Tears | Guardian | Tear Guarding, Dimensional Patrol | Large | Epic | Serpent | Ancient entity guarding dimensional tears. |
| 143 | Reality Phantom | Between Realities | Assassin | Stealth Between Realities, Phase Kill | Medium | Epic | Quadruped | Stealth creature phasing between realities. |
| 144 | Void Leech | Corrupted Zones | Utility | Corruption Consumption, Purification | Small | Uncommon | Serpent | Corruption-consuming organism. |
| 145 | Fractured Titan | Collapse Zones | World Event | World Event: Terrain Collapse | Titan | Mythic | Quadruped | World-event creature causing terrain collapse. |
| 146 | Rift Hound | Portal Zones | Scout | Portal Tracking, Dimensional Hunt | Medium | Rare | Quadruped | Portal-tracking predator. |
| 147 | Entropy Drake | Chaos Zones | DPS | Chaos Combat, Dimensional Dragon | Large | Epic | Serpent | Chaos-aligned dimensional dragon. |
| 148 | Null Manta | Cosmic Void | Support | Gravity Distortion, Cosmic Travel | Large | Epic | Bird | Gravity-distorting cosmic creature. |
| 149 | Astral Serpent | Beyond The Veil | Legendary | Celestial Link, Star Connection | Titan | Epic | Serpent | Celestial entity linked to stars beyond The Veil. |
| 150 | Worldweaver | Reality Core | Legendary | Legendary: Reality Healing, Biome Stabilization | Titan | Mythic | Quadruped | Reality-healing titan capable of stabilizing biomes. |
| 151 | The Veilmaker | Origin Point | Mythic Creator | Mythic: Dimension Sealing, Creation Entity | Titan | Mythic | Quadruped | Mythic creator entity believed responsible for sealing dimensions. |

## Empire Job Categories

All Eidolons can be assigned to empire jobs based on their abilities.

| Job Category | Description | Example Eidolons |
| --- | --- | --- |
| Gathering | Collect resources such as wood, ore, crystal, and herbs | Ember Fox, Crystal Vulpine, Ember Ant |
| Mining | Break and process stone, ore, and crystal | Ashfang, Titan Beetle, Quarry Tortoise |
| Farming | Grow crops, fertilize soil, irrigate fields | Mossfire Fox, Bloom Ape, Rootback |
| Fishing | Catch aquatic resources | Tideborn, Reef Grazer, Glowjaw Eel |
| Power | Generate or transfer electricity | Volter Fox, Volt Ox, Pulse Ram |
| Construction | Build, repair, or move structures | Ironhide Mammoth, Bronzeback Ape |
| Transport | Move cargo, workers, or military units | Stonehorn Yak, Shellback Grazer, Sky Manta, Pearl Manta |
| Scouting | Explore and detect threats or resources | Aero Finch, Voidtail Fox, Rift Hound |
| Combat | Fight enemies and defend bases | Ashfang, Ironclaw Tiger, Storm Saber |
| Healing | Restore health or cure status effects | Spirit Ember, Glow Antler |
| Processing | Refine raw materials into usable resources | Ashfang, Furnace Boar, Magma Crawler |
| Patrol | Automated area defense | Hollow Wolf, Duskrunner, Shadow Prowler |
| Communication | Relay signals between bases or worlds | Static Finch, Echo Raven |
| Weather | Stabilize or control weather conditions | Tempest Seraph, Nimbus Crane |
| Portal | Detect, stabilize, or disrupt dimensional rifts | Rift Fox, Echo Raven, Rift Hound |

## Asset Generation Pipeline

### Blender MCP Batch Recipe

Each creature needs a blockout mesh generated with:

```text
blender_generate_creature_blockout(body_type, size, name)
```

| Body Type | Eidolons Using It | Approximate Count |
| --- | --- | --- |
| Quadruped | Fox, cat, wolf, bear, boar, elk, tiger, mammoth, etc. | 95 |
| Bird | Finch, owl, falcon, roc, sky manta, etc. | 25 |
| Serpent | Fish, eel, serpent, wyrm, leviathan, etc. | 25 |
| Biped | Ape, Thunder Ape, Static Ape, Bloom Ape | 6 |

### Size Scale Reference

| Size Tier | Scale Value | Examples |
| --- | --- | --- |
| Tiny | 0.3 | Ember Ant, Aero Finch, Arc Moth |
| Small | 0.5-0.7 | Ember Fox, Strayn Claw, Tideborn |
| Medium | 0.8-1.2 | Ashfang, Hollow Wolf, Rift Panther |
| Large | 1.5-2.5 | Ironclaw Tiger, Thunder Roc, Brine Drake |
| Titan | 3.0-5.0 | Rootback, Ironhide Mammoth, World Roc, Worldweaver |

### Material Slot Standard

Every Eidolon mesh requires these material slots:

1. Body: base body material
2. Glow/Element: elemental effect overlay such as fire, electric, void, or crystal
3. Eyes: emissive eye material
4. Accent: variant-specific detail such as armor plates, crystals, or fur patterns

### VFX Attach Point Standard

Every Eidolon skeleton requires these sockets:

- `Mouth`: breath and bite attacks
- `Spine_Mid`: aura and buff effects
- `Tail_Tip`: trail effects
- `Paw_FL`: front-left ground impact effects
- `Paw_FR`: front-right ground impact effects
- `Head`: status indicators

## Spawn Table Integration

Eidolons spawn based on biome assignment in the biome data `ValidCreatures` array.

| Biome | Common Spawns | Uncommon Spawns | Rare Spawns | Legendary Spawns |
| --- | --- | --- | --- | --- |
| Fractured Forest | Ember Fox, Hollow Wolf, Thornfur Fox | Mossfire Fox, Duskflare, Barkhide Elk | Spirit Ember, Mossfang Bear | Wildroot Colossus |
| Ashfall Crater | Scorch Hound, Lava Crawler, Ash Vulture | Ashfang, Ashmane Cougar, Molten Beetle | Cinder Warden, Obsidian Rhino | Phoenix Wyrm |
| Stormbreak | Storm Crow, Arc Moth, Static Finch | Volter Fox, Arc Talon, Gale Wolf | Thunder Roc, Tempest Seraph | Skybreaker Wyvern |
| Crystal Basin | Crystal Eel | Crystal Fang, Crystal Vulpine, Prism Falcon | None assigned | None assigned |
| Void Rift | Void Leech, Rift Hound | Voidtail Fox, Rift Panther, Voidwing | Echo Beast, Reality Phantom | Fractured Titan, Worldweaver |
| Deep Ocean | Reef Grazer, Glowjaw Eel, Tide Crawler | Coral Serpent, Abyss Ray, Mirefin | Stormfin Shark, Brine Drake | Umbra Leviathan |

## MVP Eidolon Set

The full roster is long-term scope. For the first playable RTS prototype, use a tiny slice:

| MVP Name | Source Roster Equivalent | Type | First Use |
| --- | --- | --- | --- |
| Ember Fox | Ember Fox | Fire | Scout/combat starter, furnace support |
| Stoneback | Quarry Tortoise or Stonehorn Yak | Earth | Mining and construction support |
| Sparkling | Volter Fox, Arc Moth, or Volt Ox | Electric | Building power and portal support |
| Waterhorn | Tideborn or Reef Grazer | Water | Food economy and healing support |
| Rootling | Mossfire Fox or Bloom Ape | Nature | Farming, wood, regeneration |

The MVP names from the build summary can either remain simplified prototype names or become early-stage forms in the final 151 roster.

## Design Notes

- The complete roster is a content bible, not the first production target.
- Titans and Mythic Eidolons should be treated as world events, bosses, campaign anchors, or late-game strategic objectives.
- Common and Uncommon Eidolons should drive day-to-day RTS economy decisions.
- Rare Eidolons should create map-control pressure.
- Epic, Mythic, and Legendary Eidolons should change the shape of a match when discovered.
- Each faction should interact with Eidolons differently through bonding, exploitation, corruption, research, or automation.

## End State

The full Eidolon system should make creatures feel like living infrastructure:

- They are resources.
- They are workers.
- They are allies.
- They are weapons.
- They are environmental forces.
- They are part of the ancient mystery of the Veil.

The player should not only ask, "Which army should I build?"

The player should also ask, "What kind of relationship will my civilization have with the living world?"
