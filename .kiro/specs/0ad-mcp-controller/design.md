# Design Document: 0 A.D. MCP Controller

## Overview

This document describes the technical architecture of an in-process Model Context Protocol (MCP) server embedded within the 0 A.D. (Pyrogenesis) game engine. The server exposes full game state reading and command execution to external AI agents over network transport, enabling autonomous AI gameplay through the standardized MCP protocol.

## Architecture

### High-Level Architecture

```
┌──────────────────────────────────────────────────────────┐
│                   0 A.D. Game Engine                       │
│                                                           │
│  ┌─────────────────┐    ┌──────────────────────────────┐ │
│  │  Simulation      │    │      MCP Server Module        │ │
│  │  Thread          │    │                              │ │
│  │  ┌────────────┐  │    │  ┌──────────┐ ┌───────────┐ │ │
│  │  │ Components │  │◄──►│  │ State    │ │ Command   │ │ │
│  │  │ (JS/C++)   │  │    │  │ Reader   │ │ Executor  │ │ │
│  │  └────────────┘  │    │  └──────────┘ └───────────┘ │ │
│  │  ┌────────────┐  │    │  ┌──────────┐ ┌───────────┐ │ │
│  │  │ Entity     │  │    │  │ Session  │ │ Protocol  │ │ │
│  │  │ Manager    │  │    │  │ Manager  │ │ Handler   │ │ │
│  │  └────────────┘  │    │  └──────────┘ └───────────┘ │ │
│  └─────────────────┘    │  ┌──────────────────────────┐ │ │
│                          │  │  Network Thread           │ │ │
│                          │  │  (stdio / SSE transport)  │ │ │
│                          │  └──────────────────────────┘ │ │
│                          └──────────────────────────────┘ │
└──────────────────────────────────────────────────────────┘
         ▲                              ▲
         │ (game window/input)          │ (JSON-RPC 2.0)
         ▼                              ▼
      Human Player                   AI Agent
```

### Threading Model

The system operates on three threads:

1. **Simulation Thread** — The existing 0 A.D. deterministic tick-based simulation. Owns all game state and processes commands during tick updates.
2. **Network Thread** — Handles TCP socket accept/read/write for AI agent connections. Parses JSON-RPC messages and enqueues them for processing.
3. **Main/Render Thread** — The existing engine main loop. The MCP module hooks into the engine startup/shutdown lifecycle on this thread.

### Synchronization Strategy

```cpp
// Tick-boundary snapshot approach
class MCPGameStateSnapshot {
    // Immutable copy of game state taken at end of each simulation tick
    // Network thread reads from snapshot freely (no locks during read)
    // Simulation thread writes a new snapshot at tick boundary (swap pointer under lock)

    std::shared_ptr<const GameStateData> m_currentSnapshot;
    mutable std::shared_mutex m_snapshotMutex;

public:
    // Called by simulation thread at end of tick
    void updateSnapshot(std::shared_ptr<const GameStateData> newState) {
        std::unique_lock lock(m_snapshotMutex);
        m_currentSnapshot = std::move(newState);
    }

    // Called by network thread to serve AI agent queries
    std::shared_ptr<const GameStateData> getSnapshot() const {
        std::shared_lock lock(m_snapshotMutex);
        return m_currentSnapshot;
    }
};
```

Commands flow in the opposite direction through a thread-safe queue:

```cpp
// Lock-free SPSC queue for commands (network thread produces, sim thread consumes)
class MCPCommandQueue {
    moodycamel::ConcurrentQueue<MCPCommand> m_queue;

public:
    // Called by network thread after validating a command
    void enqueue(MCPCommand cmd) {
        m_queue.enqueue(std::move(cmd));
    }

    // Called by simulation thread at start of each tick
    std::vector<MCPCommand> drainAll() {
        std::vector<MCPCommand> commands;
        MCPCommand cmd;
        while (m_queue.try_dequeue(cmd)) {
            commands.push_back(std::move(cmd));
        }
        return commands;
    }
};
```

## Components

### 1. MCPModule (Lifecycle Manager)

Responsibilities:
- Registers with the engine startup/shutdown hooks
- Reads configuration file
- Initializes all MCP subsystems
- Graceful shutdown on engine exit

