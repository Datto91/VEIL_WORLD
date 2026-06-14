# VEIL//WORLD RTS Build Summary

Version: 0.1  
Engine Target: 0 A.D. fork / mod layer  
Purpose: Summarize the core game concept, MVP scope, major systems, and practical build order.

## Main Concept

VEIL//WORLD is an Age of Empires-style real-time strategy game built around interworld expansion, creature-powered civilization, and portal warfare.

Players begin on Earth Prime with a small settlement, gather resources, train units, research technologies, restore ancient Veil Gates, capture Eidolons, and expand into connected worlds. Each world adds new terrain, resources, dangers, Eidolons, and strategic pressure.

The game should feel like:

- Age of Empires-style RTS economy and combat
- Interdimensional empire building
- Creature-powered infrastructure
- Portal-based map control
- Ancient mystery layered under every match

The primary hook is that the battlefield expands during the match. A player may start on Earth Prime, discover a broken Veil Gate, restore it, and open access to a second world with rare resources, dangerous creatures, and new routes for attack or defense.

Portal control becomes as important as land control.

## Core Fantasy

The Veil is the membrane between realities. Long before the player begins, ancient civilizations mastered the Veil, built gateways between worlds, created Eidolons, engineered Titans, and manipulated reality itself.

Then the Fracture happened.

Portal networks collapsed. Worlds vanished. Titans awoke. Ancient machines went silent. The surviving civilizations now live among ruins they barely understand.

The player is not just commanding an army. The player represents a civilization deciding what it will become now that the worlds are reconnecting.

## Core Pillars

### 1. Familiar RTS Foundation

The basic gameplay loop should remain understandable to Age of Empires players:

- Gather resources
- Build economy
- Train units
- Research technologies
- Scout the map
- Expand territory
- Fight enemies
- Advance through tech stages
- Win through domination or strategic objectives

The strange parts of VEIL//WORLD should sit on top of a familiar RTS base.

### 2. Interworld Expansion

The map is not just one battlefield. It is a connected network of worlds.

Each world can have:

- Unique resources
- Unique Eidolons
- Unique terrain
- Unique environmental dangers
- Unique strategic value
- Dormant or active Veil Gates

Players must decide when to stay, expand, defend, invade, or abandon a world.

### 3. Eidolons as Living Infrastructure

Eidolons are not just creatures for combat. They are part of the economy, technology tree, and faction identity.

Eidolons can:

- Fight
- Work
- Gather
- Power buildings
- Boost production
- Stabilize portals
- Improve farming
- Improve mining
- Transport resources
- Support research

Examples:

- Electric Eidolons power workshops, gates, batteries, and advanced buildings.
- Earth Eidolons boost mining, construction, and defense.
- Water Eidolons improve farms, healing, and food systems.
- Fire Eidolons improve forges, siege production, and offensive power.
- Nature Eidolons improve farms, wood gathering, regeneration, and world health.
- Void Eidolons improve Veil Energy and advanced portal research, but carry instability risk.

Some factions bond with Eidolons. Some domesticate them. Some enslave them for higher output but risk rebellion.

### 4. Portal Warfare

Veil Gates are the central strategic mechanic.

Portals allow:

- Unit transfer
- Resource movement
- World expansion
- Enemy invasions
- Trade routes
- Strategic flanking
- Supply chain control

Portals can be:

- Discovered
- Restored
- Built
- Captured
- Upgraded
- Sabotaged
- Destroyed
- Destabilized

A player can lose a war by losing portal access even if their army is stronger.

### 5. AI-Playable Simulation

The game should be built so AI can play and test it without relying on the normal UI.

This is important because the long-term goal is an RTS that can be repeatedly simulated, analyzed, and improved with AI help.

The game should eventually support headless AI-vs-AI matches that track:

- Win rates
- Resource income
- Match length
- Unit usage
- Portal usage
- Time to first attack
- Time to first expansion
- Time to first portal capture
- Faction balance
- Stalemates
- Overpowered units
- Useless buildings
- Broken strategies

## Engine Foundation

