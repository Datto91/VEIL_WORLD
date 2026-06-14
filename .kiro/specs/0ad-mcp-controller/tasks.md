# Implementation Plan: 0 A.D. MCP Controller

## Overview

This plan implements an in-process MCP server within the 0 A.D. game engine on Windows, following a 6-phase approach: validate the vanilla build, scaffold the MCP module, implement network transport, add game state reading, wire up command execution, and verify with integration tests. All code is C++ targeting Visual Studio, placed in `source/mcp/`, using nlohmann/json (header-only) and Winsock2.

## Tasks

- [x] 1. Vanilla Build Validation
  - [x] 1.1 Clone 0 A.D. source and set up Visual Studio build environment
    - Clone the 0 A.D. source repository (SVN or Git mirror)
    - Install required dependencies (SpiderMonkey, Boost, SDL, etc.)
    - Run `update-workspaces.bat` to generate Visual Studio solution via premake
    - _Requirements: 1.1, 1.2_

  - [x] 1.2 Build unmodified source and verify runnable executable
    - Open generated `.sln` in Visual Studio and build Release configuration
    - Verify pyrogenesis.exe is produced without errors
    - Launch the game and confirm it reaches the main menu
    - If build fails, document missing dependencies with expected locations
    - _Requirements: 1.1, 1.2, 1.3_