```cpp
class MCPModule {
public:
    static MCPModule& getInstance();

    void initialize(const MCPConfig& config);
    void shutdown();
    bool isActive() const;

private:
    std::unique_ptr<MCPNetworkTransport> m_transport;
    std::unique_ptr<MCPSessionManager> m_sessionManager;
    std::unique_ptr<MCPStateReader> m_stateReader;
    std::unique_ptr<MCPCommandExecutor> m_commandExecutor;
    std::unique_ptr<MCPGameStateSnapshot> m_snapshot;
    std::unique_ptr<MCPCommandQueue> m_commandQueue;
    MCPConfig m_config;
    bool m_active = false;
};
```

### 2. MCPNetworkTransport

Responsibilities:
- Listens on configured TCP port for incoming connections
- Implements stdio and HTTP+SSE transport per MCP specification
- Reads/writes JSON-RPC 2.0 framed messages
- Runs on dedicated network thread

```cpp
class MCPNetworkTransport {
public:
    void start(uint16_t port);
    void stop();
    void sendResponse(SessionId session, const json& response);
    void setRequestHandler(std::function<void(SessionId, const json&)> handler);

private:
    std::thread m_networkThread;
    uint16_t m_port;
    std::vector<TCPConnection> m_connections;
    // Platform: Windows Winsock2 API
};
```

### 3. MCPSessionManager

Responsibilities:
- Manages active AI agent sessions
- Assigns sessions to player slots
- Enforces max concurrent session limit
- Handles MCP capability negotiation handshake
- Cleans up on disconnect

```cpp
struct MCPSession {
    SessionId id;
    PlayerId assignedPlayer;
    ConnectionState state;
    std::chrono::steady_clock::time_point connectedAt;
};

class MCPSessionManager {
public:
    SessionId createSession(PlayerId player);
    void destroySession(SessionId id);
    MCPSession* getSession(SessionId id);
    size_t activeSessionCount() const;

private:
    std::unordered_map<SessionId, MCPSession> m_sessions;
    size_t m_maxSessions;
    std::mutex m_sessionMutex;
};
```

### 4. MCPStateReader

Responsibilities:
- Reads from the game state snapshot (thread-safe, no simulation lock needed)
- Translates internal game data structures into MCP resource responses
- Applies fog-of-war filtering per requesting player
- Handles all game state query types: units, buildings, resources, terrain, tech tree, player info

```cpp
class MCPStateReader {
public:
    json queryUnits(PlayerId viewer) const;
    json queryBuildings(PlayerId viewer) const;
    json queryResources(PlayerId viewer) const;
    json queryResourceLocations(PlayerId viewer) const;
    json queryTerrain(PlayerId viewer, int resolution) const;
    json queryTechTree(PlayerId viewer) const;
    json queryPlayerInfo() const;

private:
    MCPGameStateSnapshot& m_snapshot;
    // Fog-of-war filtering helpers
    bool isEntityVisible(EntityId entity, PlayerId viewer) const;
};
```

### 5. MCPCommandExecutor

Responsibilities:
- Validates commands against current game state before queuing
- Translates MCP tool calls into simulation commands
- Returns validation errors immediately (before queuing)
- Enqueues valid commands for next simulation tick

```cpp
enum class CommandValidationResult {
    Valid,
    InsufficientResources,
    InvalidTarget,
    InvalidPlacement,
    UnmetPrerequisites,
    BuildingOccupied,
    InvalidGatherer,
    InvalidProduction,
    UnknownCommand
};

class MCPCommandExecutor {
public:
    CommandValidationResult validate(const MCPCommand& cmd, const GameStateData& state);
    void execute(MCPCommand cmd); // Enqueues to command queue

    // Command types
    json handleTrain(PlayerId player, EntityId building, const std::string& unitType);
    json handleMove(PlayerId player, std::vector<EntityId> units, Vec3 target, std::optional<std::string> formation);
    json handleAttack(PlayerId player, std::vector<EntityId> units, EntityId target);
    json handleAttackMove(PlayerId player, std::vector<EntityId> units, Vec3 target);
    json handleBuild(PlayerId player, const std::string& buildingType, Vec3 position, std::vector<EntityId> builders);
    json handleResearch(PlayerId player, const std::string& techId, EntityId building);
    json handleSetGatherPoint(PlayerId player, EntityId building, std::variant<Vec3, EntityId> target);
    json handleGather(PlayerId player, std::vector<EntityId> units, EntityId resource);

private:
    MCPGameStateSnapshot& m_snapshot;
    MCPCommandQueue& m_commandQueue;
};
```