Use 0 A.D. as the base. Do not build the RTS engine from scratch.

0 A.D. already provides:

- RTS camera
- Unit selection
- Pathfinding
- Resource gathering
- Building placement
- Combat
- AI opponents
- Map editor
- Multiplayer foundation
- Tech trees
- XML unit templates
- JavaScript simulation logic
- Mod support

The correct first approach is to build VEIL//WORLD as a 0 A.D. mod. Only modify the C++ engine when the mod layer cannot support a required feature.

## High-Level Systems To Build

1. VEIL//WORLD mod structure
2. Custom resources
3. Custom factions
4. Custom buildings
5. Custom units
6. Eidolon unit templates
7. Eidolon labor assignment system
8. Eidolon bond / loyalty system
9. Veil Energy resource system
10. Veil Gate / portal system
11. Multi-world map system
12. Portal-based unit travel
13. Portal-based supply chains
14. Tech tree / age progression
15. AI bot support
16. Balance analytics
17. Custom maps
18. Custom art and audio later

## MVP Scope

The first playable version should be small, testable, and fun before adding the full lore scope.

### Map

- One main world: Earth Prime
- One secondary world: Crystal Grove
- One or two Veil Gates connecting them
- A single large map divided into world regions

For the MVP, do not load separate maps for separate worlds. Place both worlds on one large map and use portal teleportation between regions.

### Factions

- Playable faction: Earthborn Coalition
- Enemy faction: Wild Covenant or Iron Dominion

Recommended first enemy: Wild Covenant, because it tests Eidolon capture and creature synergy earlier.

### Resources

Use these resources in the MVP:

- Food
- Wood
- Stone
- Metal
- Veil Energy

Save these for later:

- Knowledge
- Influence
- World Stability
- Titan Favor

### Buildings

Earthborn MVP buildings:

- Veil Camp / Town Center
- House
- Farm
- Lumber Camp
- Mine
- Barracks
- Archery Range
- Workshop
- Research Center
- Eidolon Sanctuary
- Veil Gate
- Watch Tower

### Units

Earthborn MVP units:

- Worker
- Scout
- Infantry
- Archer
- Cavalry
- Engineer
- Siege Cart
- Eidolon Tamer

### Eidolons

Start with five Eidolons:

| Eidolon | Type | Main Use |
| --- | --- | --- |
| Sparkling | Electric | Powers buildings, boosts production, stabilizes portals |
| Stoneback | Earth | Boosts mining, boosts construction, works as a defensive creature |
| Emberling | Fire | Boosts forge/workshop output, works as a fire combat unit |
| Waterhorn | Water | Boosts farms, supports healing or food economy |
| Rootling | Nature | Boosts farms/wood, supports regeneration |

### Victory Conditions

MVP victory should support:

- Destroy the enemy Veil Camp
- Control all major Veil Gates for a set duration

## Low-Level Build Requirements

### 1. Mod Folder Structure

Suggested structure:

```text
mods/veil_world/
  mod.json
  simulation/
    templates/
      units/
      structures/
      eidolons/
      resources/
      technologies/
    components/
      VeilGate.js
      EidolonWorker.js
      EidolonBond.js
      VeilEnergy.js
      PortalTravel.js
      WorldLayer.js
      MatchAnalytics.js
  maps/
    skirmishes/
    random/
  gui/
  art/
  audio/
  ai/
    veil_bot/
  data/
    factions/
    eidolons/
    worlds/
```

### 2. Resource System

For MVP, add or simulate:

- food
- wood
- stone
- metal
- veil_energy

Veil Energy should be used for:

- Portal activation
- Portal upkeep
- Eidolon abilities
- Advanced research
- Late-game units

Keep Veil Energy rare enough that portal control and Eidolon assignment matter.

### 3. Veil Gate Component

Build a custom portal component.

Required state:

```text
VeilGate
  gate_id
  destination_gate_id
  active
  owner
  stability
  veil_energy_cost
  capture_progress
  disabled_until
```

Required behavior:

- Activate and deactivate gates
- Link source and destination gates
- Transfer units
- Consume Veil Energy
- Allow capture
- Allow damage and repair
- Temporarily disable when sabotaged or destabilized

Core functions:

```text
activate_gate()
deactivate_gate()
link_gate(source_gate_id, destination_gate_id)
transfer_units(unit_ids, destination_gate_id)
capture_gate(player_id)
damage_gate(amount)
repair_gate(amount)
destabilize_gate(duration)
```

MVP rule: transferring units should teleport selected units from the source gate area to the destination gate area after a short delay.

### 4. Multi-World System

For MVP, use one large map divided into world regions.

Example:

- Earth Prime region on the left side of the map
- Crystal Grove region on the right side of the map
- Veil Gate connects the two regions

World region data:

```text
world_id
world_name
biome
resource_modifiers
eidolon_spawn_table
environment_effects
portal_list
```

MVP world effects should be simple:

- Earth Prime: normal economy
- Crystal Grove: more stone/metal, more Eidolons, slightly dangerous wildlife

Later versions can support true separate map instances.

### 5. Eidolon System

Eidolons need normal unit templates plus custom behavior.

Each Eidolon should have:

```text
eidolon_id
eidolon_type
tier
wild_state
owner
loyalty
bond_level
assigned_building
labor_role
combat_role
evolution_progress
rebellion_risk
```

Eidolon states:

- Wild
- Tamed
- Bonded
- Assigned
- Fighting
- Injured
- Rebellious

MVP Eidolon actions:

- Capture / tame
- Assign to building
- Unassign from building
- Fight as a unit
- Provide passive building bonus
- Gain loyalty over time

Later Eidolon actions:

- Evolve
- Breed
- Rebel
- Mutate
- Fuse with artifacts
- Stabilize or corrupt portals

### 6. Eidolon Labor Assignment

Buildings should support Eidolon slots.

Example:

```text
BuildingEidolonSlots
  max_slots
  accepted_types
  assigned_eidolons
  output_bonus
  upkeep_cost
  rebellion_modifier
```

Example assignments:

- Sparkling assigned to Workshop: production speed bonus
- Sparkling assigned to Veil Gate: reduced Veil Energy cost
- Stoneback assigned to Mine: metal/stone income bonus
- Emberling assigned to Workshop: siege production bonus
- Waterhorn assigned to Farm: food income bonus
- Rootling assigned to Lumber Camp or Farm: wood/food bonus

MVP should support simple percentage bonuses before adding complex abilities.

### 7. Earthborn Coalition

Earthborn Coalition is the first playable faction.

Identity:

- Human survival
- Rebuilding civilization
- Practical use of ancient technology
- Balanced RTS mechanics

Strengths:

- Reliable economy
- Flexible army
- Strong workers
- Good defenses
- Good beginner faction

Weaknesses:

- No extreme early-game power
- Average portal technology
- Average Eidolon synergy
- Less specialized than other factions

Unique mechanic for later:

- Adaptive Doctrine

Doctrine options:

- Frontier Doctrine
- Industrial Doctrine
- Military Doctrine
- Veil Recovery Doctrine

For MVP, Earthborn can ship without doctrines. Add doctrines after the first playable match works.

### 8. First Enemy Faction

Use Wild Covenant as the recommended first enemy faction.

Identity:

- Nature-aligned civilization
- Bonds with Eidolons instead of exploiting them
- Strong map control
- Strong creature synergy

MVP Wild Covenant can be partial:

- Basic worker
- Basic infantry
- Eidolon-heavy army
- Simple AI that captures nearby Eidolons and attacks through portals

Do not build the full Wild Covenant tech tree first.

### 9. AI Bot Support

The first AI does not need to be smart. It needs to be reliable enough to test the game loop.

MVP bot goals:

- Build workers
- Gather resources
- Build houses
- Train units
- Scout
- Expand toward gate
- Capture or attack portal
- Attack enemy base

Later AI goals:

- Decide when to restore gates
- Decide when to invade secondary worlds
- Assign Eidolons intelligently
- Retreat from losing worlds
- Sabotage portals
- Analyze match results

