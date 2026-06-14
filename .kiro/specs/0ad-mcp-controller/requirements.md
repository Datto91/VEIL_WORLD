# Requirements Document

## Introduction

This feature integrates a Model Context Protocol (MCP) server directly into the 0 A.D. open-source real-time strategy game engine. The MCP server enables an external AI agent to observe full game state and execute game commands as an autonomous player. The implementation is an in-process C++ module compiled into the game engine, communicating with AI agents over a network transport without any inter-process communication bridge.

## Glossary

- **MCP_Server**: The in-process C++ Model Context Protocol server module embedded within the 0 A.D. game engine that accepts connections from AI agents and exposes game state reading and command execution capabilities.
- **Game_Engine**: The 0 A.D. C++ game engine (Pyrogenesis) including its SpiderMonkey JavaScript scripting layer, simulation systems, and rendering pipeline.
- **AI_Agent**: An external software client that connects to the MCP_Server to read game state and issue game commands as an autonomous player.
- **Game_State**: The complete set of observable information about the current game including unit positions, building statuses, resource counts, map terrain, technology tree progress, and player information.
- **Command**: A game action issued by the AI_Agent through the MCP_Server that affects the simulation, such as training units, moving entities, attacking targets, constructing buildings, researching technologies, or setting gather points.
- **Session**: An active connection between an AI_Agent and the MCP_Server during which the AI_Agent can read Game_State and issue Commands.
- **Simulation_Component**: A subsystem within the Game_Engine responsible for simulating game logic including entity management, pathfinding, combat, resource gathering, and technology research.
- **Build_System**: The compilation and linking toolchain used to produce the 0 A.D. executable on Windows, including Visual Studio, premake, and associated dependencies.

## Requirements

### Requirement 1: Vanilla Build Compilation

**User Story:** As a developer, I want to compile the unmodified 0 A.D. source code on Windows, so that I have a validated toolchain baseline before adding MCP modifications.

#### Acceptance Criteria

1. THE Build_System SHALL produce a runnable 0 A.D. executable from the unmodified source repository on Windows using Visual Studio.
2. WHEN the build completes successfully, THE Build_System SHALL generate all required libraries and assets without manual intervention beyond initial dependency setup.
3. IF the build encounters missing dependencies, THEN THE Build_System SHALL report the specific missing component and expected location.

### Requirement 2: MCP Server Initialization

**User Story:** As a developer, I want the MCP server to start automatically when the game launches, so that AI agents can connect without manual server startup.

#### Acceptance Criteria

1. WHEN the Game_Engine starts, THE MCP_Server SHALL initialize and begin listening for incoming connections on a configurable network port.
2. THE MCP_Server SHALL operate in-process within the Game_Engine address space without spawning separate processes or using inter-process communication.
3. IF the configured port is unavailable, THEN THE MCP_Server SHALL log an error message indicating the port conflict and continue Game_Engine startup without the MCP_Server active.
4. WHERE a configuration file specifies the MCP_Server port, THE MCP_Server SHALL use the configured port value.

### Requirement 3: Game State Reading - Units

**User Story:** As an AI agent, I want to read the current state of all units on the map, so that I can make informed tactical decisions.

#### Acceptance Criteria

1. WHEN the AI_Agent requests unit state, THE MCP_Server SHALL return the position, health, owner, type, and current action for each unit visible to the AI_Agent player.
2. THE MCP_Server SHALL provide unit data that reflects the current simulation tick without stale information from previous ticks.
3. WHILE a Session is active, THE MCP_Server SHALL respond to unit state queries within one simulation tick of the request.

### Requirement 4: Game State Reading - Buildings

**User Story:** As an AI agent, I want to read the state of all buildings, so that I can assess economic and military infrastructure.

#### Acceptance Criteria

1. WHEN the AI_Agent requests building state, THE MCP_Server SHALL return the position, health, owner, type, construction progress, and current production queue for each building visible to the AI_Agent player.
2. THE MCP_Server SHALL include garrison information for buildings that support garrisoned units.

### Requirement 5: Game State Reading - Resources

**User Story:** As an AI agent, I want to read current resource levels, so that I can plan economic strategy.

#### Acceptance Criteria