- [x] 2. MCP Module Scaffold
  - [x] 2.1 Create `source/mcp/` directory structure and core header files
    - Create directories: `source/mcp/`, `source/mcp/network/`, `source/mcp/state/`, `source/mcp/commands/`, `source/mcp/protocol/`
    - Create `MCPModule.h` with singleton class declaration (initialize, shutdown, isActive)
    - Create `MCPConfig.h` with config struct and `loadFromFile`/`defaults` static methods
    - Create `MCPTypes.h` with shared type definitions (SessionId, PlayerId, EntityId, Vec3, enums)
    - _Requirements: 2.2_

  - [x] 2.2 Update premake5 configuration to include MCP source group
    - Add MCP source files as a new source group in the pyrogenesis premake configuration
    - Add include path for nlohmann/json header-only library
    - Link Winsock2 (`ws2_32.lib`) for Windows networking
    - Verify solution regenerates correctly with `update-workspaces.bat`
    - _Requirements: 2.2_

  - [x] 2.3 Implement MCPModule lifecycle (init/shutdown) and engine hook
    - Implement `MCPModule::getInstance()` singleton pattern
    - Implement `MCPModule::initialize()` — reads config, initializes subsystems, sets `m_active`
    - Implement `MCPModule::shutdown()` — graceful teardown of all subsystems
    - Hook MCPModule init into engine startup (e.g., in `main.cpp` or startup sequence)
    - Hook MCPModule shutdown into engine exit path
    - If port unavailable, log error and continue without MCP active
    - _Requirements: 2.1, 2.3_

  - [x] 2.4 Implement MCPConfig loader with file parsing and defaults
    - Implement `MCPConfig::loadFromFile()` reading `mcp_config.json` from `%APPDATA%\0ad\config\`
    - Implement `MCPConfig::defaults()` returning port=6374, maxSessions=4, logVerbosity=1
    - Use nlohmann/json for JSON parsing
    - Handle missing file gracefully (fall back to defaults)
    - Handle partial config (use defaults for missing keys)
    - _Requirements: 2.4, 20.1, 20.2, 20.3_

  - [ ]* 2.5 Write property test for configuration application
    - **Property 9: Configuration application**
    - For any valid config file with a subset of keys, verify specified values are used and absent keys get defaults
    - **Validates: Requirements 2.4, 20.1, 20.2, 20.3**

  - [x] 2.6 Verify engine builds and starts with empty MCP module
    - Build the full solution with MCP module included
    - Verify game launches normally with MCP module initializing
    - Verify log output shows MCP initialization message
    - _Requirements: 2.1, 2.2_

- [x] 3. Checkpoint - Verify scaffold builds cleanly
  - Ensure all tests pass, ask the user if questions arise.

- [x] 4. Network Transport
  - [x] 4.1 Implement TCP listener on dedicated network thread (Winsock2)
    - Implement `MCPNetworkTransport` class with `start(port)` and `stop()` methods
    - Initialize Winsock2 (`WSAStartup`), create non-blocking TCP socket, bind, listen
    - Spawn `m_networkThread` that runs accept loop
    - Handle multiple connections (select/poll based)
    - Graceful shutdown with socket close and thread join
    - _Requirements: 2.1, 19.2_

  - [x] 4.2 Implement JSON-RPC 2.0 message framing and parsing
    - Implement message delimiter protocol (Content-Length header + body, per MCP spec)
    - Parse incoming bytes into complete JSON-RPC messages using nlohmann/json
    - Handle partial reads, buffer management, and message boundaries
    - Return parse error (-32700) for malformed JSON
    - Return invalid request error (-32600) for non-JSON-RPC messages
    - _Requirements: 17.1, 18.1_

  - [x] 4.3 Implement MCP capability negotiation handshake
    - Implement `initialize` method handler returning server capabilities
    - Expose resources (game state) and tools (commands) in capabilities response
    - Handle `initialized` notification from client
    - Build capabilities response listing all supported resource URIs and tool names
    - _Requirements: 17.4, 16.1_

  - [x] 4.4 Implement MCPSessionManager with session creation/destruction
    - Implement `createSession()` assigning a player slot and returning SessionId
    - Implement `destroySession()` cleaning up resources
    - Implement `getSession()` for session lookup
    - Enforce max concurrent sessions limit from config
    - Handle unexpected disconnection (detect closed socket, cleanup, log)
    - Implement graceful disconnect on client request
    - Thread-safe session map with mutex
    - _Requirements: 16.1, 16.2, 16.3, 16.4_

  - [ ]* 4.5 Write property test for malformed input resilience
    - **Property 6: Malformed input resilience**
    - Send random byte sequences, truncated JSON, invalid JSON-RPC, oversized payloads
    - Verify server returns protocol-compliant error and does not crash
    - **Validates: Requirements 18.1, 18.3**

  - [ ]* 4.6 Write property test for MCP protocol message round-trip
    - **Property 7: MCP protocol message round-trip**
    - Verify serialize→transmit→deserialize produces structurally identical objects
    - **Validates: Requirements 17.1**

- [x] 5. Checkpoint - Verify network transport accepts connections
  - Ensure all tests pass, ask the user if questions arise.

- [x] 6. Game State Reading
  - [x] 6.1 Implement MCPSimulationHook and tick-boundary snapshot capture
    - Implement `MCPSimulationHook` class with `onTickStart()` and `onTickEnd()` methods
    - Implement `MCPGameStateSnapshot` with `shared_mutex` for concurrent access
    - In `onTickEnd()`: iterate simulation entities, build `GameStateData`, call `updateSnapshot()`
    - Hook into `CSimulation2::Update()` — call `onTickStart` before tick, `onTickEnd` after tick
    - Register hook in `CSimulation2` initialization when MCPModule is active
    - _Requirements: 3.2, 19.1, 19.3_

  - [x] 6.2 Implement MCPStateReader for units and buildings
    - Implement `queryUnits(PlayerId)` reading from snapshot: position, health, owner, type, currentAction
    - Implement `queryBuildings(PlayerId)`: position, health, owner, type, constructionProgress, productionQueue, garrison
    - Access components: ICmpPosition, ICmpHealth, ICmpOwnership, ICmpUnitAI, ICmpProductionQueue, ICmpGarrisonHolder
    - Format results as JSON matching MCP resource response schema
    - _Requirements: 3.1, 4.1, 4.2_

  - [x] 6.3 Implement MCPStateReader for resources, terrain, tech tree, and player info
    - Implement `queryResources(PlayerId)`: food, wood, stone, metal + gathering rates
    - Implement `queryResourceLocations(PlayerId)`: resource deposit positions and quantities
    - Implement `queryTerrain(PlayerId, resolution)`: grid of terrain type, elevation, passability
    - Implement `queryTechTree(PlayerId)`: available techs, prerequisites, costs, status, progress
    - Implement `queryPlayerInfo()`: civilization, team, diplomacy, population for all players
    - _Requirements: 5.1, 5.2, 5.3, 6.1, 6.2, 7.1, 7.2, 8.1, 8.2_

  - [x] 6.4 Implement fog-of-war filtering in state reader
    - Implement `isEntityVisible(EntityId, PlayerId)` using ICmpRangeManager visibility queries
    - Filter unit/building queries to only return entities visible to requesting player
    - Filter resource deposits by visibility
    - Mark terrain cells with FogOfWarState (VISIBLE, EXPLORED, UNEXPLORED)
    - _Requirements: 3.1, 5.3, 6.3_

  - [x] 6.5 Wire up MCP resource handlers in MCPProtocolHandler
    - Implement `handleResourceRead()` routing resource URIs to appropriate state reader methods
    - Map `game://units`, `game://buildings`, `game://resources`, `game://resources/map`, `game://terrain`, `game://tech`, `game://players`
    - Return method-not-found error (-32601) for unknown resource URIs
    - _Requirements: 17.2_

  - [ ]* 6.6 Write property test for game state response completeness
    - **Property 1: Game state response completeness**
    - For any entity type query, verify all required fields are present for every visible entity
    - **Validates: Requirements 3.1, 4.1, 4.2, 5.1, 5.2, 5.3, 6.1, 7.1, 7.2, 8.1**

  - [ ]* 6.7 Write property test for visibility-filtered state access
    - **Property 2: Visibility-filtered state access**
    - Verify responses contain only entities visible/explored by the requesting player
    - **Validates: Requirements 3.1, 5.3, 6.3**

  - [ ]* 6.8 Write property test for snapshot tick consistency
    - **Property 3: Snapshot tick consistency**
    - Verify reads during tick N+1 processing return exactly end-of-tick-N state
    - **Validates: Requirements 3.2, 19.3**

  - [ ]* 6.9 Write property test for terrain grid resolution conformance
    - **Property 10: Terrain grid resolution conformance**
    - For any resolution R, verify grid dimensions equal map_size/R and all cells have required fields
    - **Validates: Requirements 6.1, 6.2**