### 10. Match Analytics

Add lightweight match stats as early as possible.

Track:

- Match duration
- Winner
- Resources gathered
- Units trained
- Units lost
- Buildings built
- Gates activated
- Gates captured
- Eidolons captured
- Eidolons assigned
- First attack time
- First portal activation time

This will make balancing much easier later.

## Faction Roadmap

Full faction roster:

1. Earthborn Coalition
2. Veilborn Conclave
3. Iron Dominion
4. Wild Covenant
5. Void Kin
6. Ashen Empire
7. Crystal Synod
8. Machine Remnant

Implementation order:

1. Earthborn Coalition
2. Wild Covenant
3. Iron Dominion
4. Veilborn Conclave
5. Void Kin
6. Ashen Empire
7. Crystal Synod
8. Machine Remnant

Do not build all factions first. Build the faction framework, one complete faction, and one partial enemy faction.

## Campaign Direction

Planned campaign arc:

1. The First Gate
2. The Sleeping Titans
3. The Machine Awakening
4. The Void Breach
5. The Truth of the Fracture
6. Ascension

Campaigns should reveal the world gradually. The player should start with survival and rebuilding, then discover that the portal network, Eidolons, Titans, and ancient civilizations are all connected.

## Development Phases

### Phase 1: Baseline RTS Match

Goal: make a normal 0 A.D.-style match work with Earthborn units and buildings.

Build:

- Mod folder
- Earthborn templates
- Basic economy
- Basic military
- Basic AI enemy
- One playable map

Done when:

- Player can gather resources
- Player can build a base
- Player can train units
- Player can fight and win

### Phase 2: Veil Gate Prototype

Goal: make portals work.

Build:

- Veil Gate structure
- Portal linking
- Unit transfer
- Gate activation cost
- Gate capture behavior

Done when:

- Units can move between Earth Prime and Crystal Grove through a gate
- Player can fight over portal control
- Losing a gate matters strategically

### Phase 3: Eidolon Prototype

Goal: make Eidolons useful in economy and combat.

Build:

- Five MVP Eidolons
- Eidolon Tamer
- Capture/tame behavior
- Building assignment slots
- Simple output bonuses

Done when:

- Player can capture an Eidolon
- Player can assign it to a building
- Assignment changes economy in a visible way
- Eidolons can also fight

### Phase 4: First Fun Match

Goal: make the full MVP loop enjoyable.

Build:

- Earth Prime and Crystal Grove map
- Gate-control victory condition
- Partial Wild Covenant enemy
- Basic match analytics
- Balance pass on resource rates, training times, and portal value

Done when:

- A match has a clear early, mid, and late game
- The second world is worth fighting over
- Eidolons are worth capturing
- Portals create interesting attacks and defenses

### Phase 5: Expand Depth

Goal: deepen the game only after the MVP works.

Add:

- Earthborn doctrines
- More Eidolons
- Iron Dominion
- Better AI
- More maps
- More portal mechanics
- More world effects
- More research
- Better art/audio

## Design Warnings

Do not try to implement the full dream version first.

The full concept includes many worlds, Titans, eight factions, 151 Eidolons, empire building, portal networks, advanced AI, faction ascension, mining, cargo systems, farming, terraforming, and large-scale warfare. That is too much for the first build.

The correct order is:

1. Build one faction fully.
2. Build one enemy faction partially.
3. Make a fun normal match.
4. Add portals.
5. Add Eidolons.
6. Add a second world.
7. Make that loop fun.
8. Then expand.

A boring game with eight factions is worse than a fun game with one faction.

## Main Build Target

The first successful version of VEIL//WORLD should prove this loop:

1. Start on Earth Prime.
2. Build a small base.
3. Scout the map.
4. Find Eidolons.
5. Capture and assign Eidolons.
6. Restore a Veil Gate.
7. Enter Crystal Grove.
8. Fight over rare resources and gate control.
9. Defend both sides of the portal network.
10. Win by destroying the enemy base or controlling the major gates.

Once that loop is fun, the larger VEIL//WORLD vision becomes much more realistic.