1. WHEN the AI_Agent requests resource state, THE MCP_Server SHALL return the current quantity of food, wood, stone, and metal held by the AI_Agent player.
2. THE MCP_Server SHALL include resource gathering rates for each resource type.
3. WHEN the AI_Agent requests resource locations, THE MCP_Server SHALL return positions and remaining quantities of resource deposits on the map visible to the AI_Agent player.

### Requirement 6: Game State Reading - Map Terrain

**User Story:** As an AI agent, I want to read map terrain data, so that I can plan pathfinding and base placement.

#### Acceptance Criteria

1. WHEN the AI_Agent requests terrain data, THE MCP_Server SHALL return terrain type, elevation, and passability information for the map.
2. THE MCP_Server SHALL provide terrain data in a grid format with configurable resolution.
3. THE MCP_Server SHALL include fog-of-war information indicating which areas are currently visible, explored, or unexplored for the AI_Agent player.

### Requirement 7: Game State Reading - Technology Tree

**User Story:** As an AI agent, I want to read the technology tree state, so that I can plan research priorities.

#### Acceptance Criteria

1. WHEN the AI_Agent requests technology state, THE MCP_Server SHALL return all available technologies, their research prerequisites, costs, and completion status for the AI_Agent player.
2. THE MCP_Server SHALL indicate which technologies are currently being researched and their progress percentage.

### Requirement 8: Game State Reading - Player Information

**User Story:** As an AI agent, I want to read information about all players in the game, so that I can assess diplomatic and competitive context.

#### Acceptance Criteria

1. WHEN the AI_Agent requests player information, THE MCP_Server SHALL return the civilization, team assignment, diplomatic stance, and population statistics for each player in the game.
2. THE MCP_Server SHALL include the AI_Agent player identifier so the AI_Agent can distinguish its own state from opponent state.

### Requirement 9: Command Execution - Unit Training

**User Story:** As an AI agent, I want to train units from production buildings, so that I can build armies and workers.

#### Acceptance Criteria

1. WHEN the AI_Agent issues a train command specifying a building and unit type, THE MCP_Server SHALL queue the unit for production in the specified building.
2. IF the AI_Agent player lacks sufficient resources for the train command, THEN THE MCP_Server SHALL return an error response indicating insufficient resources.
3. IF the specified building cannot produce the requested unit type, THEN THE MCP_Server SHALL return an error response indicating invalid production capability.

### Requirement 10: Command Execution - Unit Movement

**User Story:** As an AI agent, I want to move units to specified locations, so that I can maneuver forces strategically.

#### Acceptance Criteria

1. WHEN the AI_Agent issues a move command specifying one or more units and a target position, THE MCP_Server SHALL instruct the specified units to move to the target position.
2. THE MCP_Server SHALL support both single-unit and group movement commands.
3. WHEN the AI_Agent issues a move command with a formation type, THE MCP_Server SHALL apply the specified formation to the moving group.

### Requirement 11: Command Execution - Attack

**User Story:** As an AI agent, I want to order units to attack enemy targets, so that I can engage in combat.

#### Acceptance Criteria

1. WHEN the AI_Agent issues an attack command specifying one or more units and a target entity, THE MCP_Server SHALL instruct the specified units to attack the target.
2. WHEN the AI_Agent issues an attack-move command specifying units and a position, THE MCP_Server SHALL instruct the specified units to move to the position while engaging enemies encountered along the path.
3. IF the target entity does not exist or is not attackable, THEN THE MCP_Server SHALL return an error response indicating an invalid target.

### Requirement 12: Command Execution - Building Construction

**User Story:** As an AI agent, I want to place and construct buildings, so that I can develop base infrastructure.

#### Acceptance Criteria

1. WHEN the AI_Agent issues a build command specifying a building type, position, and builder units, THE MCP_Server SHALL place a building foundation at the specified position and assign the builders to construct the building.
2. IF the specified position is invalid for building placement, THEN THE MCP_Server SHALL return an error response indicating invalid placement.
3. IF the AI_Agent player lacks sufficient resources for the building, THEN THE MCP_Server SHALL return an error response indicating insufficient resources.

### Requirement 13: Command Execution - Technology Research

**User Story:** As an AI agent, I want to research technologies, so that I can unlock upgrades and new capabilities.

#### Acceptance Criteria