- [x] 7. Checkpoint - Verify game state reads return correct data
  - Ensure all tests pass, ask the user if questions arise.

- [x] 8. Command Execution
  - [x] 8.1 Implement MCPCommandExecutor validation logic
    - Implement `validate()` method checking commands against current game state snapshot
    - Validate resource sufficiency for train/build/research commands
    - Validate target existence and attackability for attack commands
    - Validate building capabilities (can produce unit type, not occupied for research)
    - Validate placement validity for build commands
    - Validate prerequisites for research commands
    - Validate gatherer capability for gather commands
    - Return appropriate `CommandValidationResult` enum values
    - _Requirements: 9.2, 9.3, 11.3, 12.2, 12.3, 13.2, 13.3, 15.2, 18.3_

  - [x] 8.2 Implement MCPCommandQueue and tick-boundary injection
    - Implement `MCPCommandQueue` using `moodycamel::ConcurrentQueue` (or equivalent lock-free queue)
    - Implement `enqueue()` for validated commands from network thread
    - Implement `drainAll()` called by simulation thread at tick start
    - In `MCPSimulationHook::onTickStart()`: drain queue and inject commands
    - Handle command rate exceeding tick rate via queuing across ticks
    - _Requirements: 18.4, 19.1_

  - [x] 8.3 Implement SpiderMonkey bridge for command posting
    - Implement `convertToJSCommand()` translating MCPCommand to JS value via ScriptInterface
    - Use `CSimulation2::PostCommand(playerId, jsValue)` — same path as existing AI system
    - Map each MCPCommandType to the corresponding JS command format expected by simulation
    - Handle all command types: train, move, attack, attack-move, build, research, gather-point, gather
    - _Requirements: 9.1, 10.1, 10.2, 10.3, 11.1, 11.2, 12.1, 13.1, 14.1, 14.2, 15.1_

  - [x] 8.4 Wire up MCP tool handlers in MCPProtocolHandler
    - Implement `handleToolCall()` routing tool names to command executor methods
    - Map `train_unit`, `move_units`, `attack_target`, `attack_move`, `build_structure`, `research_tech`, `set_gather_point`, `gather_resource`
    - Return validation errors as JSON-RPC error responses with appropriate codes
    - Return success responses with `{"status": "accepted", "tickQueued": N}` format
    - Return method-not-found error (-32601) for unknown tool names
    - _Requirements: 17.3, 18.2_

  - [ ]* 8.5 Write property test for valid command acceptance
    - **Property 4: Valid command acceptance**
    - For any command passing game rule validation, verify it is accepted and game state reflects the effect after next tick
    - **Validates: Requirements 9.1, 10.1, 10.2, 10.3, 11.1, 11.2, 12.1, 13.1, 14.1, 14.2, 15.1**

  - [ ]* 8.6 Write property test for invalid command rejection with stable state
    - **Property 5: Invalid command rejection with stable state**
    - For any command violating game rules, verify error response is returned and game state is unchanged
    - **Validates: Requirements 9.2, 9.3, 11.3, 12.2, 12.3, 13.2, 13.3, 15.2**

  - [ ]* 8.7 Write property test for command ordering preservation
    - **Property 8: Command ordering preservation**
    - For any sequence of commands in a single session, verify processing order matches receive order within tick boundaries
    - **Validates: Requirements 16.2, 18.4**