### 6. MCPProtocolHandler

Responsibilities:
- Parses incoming JSON-RPC 2.0 messages
- Routes requests to appropriate handler (state reader or command executor)
- Formats responses per MCP specification
- Handles capability negotiation
- Maps errors to JSON-RPC error codes

```cpp
class MCPProtocolHandler {
public:
    json handleRequest(SessionId session, const json& request);
    json buildCapabilitiesResponse();
    json buildErrorResponse(int code, const std::string& message, const json& id);

private:
    // MCP resource handlers (game state)
    json handleResourceRead(SessionId session, const std::string& uri);
    // MCP tool handlers (commands)
    json handleToolCall(SessionId session, const std::string& toolName, const json& args);

    MCPStateReader& m_stateReader;
    MCPCommandExecutor& m_commandExecutor;
    MCPSessionManager& m_sessionManager;
};
```

### 7. MCPSimulationHook

Responsibilities:
- Hooks into the simulation tick loop
- At end of tick: captures game state snapshot
- At start of tick: drains command queue and injects commands into simulation
- Bridges between MCP module and SpiderMonkey JS engine via existing component interfaces

```cpp
class MCPSimulationHook {
public:
    // Called at the start of each simulation tick
    void onTickStart(CSimulation2& sim);
    // Called at the end of each simulation tick
    void onTickEnd(CSimulation2& sim);

private:
    void captureSnapshot(CSimulation2& sim);
    void injectCommands(CSimulation2& sim, const std::vector<MCPCommand>& commands);

    MCPGameStateSnapshot& m_snapshot;
    MCPCommandQueue& m_commandQueue;
};
```

## Interfaces

### MCP Resources (Game State Reading)

Resources are exposed as MCP resource URIs:

| Resource URI | Description | Required Fields |
|---|---|---|
| `game://units` | All visible units | position, health, owner, type, currentAction |
| `game://buildings` | All visible buildings | position, health, owner, type, constructionProgress, productionQueue, garrison |
| `game://resources` | Player resource levels | food, wood, stone, metal, gatheringRates |
| `game://resources/map` | Visible resource deposits | position, remainingQuantity, resourceType |
| `game://terrain` | Map terrain data | type, elevation, passability, fogOfWar (grid format) |
| `game://tech` | Technology tree state | techId, prerequisites, cost, status, researchProgress |
| `game://players` | All player info | civilization, team, diplomacy, population, selfPlayerId |

### MCP Tools (Command Execution)

Tools are exposed as MCP tool definitions:

| Tool Name | Parameters | Description |
|---|---|---|
| `train_unit` | buildingId, unitType | Queue unit production |
| `move_units` | unitIds[], targetPosition, formation? | Move units to position |
| `attack_target` | unitIds[], targetId | Attack an entity |
| `attack_move` | unitIds[], targetPosition | Attack-move to position |
| `build_structure` | buildingType, position, builderIds[] | Place and construct building |
| `research_tech` | techId, buildingId | Begin technology research |
| `set_gather_point` | buildingId, target (position or entityId) | Set building rally point |
| `gather_resource` | unitIds[], resourceId | Gather from resource |

### JSON-RPC 2.0 Message Format

Request example:
```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "tools/call",
    "params": {
        "name": "move_units",
        "arguments": {
            "unitIds": [42, 43, 44],
            "targetPosition": {"x": 150.0, "y": 0.0, "z": 200.0},
            "formation": "line"
        }
    }
}
```

Success response:
```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
        "content": [
            {
                "type": "text",
                "text": "{\"status\": \"accepted\", \"tickQueued\": 1042}"
            }
        ]
    }
}
```

Error response:
```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "error": {
        "code": -32602,
        "message": "Insufficient resources",
        "data": {"required": {"food": 100}, "available": {"food": 50}}
    }
}
```

## Data Models

### GameStateData (Snapshot)