1. WHEN the AI_Agent issues a research command specifying a technology and a building, THE MCP_Server SHALL begin researching the specified technology at the specified building.
2. IF the prerequisites for the technology are not met, THEN THE MCP_Server SHALL return an error response indicating unmet prerequisites.
3. IF the building is already researching a technology, THEN THE MCP_Server SHALL return an error response indicating the building is occupied.

### Requirement 14: Command Execution - Gather Points

**User Story:** As an AI agent, I want to set rally points and gather points for buildings, so that I can direct newly produced units.

#### Acceptance Criteria

1. WHEN the AI_Agent issues a set-gather-point command specifying a building and a target position, THE MCP_Server SHALL set the gather point for the specified building to the target position.
2. WHEN the AI_Agent issues a set-gather-point command specifying a building and a target entity, THE MCP_Server SHALL set the gather point for the specified building to the target entity.

### Requirement 15: Command Execution - Resource Gathering

**User Story:** As an AI agent, I want to order units to gather resources, so that I can manage economic production.

#### Acceptance Criteria

1. WHEN the AI_Agent issues a gather command specifying one or more units and a resource entity, THE MCP_Server SHALL instruct the specified units to gather from the target resource.
2. IF the specified units cannot gather the target resource type, THEN THE MCP_Server SHALL return an error response indicating invalid gatherer capability.

### Requirement 16: Session Management

**User Story:** As an AI agent, I want to establish and maintain a connection session with the MCP server, so that I can interact with the game over time.

#### Acceptance Criteria

1. WHEN an AI_Agent connects to the MCP_Server, THE MCP_Server SHALL establish a Session and assign the AI_Agent to a specific player slot.
2. WHILE a Session is active, THE MCP_Server SHALL maintain connection state and process requests sequentially.
3. IF a Session connection is lost unexpectedly, THEN THE MCP_Server SHALL clean up session resources and log the disconnection event.
4. WHEN the AI_Agent sends a disconnect request, THE MCP_Server SHALL terminate the Session and release associated resources.

### Requirement 17: MCP Protocol Compliance

**User Story:** As a developer, I want the server to follow the MCP protocol specification, so that standard MCP clients can connect without custom adapters.

#### Acceptance Criteria

1. THE MCP_Server SHALL implement the Model Context Protocol message format for all request and response payloads.
2. THE MCP_Server SHALL expose game state reading capabilities as MCP resources.
3. THE MCP_Server SHALL expose command execution capabilities as MCP tools.
4. THE MCP_Server SHALL support the MCP capability negotiation handshake during Session establishment.

### Requirement 18: Error Handling and Resilience

**User Story:** As a developer, I want the MCP server to handle errors gracefully, so that the game remains stable regardless of AI agent behavior.

#### Acceptance Criteria

1. IF the AI_Agent sends a malformed request, THEN THE MCP_Server SHALL return a protocol-compliant error response without affecting Game_Engine stability.
2. IF an internal error occurs during command processing, THEN THE MCP_Server SHALL log the error details and return a generic error response to the AI_Agent.
3. THE MCP_Server SHALL validate all incoming commands against the current game state before forwarding commands to the Simulation_Component.
4. IF the AI_Agent sends commands at a rate exceeding the simulation tick rate, THEN THE MCP_Server SHALL queue commands and process them on subsequent ticks.

### Requirement 19: Thread Safety

**User Story:** As a developer, I want the MCP server to operate safely alongside the game engine threads, so that concurrent access does not cause crashes or data corruption.

#### Acceptance Criteria

1. THE MCP_Server SHALL synchronize access to Game_State data with the Game_Engine simulation thread to prevent data races.
2. THE MCP_Server SHALL process network communication on a dedicated thread separate from the Game_Engine simulation thread.
3. WHEN the MCP_Server reads Game_State, THE MCP_Server SHALL access a consistent snapshot of the simulation state from the most recently completed tick.

### Requirement 20: Configuration

**User Story:** As a developer, I want to configure MCP server parameters without recompiling, so that I can adjust behavior during testing.

#### Acceptance Criteria

1. THE MCP_Server SHALL read configuration values from a configuration file at startup.
2. THE MCP_Server SHALL support configuration of the listening port, maximum concurrent sessions, and logging verbosity level.
3. WHERE configuration values are not specified, THE MCP_Server SHALL use documented default values.