- [x] 9. Checkpoint - Verify commands execute correctly in simulation
  - Ensure all tests pass, ask the user if questions arise.

- [x] 10. Integration Testing and Hardening
  - [x] 10.1 Create integration test harness with external MCP client
    - Write a standalone C++ or Python MCP client test harness
    - Connect to the MCP server on configured port
    - Perform capability negotiation handshake
    - Establish a session and assign to a player slot
    - Verify connection lifecycle (connect, interact, disconnect)
    - _Requirements: 16.1, 16.4, 17.4_

  - [x] 10.2 Verify full read/write cycle end-to-end
    - Read initial game state (units, buildings, resources)
    - Issue commands (train unit, move unit, build structure)
    - Read updated game state and verify command effects reflected
    - Test all resource URIs and all tool names
    - Verify error responses for invalid commands
    - _Requirements: 3.1, 4.1, 5.1, 9.1, 10.1, 12.1_

  - [ ]* 10.3 Stress test with rapid command streams
    - Send commands at rate exceeding simulation tick rate
    - Verify all commands are queued and eventually processed
    - Verify no commands are dropped or duplicated
    - Verify server remains responsive under load
    - _Requirements: 18.4_

  - [ ]* 10.4 Write property test for session isolation
    - **Property 11: Session isolation**
    - Connect two sessions as different players
    - Verify commands from one don't affect the other's visible state (beyond game interactions)
    - Verify malformed request from one session doesn't disrupt the other
    - **Validates: Requirements 16.1, 16.2, 18.1**

  - [ ]* 10.5 Verify thread safety under concurrent load
    - Run multiple concurrent readers and writers
    - Verify no data races (use ThreadSanitizer if available, or stress patterns)
    - Verify snapshot reads are consistent (no partial tick data)
    - Verify command queue operates correctly under contention
    - _Requirements: 19.1, 19.2, 19.3_

- [x] 11. Final Checkpoint - Full integration verified
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation at each phase boundary
- Property tests validate universal correctness properties from the design document
- All code is C++ compiled with Visual Studio on Windows
- nlohmann/json is header-only (no build complexity), Winsock2 is a system library
- Commands use the same `PostCommand` path as the existing AI system for proper game rule enforcement
- The shared_mutex snapshot pattern ensures network thread never blocks the simulation thread for reads

## Task Dependency Graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["1.1"] },
    { "id": 1, "tasks": ["1.2"] },
    { "id": 2, "tasks": ["2.1", "2.2"] },
    { "id": 3, "tasks": ["2.3", "2.4"] },
    { "id": 4, "tasks": ["2.5", "2.6"] },
    { "id": 5, "tasks": ["4.1"] },
    { "id": 6, "tasks": ["4.2", "4.4"] },
    { "id": 7, "tasks": ["4.3", "4.5", "4.6"] },
    { "id": 8, "tasks": ["6.1"] },
    { "id": 9, "tasks": ["6.2", "6.3"] },
    { "id": 10, "tasks": ["6.4"] },
    { "id": 11, "tasks": ["6.5", "6.6", "6.7", "6.8", "6.9"] },
    { "id": 12, "tasks": ["8.1", "8.2"] },
    { "id": 13, "tasks": ["8.3"] },
    { "id": 14, "tasks": ["8.4", "8.5", "8.6", "8.7"] },
    { "id": 15, "tasks": ["10.1"] },
    { "id": 16, "tasks": ["10.2"] },
    { "id": 17, "tasks": ["10.3", "10.4", "10.5"] }
  ]
}
```