```cpp
struct UnitState {
    EntityId id;
    Vec3 position;
    int health;
    int maxHealth;
    PlayerId owner;
    std::string templateName; // unit type
    std::string currentAction; // idle, moving, attacking, gathering, building
};

struct BuildingState {
    EntityId id;
    Vec3 position;
    int health;
    int maxHealth;
    PlayerId owner;
    std::string templateName;
    float constructionProgress; // 0.0 to 1.0
    std::vector<ProductionQueueEntry> productionQueue;
    std::vector<EntityId> garrisonedUnits;
    bool supportsGarrison;
};

struct ProductionQueueEntry {
    std::string unitType;
    float progress; // 0.0 to 1.0
    int timeRemaining; // in simulation ticks
};

struct PlayerResources {
    int food;
    int wood;
    int stone;
    int metal;
    float foodRate;
    float woodRate;
    float stoneRate;
    float metalRate;
};

struct ResourceDeposit {
    EntityId id;
    Vec3 position;
    std::string resourceType; // food, wood, stone, metal
    int remainingQuantity;
};

struct TerrainCell {
    std::string terrainType;
    float elevation;
    bool passable;
    FogOfWarState visibility; // VISIBLE, EXPLORED, UNEXPLORED
};

struct TechnologyState {
    std::string techId;
    std::string name;
    std::vector<std::string> prerequisites;
    PlayerResources cost;
    TechStatus status; // AVAILABLE, RESEARCHING, COMPLETED, LOCKED
    float researchProgress; // 0.0 to 1.0 when RESEARCHING
};

struct PlayerInfo {
    PlayerId id;
    std::string civilization;
    int team;
    std::string diplomacyStance; // ally, neutral, enemy
    int currentPopulation;
    int maxPopulation;
};

struct GameStateData {
    uint32_t tickNumber;
    std::vector<UnitState> units;
    std::vector<BuildingState> buildings;
    std::unordered_map<PlayerId, PlayerResources> playerResources;
    std::vector<ResourceDeposit> resourceDeposits;
    std::vector<std::vector<TerrainCell>> terrainGrid;
    int terrainResolution;
    std::unordered_map<PlayerId, std::vector<TechnologyState>> techTrees;
    std::vector<PlayerInfo> players;
};
```

### MCPCommand

```cpp
enum class MCPCommandType {
    TrainUnit,
    MoveUnits,
    AttackTarget,
    AttackMove,
    BuildStructure,
    ResearchTech,
    SetGatherPoint,
    GatherResource
};

struct MCPCommand {
    MCPCommandType type;
    PlayerId issuingPlayer;
    uint64_t requestId; // JSON-RPC request ID for response correlation
    json parameters; // Type-specific parameters
    std::chrono::steady_clock::time_point enqueuedAt;
};
```

### MCPConfig

```cpp
struct MCPConfig {
    uint16_t port = 6374;              // Default MCP listen port
    size_t maxSessions = 4;            // Max concurrent AI agent sessions
    int logVerbosity = 1;              // 0=error, 1=info, 2=debug
    std::string configFilePath = "mcp_config.json";

    static MCPConfig loadFromFile(const std::string& path);
    static MCPConfig defaults();
};
```

## Integration with 0 A.D. Engine

### Build System Integration

The MCP module is compiled as part of the Pyrogenesis engine build:

1. Source files placed in `source/mcp/` directory
2. Premake5 project file updated to include MCP source group
3. Link against: nlohmann/json (header-only), Winsock2 (Windows networking)
4. No external runtime dependencies beyond what 0 A.D. already requires

### Simulation Hook Registration

The MCP module hooks into the simulation via the existing `CSimulation2` class:

```cpp
// In simulation initialization (called once per game start)
void CSimulation2::InitMCPHook() {
    if (MCPModule::getInstance().isActive()) {
        m_mcpHook = std::make_unique<MCPSimulationHook>(
            MCPModule::getInstance().getSnapshot(),
            MCPModule::getInstance().getCommandQueue()
        );
    }
}

// In simulation tick loop
void CSimulation2::Update(int turnLength) {
    if (m_mcpHook) m_mcpHook->onTickStart(*this);
    // ... existing tick processing ...
    if (m_mcpHook) m_mcpHook->onTickEnd(*this);
}
```

### SpiderMonkey Bridge for Command Injection

Commands are injected into the simulation through the same interface used by the existing AI system (CmpAI). This ensures commands go through proper game rule validation:

```cpp
void MCPSimulationHook::injectCommands(CSimulation2& sim, const std::vector<MCPCommand>& commands) {
    for (const auto& cmd : commands) {
        // Use the simulation's existing PostCommand interface
        // This is the same path used by CmpAI for JS-based AI players
        JS::RootedValue cmdVal(sim.GetScriptInterface().GetContext());
        convertToJSCommand(sim.GetScriptInterface(), cmd, &cmdVal);
        sim.PostCommand(cmd.issuingPlayer, cmdVal);
    }
}
```

### State Capture from Components

Game state is read through the existing component query interfaces:

```cpp
void MCPSimulationHook::captureSnapshot(CSimulation2& sim) {
    auto state = std::make_shared<GameStateData>();
    state->tickNumber = sim.GetCurrentTurn();

    // Query position component for all entities
    CmpPtr<ICmpRangeManager> cmpRange(sim, SYSTEM_ENTITY);
    CmpPtr<ICmpTerritoryManager> cmpTerritory(sim, SYSTEM_ENTITY);

    // Iterate entities and read components
    const CSimulation2::InterfaceListUnordered& positions =
        sim.GetEntitiesWithInterface(IID_Position);
    for (auto& [entityId, component] : positions) {
        // Read ICmpPosition, ICmpHealth, ICmpOwnership, ICmpUnitAI, etc.
        // Populate UnitState or BuildingState
    }

    // Capture terrain grid
    CmpPtr<ICmpTerrain> cmpTerrain(sim, SYSTEM_ENTITY);
    // ... read terrain data ...

    m_snapshot.updateSnapshot(std::move(state));
}
```

## Error Handling

### Error Categories

| Category | JSON-RPC Code | Description |
|---|---|---|
| Parse Error | -32700 | Malformed JSON |
| Invalid Request | -32600 | Not a valid JSON-RPC 2.0 request |
| Method Not Found | -32601 | Unknown resource URI or tool name |
| Invalid Params | -32602 | Missing or invalid parameters |
| Game Rule Violation | -32000 | Insufficient resources, invalid target, etc. |
| Internal Error | -32603 | Unexpected server error |

### Error Handling Flow

```
AI Agent Request
    │
    ▼
[Parse JSON] ──── malformed ──── Error -32700
    │
    ▼
[Validate JSON-RPC] ── invalid ── Error -32600
    │
    ▼
[Route to Handler] ── unknown ── Error -32601
    │
    ▼
[Validate Params] ── invalid ── Error -32602
    │
    ▼
[Validate Game State] ── violation ── Error -32000
    │
    ▼
[Execute / Enqueue]
    │
    ▼
Success Response
```

### Resilience Guarantees

- No AI agent input can crash the game engine (all inputs validated before processing)
- Rate limiting: commands exceeding tick rate are queued, not dropped
- Session isolation: one misbehaving agent cannot affect another agent's session
- Network thread exceptions are caught and logged, never propagate to simulation thread

## Configuration

### Configuration File (mcp_config.json)

```json
{
    "port": 6374,
    "maxSessions": 4,
    "logVerbosity": 1
}
```

### Configuration Resolution Order

1. Configuration file values (if file exists and key is present)
2. Documented defaults (port=6374, maxSessions=4, logVerbosity=1)

### File Location

The configuration file is searched for in the 0 A.D. user data directory:
- Windows: `%APPDATA%\0ad\config\mcp_config.json`

## Development Sequence

### Phase 1: Vanilla Build Validation
1. Clone 0 A.D. source repository
2. Set up Visual Studio build environment with required dependencies
3. Build unmodified source to produce runnable executable
4. Verify game launches and runs correctly

### Phase 2: MCP Module Scaffold
1. Create `source/mcp/` directory structure
2. Add premake5 configuration for MCP source group
3. Implement MCPModule lifecycle (init/shutdown)
4. Implement MCPConfig loader
5. Verify engine builds and starts with empty MCP module

### Phase 3: Network Transport
1. Implement TCP listener on network thread (Winsock2)
2. Implement JSON-RPC 2.0 message framing
3. Implement MCP capability negotiation handshake
4. Implement session creation/destruction

### Phase 4: Game State Reading
1. Implement MCPSimulationHook snapshot capture
2. Implement MCPStateReader for each entity type
3. Implement fog-of-war filtering
4. Wire up MCP resource handlers

### Phase 5: Command Execution
1. Implement MCPCommandExecutor validation logic
2. Implement command queue and tick-boundary injection
3. Implement SpiderMonkey bridge for command posting
4. Wire up MCP tool handlers

### Phase 6: Integration Testing
1. Connect external MCP client
2. Verify full read/write cycle
3. Stress test with rapid command streams
4. Verify thread safety under load

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system — essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Game state response completeness

*For any* game state query of a given entity type (units, buildings, resources, terrain, tech tree, players), the response SHALL contain all required fields for every entity of that type visible to the requesting player, with no field missing or null unless the field is explicitly optional.

**Validates: Requirements 3.1, 4.1, 4.2, 5.1, 5.2, 5.3, 6.1, 7.1, 7.2, 8.1**

### Property 2: Visibility-filtered state access

*For any* game state query issued by an AI agent assigned to player P, the response SHALL contain only entities and terrain cells that are visible or explored according to player P's fog-of-war state, and SHALL NOT include any data from unexplored or invisible regions.

**Validates: Requirements 3.1, 5.3, 6.3**

### Property 3: Snapshot tick consistency

*For any* game state read that occurs while the simulation is processing tick N+1, the returned data SHALL reflect exactly the state at the end of tick N (the most recently completed tick), with no partial updates from the in-progress tick visible.

**Validates: Requirements 3.2, 19.3**

### Property 4: Valid command acceptance

*For any* command that passes game rule validation (sufficient resources, valid target, valid building capability, met prerequisites, available building), the MCP server SHALL accept the command and enqueue it for execution, and the game state after the next tick SHALL reflect the command's effect.

**Validates: Requirements 9.1, 10.1, 10.2, 10.3, 11.1, 11.2, 12.1, 13.1, 14.1, 14.2, 15.1**

### Property 5: Invalid command rejection with stable state

*For any* command that violates a game rule (insufficient resources, invalid target, invalid placement, unmet prerequisites, occupied building, invalid gatherer), the MCP server SHALL return a descriptive error response AND the game state SHALL remain unchanged — no side effects from the rejected command.

**Validates: Requirements 9.2, 9.3, 11.3, 12.2, 12.3, 13.2, 13.3, 15.2**

### Property 6: Malformed input resilience

*For any* byte sequence sent to the MCP server (including random binary data, truncated JSON, invalid JSON-RPC, and oversized payloads), the server SHALL return a protocol-compliant JSON-RPC error response and SHALL NOT crash, corrupt game state, or affect other active sessions.

**Validates: Requirements 18.1, 18.3**

### Property 7: MCP protocol message round-trip

*For any* valid MCP request message, serializing the request to JSON, transmitting it over the wire, and deserializing it on the server side SHALL produce a structurally identical request object. Similarly, the server's response serialized and deserialized by the client SHALL be structurally identical to the original response.

**Validates: Requirements 17.1**

### Property 8: Command ordering preservation

*For any* sequence of commands sent within a single session, the commands SHALL be processed by the simulation in the same order they were received by the server, within each simulation tick boundary.

**Validates: Requirements 16.2, 18.4**

### Property 9: Configuration application

*For any* valid configuration file containing a subset of configurable parameters (port, maxSessions, logVerbosity), the MCP server SHALL use the specified values for present keys and documented default values for absent keys.

**Validates: Requirements 2.4, 20.1, 20.2, 20.3**

### Property 10: Terrain grid resolution conformance

*For any* terrain query with a specified resolution R, the returned grid SHALL have dimensions matching the map size divided by R, and every cell in the grid SHALL contain terrain type, elevation, and passability data.

**Validates: Requirements 6.1, 6.2**

### Property 11: Session isolation

*For any* two concurrent sessions assigned to different players, commands issued by one session SHALL NOT affect the game state visible to the other session's player (beyond normal game interactions), and a malformed request from one session SHALL NOT disrupt the other session.

**Validates: Requirements 16.1, 16.2, 18.1**
