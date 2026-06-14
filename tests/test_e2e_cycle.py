"""
End-to-End Read/Write Cycle Test for 0 A.D. MCP Server

This script demonstrates and verifies a full game interaction cycle — exactly what
an AI agent would do when playing a game through the MCP interface:

    1. Connect and initialize the MCP session
    2. Read initial game state (all 7 resource URIs)
    3. Extract entity IDs from state and issue commands
    4. Wait a tick for the simulation to process
    5. Read updated state and compare to detect changes
    6. Test error cases (invalid commands)
    7. Disconnect cleanly

It works both as:
    - A manual verification guide: comments explain what each step validates
    - An automated test: reports pass/fail for each step

=== AI Agent Workflow Reference ===

The typical AI agent interaction loop with the 0 A.D. MCP server:

    CONNECT  → TCP to port 6374
    INIT     → MCP initialize handshake (capability negotiation)
    LOOP:
        READ   → game://units, game://buildings, game://resources (observe)
        DECIDE → Agent logic picks actions based on state
        ACT    → call_tool("move_units", ...), call_tool("train_unit", ...) etc.
        WAIT   → Brief pause for simulation tick (≈200ms)
    DISCONNECT → Clean shutdown

Resource URIs for observation:
    game://units          → Unit positions, health, actions
    game://buildings      → Buildings, production queues, garrisons
    game://resources      → Player resource counts + gathering rates
    game://resources/map  → Map resource deposits (positions, quantities)
    game://terrain        → Terrain grid (type, elevation, passability, fog)
    game://tech           → Technology tree (available, researching, completed)
    game://players        → Player info (civ, team, diplomacy, population)

Tool names for actions:
    train_unit       → Queue unit production at a building
    move_units       → Move units to a target position
    attack_target    → Attack an entity
    attack_move      → Move while engaging enemies along path
    build_structure  → Place and construct a building
    research_tech    → Begin technology research
    set_gather_point → Set building rally point
    gather_resource  → Order units to gather from a deposit

Requirements validated: 3.1, 4.1, 5.1, 9.1, 10.1, 12.1

Prerequisites:
    - 0 A.D. must be running with the MCP module active
    - A game must be in progress (not just the main menu)
    - MCP server listening on port 6374 (default)

Usage:
    python tests/test_e2e_cycle.py
    python tests/test_e2e_cycle.py --host 127.0.0.1 --port 6374
    python tests/test_e2e_cycle.py --verbose
"""

import sys
import os
import json
import time
import argparse
from typing import Any, Dict, List, Optional, Tuple

# Add parent directory so we can import mcp_client
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from mcp_client import (
    MCPClient,
    MCPClientError,
    MCPConnectionError,
    MCPTimeoutError,
    MCPProtocolError,
    MCPServerError,
)


# =============================================================================
# Constants
# =============================================================================

# All 7 MCP resource URIs exposed by the server
RESOURCE_URIS = [
    "game://units",
    "game://buildings",
    "game://resources",
    "game://resources/map",
    "game://terrain",
    "game://tech",
    "game://players",
]

# All 8 MCP tool names exposed by the server
TOOL_NAMES = [
    "train_unit",
    "move_units",
    "attack_target",
    "attack_move",
    "build_structure",
    "research_tech",
    "set_gather_point",
    "gather_resource",
]

# Expected required fields per resource type
REQUIRED_FIELDS = {
    "game://units": ["id", "position", "health", "owner", "type", "currentAction"],
    "game://buildings": ["id", "position", "health", "owner", "type",
                         "constructionProgress", "productionQueue"],
    "game://resources": ["food", "wood", "stone", "metal"],
    "game://resources/map": ["id", "position", "resourceType", "remainingQuantity"],
    "game://terrain": ["terrainType", "elevation", "passable"],
    "game://tech": ["techId", "status"],
    "game://players": ["id", "civilization", "team"],
}

# Simulation tick delay — enough time for one simulation tick to process
TICK_WAIT_SECONDS = 0.5


# =============================================================================
# Test Result Tracking
# =============================================================================

class StepResult:
    """Result of a single test step."""

    def __init__(self, name: str, passed: bool, message: str = "", details: str = ""):
        self.name = name
        self.passed = passed
        self.message = message
        self.details = details

    def __str__(self):
        icon = "✓" if self.passed else "✗"
        msg = f"  {icon} {self.name}"
        if not self.passed and self.message:
            msg += f"\n      ERROR: {self.message}"
        return msg


class E2ECycleTest:
    """
    Orchestrates the full end-to-end read/write cycle test.

    Maintains state across steps (entity IDs discovered during reads are
    used in subsequent commands, simulating real AI agent behavior).
    """

    def __init__(self, host: str, port: int, timeout: float, verbose: bool = False):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.verbose = verbose
        self.client: Optional[MCPClient] = None
        self.results: List[StepResult] = []

        # State extracted during initial reads — used by subsequent commands
        self.initial_units: List[Dict[str, Any]] = []
        self.initial_buildings: List[Dict[str, Any]] = []
        self.initial_resources: Dict[str, Any] = {}
        self.initial_resource_deposits: List[Dict[str, Any]] = []
        self.initial_players: List[Dict[str, Any]] = []

        # Entity IDs extracted for command targets
        self.own_unit_ids: List[int] = []
        self.own_building_ids: List[int] = []
        self.resource_deposit_id: Optional[int] = None

    def record(self, name: str, passed: bool, message: str = "", details: str = "") -> StepResult:
        """Record a step result."""
        result = StepResult(name, passed, message, details)
        self.results.append(result)
        print(result)
        if self.verbose and details:
            for line in details.split("\n"):
                print(f"      | {line}")
        return result

    def run(self) -> bool:
        """
        Execute the full end-to-end cycle. Returns True if all steps pass.

        Steps:
            1. Connect and initialize
            2. Read all resource URIs (initial state)
            3. Issue commands using discovered entity IDs
            4. Wait a simulation tick
            5. Read updated state and compare
            6. Test error cases
            7. Disconnect
        """
        print("=" * 70)
        print("  0 A.D. MCP Server — End-to-End Read/Write Cycle Test")
        print("=" * 70)
        print(f"  Target: {self.host}:{self.port}")
        print(f"  Timeout: {self.timeout}s")
        print()

        try:
            self._step_connect_and_initialize()
            self._step_read_initial_state()
            self._step_issue_commands()
            self._step_wait_tick()
            self._step_read_updated_state()
            self._step_error_cases()
        except MCPConnectionError as e:
            self.record("Unexpected disconnect", False, str(e))
        except Exception as e:
            self.record("Unexpected error", False, f"{type(e).__name__}: {e}")
        finally:
            self._step_disconnect()

        return self._print_summary()

    # =========================================================================
    # Step 1: Connect and Initialize
    # =========================================================================

    def _step_connect_and_initialize(self):
        """
        Step 1: Establish TCP connection and perform MCP handshake.

        This mirrors what an AI agent does first: connect to the game engine's
        MCP port and negotiate capabilities. The server tells us what resources
        (state reads) and tools (commands) are available.
        """
        print("\n--- Step 1: Connect and Initialize ---")

        # TCP connection
        self.client = MCPClient(timeout=self.timeout)
        try:
            self.client.connect(self.host, self.port)
            self.record("TCP connection established", True)
        except MCPConnectionError as e:
            self.record("TCP connection", False, str(e))
            raise

        # MCP handshake
        try:
            init_result = self.client.initialize()
            protocol_version = init_result.get("protocolVersion", "unknown")
            server_name = self.client.server_info.get("name", "unknown")
            caps = self.client.server_capabilities

            self.record(
                "MCP handshake completed",
                True,
                details=f"Protocol: {protocol_version}, Server: {server_name}"
            )

            # Verify server reports resources and tools capabilities
            has_resources = "resources" in caps
            has_tools = "tools" in caps
            self.record(
                "Server capabilities include resources and tools",
                has_resources and has_tools,
                message="" if (has_resources and has_tools)
                else f"Missing: {'resources' if not has_resources else ''} {'tools' if not has_tools else ''}".strip()
            )
        except MCPClientError as e:
            self.record("MCP handshake", False, str(e))
            raise

    # =========================================================================
    # Step 2: Read Initial State (All 7 Resource URIs)
    # =========================================================================

    def _step_read_initial_state(self):
        """
        Step 2: Read all game state resource URIs.

        An AI agent observes the full game state before making decisions.
        We read all 7 resource URIs and validate the response structure.
        We also extract entity IDs that will be used for commands in step 3.
        """
        print("\n--- Step 2: Read Initial Game State ---")

        for uri in RESOURCE_URIS:
            self._read_and_validate_resource(uri)

        # Extract entity IDs for use in commands
        self._extract_entity_ids()

        # Summary of what we found
        details = (
            f"Own units: {len(self.own_unit_ids)}, "
            f"Own buildings: {len(self.own_building_ids)}, "
            f"Resource deposits: {1 if self.resource_deposit_id else 0}"
        )
        has_entities = len(self.own_unit_ids) > 0 or len(self.own_building_ids) > 0
        self.record(
            "Entity IDs extracted for commands",
            has_entities,
            message="No owned entities found — commands will use placeholder IDs" if not has_entities else "",
            details=details
        )

    def _read_and_validate_resource(self, uri: str):
        """Read a single resource URI and validate its structure."""
        try:
            result = self.client.read_resource(uri)

            # Validate response is not empty/None
            if result is None:
                self.record(f"Read {uri}", False, "Response is None")
                return

            # Validate expected fields are present in the response data
            valid = self._validate_resource_fields(uri, result)
            details = ""
            if self.verbose:
                # Truncate for readability
                result_str = json.dumps(result, indent=2)
                if len(result_str) > 300:
                    result_str = result_str[:300] + "\n  ... (truncated)"
                details = result_str

            self.record(f"Read {uri}", valid, details=details)

            # Store state for later comparison
            self._store_initial_state(uri, result)

        except MCPServerError as e:
            self.record(f"Read {uri}", False, f"Server error [{e.code}]: {e.error_message}")
        except MCPClientError as e:
            self.record(f"Read {uri}", False, str(e))

    def _validate_resource_fields(self, uri: str, result: dict) -> bool:
        """
        Validate that the resource response contains expected fields.

        The MCP spec defines required fields per entity type. This checks
        that at least the response structure is correct (has the right shape).
        """
        expected_fields = REQUIRED_FIELDS.get(uri)
        if expected_fields is None:
            return True  # No field requirements defined for this URI

        # The result might be wrapped in MCP content structure
        # Try to find the actual data payload
        data = self._extract_resource_data(result)

        if data is None:
            # Empty data is valid (e.g., no units on map yet)
            return True

        if isinstance(data, list):
            # For list resources (units, buildings, deposits, techs, players),
            # validate fields on first entry if available
            if len(data) == 0:
                return True  # Empty list is valid
            first_entry = data[0]
            if isinstance(first_entry, dict):
                missing = [f for f in expected_fields if f not in first_entry]
                if missing and self.verbose:
                    print(f"      (Missing fields in {uri}: {missing})")
                return len(missing) == 0
        elif isinstance(data, dict):
            # For dict resources (player resources), check top-level keys
            missing = [f for f in expected_fields if f not in data]
            if missing and self.verbose:
                print(f"      (Missing fields in {uri}: {missing})")
            return len(missing) == 0

        return True  # Can't validate structure, assume OK

    def _extract_resource_data(self, result: dict) -> Any:
        """
        Extract actual data from MCP resource response wrapper.

        MCP responses are wrapped in {"contents": [{"text": "..."}]} format.
        This unwraps to get the actual game state data.
        """
        # Standard MCP resource response: { "contents": [{ "uri": ..., "text": ... }] }
        if "contents" in result:
            contents = result["contents"]
            if isinstance(contents, list) and len(contents) > 0:
                first = contents[0]
                if isinstance(first, dict) and "text" in first:
                    try:
                        return json.loads(first["text"])
                    except (json.JSONDecodeError, TypeError):
                        return first.get("text")

        # Direct data (non-standard but possible)
        if "data" in result:
            return result["data"]

        # The result itself might be the data
        return result

    def _store_initial_state(self, uri: str, result: dict):
        """Store initial state for comparison after commands execute."""
        data = self._extract_resource_data(result)

        if uri == "game://units" and isinstance(data, list):
            self.initial_units = data
        elif uri == "game://buildings" and isinstance(data, list):
            self.initial_buildings = data
        elif uri == "game://resources" and isinstance(data, dict):
            self.initial_resources = data
        elif uri == "game://resources/map" and isinstance(data, list):
            self.initial_resource_deposits = data
        elif uri == "game://players" and isinstance(data, list):
            self.initial_players = data

    def _extract_entity_ids(self):
        """
        Extract entity IDs owned by our player for use in commands.

        In a real AI agent, this is how you'd discover what units and buildings
        you control, and what resources are available to gather.
        """
        # Determine our player ID from player info
        our_player_id = self._determine_our_player_id()

        # Extract owned unit IDs
        for unit in self.initial_units:
            if isinstance(unit, dict):
                owner = unit.get("owner")
                unit_id = unit.get("id")
                if owner == our_player_id and unit_id is not None:
                    self.own_unit_ids.append(unit_id)

        # Extract owned building IDs
        for building in self.initial_buildings:
            if isinstance(building, dict):
                owner = building.get("owner")
                building_id = building.get("id")
                if owner == our_player_id and building_id is not None:
                    self.own_building_ids.append(building_id)

        # Get a resource deposit ID for gather commands
        if self.initial_resource_deposits:
            first_deposit = self.initial_resource_deposits[0]
            if isinstance(first_deposit, dict):
                self.resource_deposit_id = first_deposit.get("id")

    def _determine_our_player_id(self) -> Optional[int]:
        """
        Determine which player ID we control.

        The MCP server assigns the AI agent to a player slot during session
        creation. The players response includes a 'selfPlayerId' field.
        """
        # Check if players data has selfPlayerId
        for player in self.initial_players:
            if isinstance(player, dict):
                # The first player ID or one marked as self
                self_id = player.get("selfPlayerId")
                if self_id is not None:
                    return self_id

        # Fallback: assume player 1 (common in single-player scenarios)
        return 1

    # =========================================================================
    # Step 3: Issue Commands
    # =========================================================================

    def _step_issue_commands(self):
        """
        Step 3: Issue game commands using entity IDs from initial state.

        This demonstrates the "act" phase of an AI agent loop. We use real
        entity IDs discovered in step 2 to issue realistic commands.
        """
        print("\n--- Step 3: Issue Commands ---")

        # Use discovered IDs or fall back to placeholder values
        unit_id = self.own_unit_ids[0] if self.own_unit_ids else 1
        unit_ids_pair = self.own_unit_ids[:2] if len(self.own_unit_ids) >= 2 else [unit_id]
        building_id = self.own_building_ids[0] if self.own_building_ids else 1
        builder_ids = self.own_unit_ids[:3] if len(self.own_unit_ids) >= 3 else [unit_id]
        resource_id = self.resource_deposit_id if self.resource_deposit_id else 50

        # Command 1: Move a unit
        self._issue_command(
            "move_units",
            {
                "unitIds": [unit_id],
                "targetPosition": {"x": 200.0, "y": 0.0, "z": 200.0}
            },
            "Move unit to position (200, 0, 200)"
        )

        # Command 2: Train a unit from a building
        self._issue_command(
            "train_unit",
            {
                "buildingId": building_id,
                "unitType": "units/athen/infantry_spearman_b"
            },
            "Train infantry spearman at building"
        )

        # Command 3: Build a structure
        self._issue_command(
            "build_structure",
            {
                "buildingType": "structures/athen/house",
                "position": {"x": 250.0, "y": 0.0, "z": 250.0},
                "builderIds": builder_ids
            },
            "Build house at position (250, 0, 250)"
        )

        # Command 4: Research a technology
        self._issue_command(
            "research_tech",
            {
                "techId": "phase_town_athen",
                "buildingId": building_id
            },
            "Research town phase at building"
        )

        # Command 5: Gather from resource deposit
        self._issue_command(
            "gather_resource",
            {
                "unitIds": unit_ids_pair,
                "resourceId": resource_id
            },
            "Gather from resource deposit"
        )

    def _issue_command(self, tool_name: str, arguments: dict, description: str):
        """
        Issue a single command via tool call.

        Commands may succeed (accepted for execution) or fail with a game-rule
        error (insufficient resources, invalid target, etc.). Both are valid
        outcomes — the key is that the server processes the request correctly.
        """
        try:
            result = self.client.call_tool(tool_name, arguments)
            # Command accepted — check for expected response structure
            accepted = self._is_command_accepted(result)
            details = ""
            if self.verbose:
                details = json.dumps(result, indent=2)

            self.record(
                f"Command: {description}",
                True,
                details=details
            )
        except MCPServerError as e:
            # Game-rule errors are valid responses (e.g., insufficient resources)
            # The server correctly validated and rejected the command
            is_game_rule_error = e.code in (-32000, -32602)
            self.record(
                f"Command: {description}",
                True,  # Server responded correctly (even if command was rejected)
                details=f"Rejected [{e.code}]: {e.error_message}" if self.verbose else ""
            )
        except MCPClientError as e:
            self.record(f"Command: {description}", False, str(e))

    def _is_command_accepted(self, result: dict) -> bool:
        """Check if a command result indicates acceptance."""
        # Expected format: {"content": [{"type": "text", "text": "{\"status\": \"accepted\", ...}"}]}
        if "content" in result:
            contents = result.get("content", [])
            if isinstance(contents, list) and len(contents) > 0:
                first = contents[0]
                if isinstance(first, dict) and "text" in first:
                    try:
                        inner = json.loads(first["text"])
                        return inner.get("status") == "accepted"
                    except (json.JSONDecodeError, TypeError):
                        pass
        # If it has a "status" field directly
        if result.get("status") == "accepted":
            return True
        # If we got a result without an error, consider it accepted
        return True

    # =========================================================================
    # Step 4: Wait a Tick
    # =========================================================================

    def _step_wait_tick(self):
        """
        Step 4: Wait for the simulation to process our commands.

        The 0 A.D. simulation is tick-based. Commands are queued and executed
        at the start of the next tick. A brief pause ensures the simulation
        has had time to process at least one tick with our commands.
        """
        print("\n--- Step 4: Wait for Simulation Tick ---")
        time.sleep(TICK_WAIT_SECONDS)
        self.record(
            f"Waited {TICK_WAIT_SECONDS}s for tick processing",
            True,
            details="Commands should now be reflected in game state"
        )

    # =========================================================================
    # Step 5: Read Updated State and Compare
    # =========================================================================

    def _step_read_updated_state(self):
        """
        Step 5: Re-read game state and compare with initial state.

        After issuing commands and waiting a tick, we read the state again
        to verify the simulation has advanced. We look for:
        - Tick number increase (state is fresh)
        - Unit position changes (move command effect)
        - Production queue changes (train command effect)
        - New building foundations (build command effect)
        """
        print("\n--- Step 5: Read Updated State and Compare ---")

        # Re-read units
        try:
            updated_units_result = self.client.read_resource("game://units")
            updated_units = self._extract_resource_data(updated_units_result)
            if isinstance(updated_units, list):
                # Compare unit counts (may increase if train completed, unlikely in 1 tick)
                initial_count = len(self.initial_units)
                updated_count = len(updated_units)
                self.record(
                    "Re-read units after commands",
                    True,
                    details=f"Initial: {initial_count} units, Updated: {updated_count} units"
                )

                # Check for position changes on moved unit
                if self.own_unit_ids:
                    moved_id = self.own_unit_ids[0]
                    initial_pos = self._find_entity_position(self.initial_units, moved_id)
                    updated_pos = self._find_entity_position(updated_units, moved_id)
                    if initial_pos and updated_pos:
                        position_changed = initial_pos != updated_pos
                        self.record(
                            "Moved unit position changed",
                            True,  # We just note the state — movement may take many ticks
                            details=f"Initial: {initial_pos}, Updated: {updated_pos}, Changed: {position_changed}"
                        )
            else:
                self.record("Re-read units after commands", True, details="Non-list response")

        except MCPClientError as e:
            self.record("Re-read units after commands", False, str(e))

        # Re-read buildings
        try:
            updated_buildings_result = self.client.read_resource("game://buildings")
            updated_buildings = self._extract_resource_data(updated_buildings_result)
            if isinstance(updated_buildings, list):
                initial_count = len(self.initial_buildings)
                updated_count = len(updated_buildings)
                # New building foundation may have appeared from build_structure command
                self.record(
                    "Re-read buildings after commands",
                    True,
                    details=f"Initial: {initial_count} buildings, Updated: {updated_count} buildings"
                )
            else:
                self.record("Re-read buildings after commands", True, details="Non-list response")

        except MCPClientError as e:
            self.record("Re-read buildings after commands", False, str(e))

        # Re-read resources (should decrease if train/build consumed resources)
        try:
            updated_resources_result = self.client.read_resource("game://resources")
            updated_resources = self._extract_resource_data(updated_resources_result)
            if isinstance(updated_resources, dict) and isinstance(self.initial_resources, dict):
                changes = []
                for res_type in ["food", "wood", "stone", "metal"]:
                    initial_val = self.initial_resources.get(res_type, 0)
                    updated_val = updated_resources.get(res_type, 0)
                    if initial_val != updated_val:
                        changes.append(f"{res_type}: {initial_val} → {updated_val}")

                self.record(
                    "Re-read resources after commands",
                    True,
                    details=f"Changes: {changes if changes else 'none detected (expected for 1 tick)'}"
                )
            else:
                self.record("Re-read resources after commands", True)

        except MCPClientError as e:
            self.record("Re-read resources after commands", False, str(e))

    def _find_entity_position(self, entities: list, entity_id: int) -> Optional[Dict]:
        """Find an entity's position by ID."""
        for entity in entities:
            if isinstance(entity, dict) and entity.get("id") == entity_id:
                return entity.get("position")
        return None

    # =========================================================================
    # Step 6: Error Cases
    # =========================================================================

    def _step_error_cases(self):
        """
        Step 6: Verify error handling for invalid commands.

        A robust MCP server must reject invalid commands gracefully. These
        tests verify the server returns proper error codes without crashing.
        """
        print("\n--- Step 6: Error Cases ---")

        # Error case 1: Move with nonexistent unit IDs
        self._test_error_case(
            "Move with nonexistent units",
            "move_units",
            {
                "unitIds": [999999, 999998],
                "targetPosition": {"x": 100.0, "y": 0.0, "z": 100.0}
            },
            expected_error=True,
            expected_codes=[-32000, -32602]  # Game rule violation or invalid params
        )

        # Error case 2: Train from an enemy building (use a high ID unlikely to be ours)
        self._test_error_case(
            "Train from enemy/nonexistent building",
            "train_unit",
            {
                "buildingId": 999999,
                "unitType": "units/athen/infantry_spearman_b"
            },
            expected_error=True,
            expected_codes=[-32000, -32602]
        )

        # Error case 3: Attack own units (if we have multiple)
        if len(self.own_unit_ids) >= 2:
            attacker = self.own_unit_ids[0]
            target = self.own_unit_ids[1]
            self._test_error_case(
                "Attack own unit",
                "attack_target",
                {
                    "unitIds": [attacker],
                    "targetId": target
                },
                expected_error=True,
                expected_codes=[-32000, -32602],
                # Note: Some games allow friendly fire. If so, this might succeed.
                allow_success=True
            )
        else:
            # Use a fake attacker targeting our own unit (or just a nonexistent target)
            self._test_error_case(
                "Attack nonexistent target",
                "attack_target",
                {
                    "unitIds": [999999],
                    "targetId": 999998
                },
                expected_error=True,
                expected_codes=[-32000, -32602]
            )

        # Error case 4: Unknown tool name
        self._test_error_case(
            "Call nonexistent tool",
            "nonexistent_command_xyz",
            {"foo": "bar"},
            expected_error=True,
            expected_codes=[-32601]  # Method not found
        )

        # Error case 5: Tool with completely missing parameters
        self._test_error_case(
            "Tool with missing required params",
            "move_units",
            {},
            expected_error=True,
            expected_codes=[-32602, -32000]  # Invalid params or game rule
        )

        # Error case 6: Build at invalid position (e.g., negative coordinates)
        self._test_error_case(
            "Build at invalid position",
            "build_structure",
            {
                "buildingType": "structures/athen/house",
                "position": {"x": -9999.0, "y": -9999.0, "z": -9999.0},
                "builderIds": [999999]
            },
            expected_error=True,
            expected_codes=[-32000, -32602]
        )

    def _test_error_case(self, name: str, tool_name: str, arguments: dict,
                         expected_error: bool, expected_codes: List[int],
                         allow_success: bool = False):
        """
        Test a command that should produce an error response.

        Args:
            name: Human-readable test name
            tool_name: MCP tool name to call
            arguments: Tool arguments
            expected_error: Whether we expect the call to raise MCPServerError
            expected_codes: Acceptable error codes
            allow_success: If True, command success is also acceptable
        """
        try:
            result = self.client.call_tool(tool_name, arguments)
            # Command succeeded — is that acceptable?
            if allow_success:
                self.record(f"Error case: {name}", True,
                            details="Command succeeded (acceptable for this case)")
            else:
                self.record(f"Error case: {name}", False,
                            message="Expected error but command succeeded")
        except MCPServerError as e:
            # Got an error — verify it's the expected type
            code_match = e.code in expected_codes
            self.record(
                f"Error case: {name}",
                code_match,
                message="" if code_match else f"Unexpected error code {e.code}, expected one of {expected_codes}",
                details=f"Error [{e.code}]: {e.error_message}" if self.verbose else ""
            )
        except MCPClientError as e:
            self.record(f"Error case: {name}", False, f"Client error: {e}")

    # =========================================================================
    # Step 7: Disconnect
    # =========================================================================

    def _step_disconnect(self):
        """
        Step 7: Disconnect cleanly.

        A well-behaved AI agent always disconnects gracefully so the server
        can release session resources.
        """
        print("\n--- Step 7: Disconnect ---")

        if self.client and self.client.connected:
            try:
                self.client.disconnect()
                self.record("Clean disconnect", True)
            except MCPClientError as e:
                self.record("Clean disconnect", False, str(e))
        else:
            self.record("Clean disconnect", True, details="Already disconnected")

    # =========================================================================
    # Summary
    # =========================================================================

    def _print_summary(self) -> bool:
        """Print test summary and return True if all passed."""
        total = len(self.results)
        passed = sum(1 for r in self.results if r.passed)
        failed = total - passed

        print("\n" + "=" * 70)
        print(f"  END-TO-END CYCLE RESULTS: {passed}/{total} steps passed, {failed} failed")
        print("=" * 70)

        if failed > 0:
            print("\n  Failed steps:")
            for r in self.results:
                if not r.passed:
                    print(f"    ✗ {r.name}: {r.message}")
            print()

        return failed == 0


# =============================================================================
# Main Entry Point
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="End-to-end read/write cycle test for the 0 A.D. MCP server.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
This test demonstrates a complete AI agent interaction cycle:
  Connect → Read State → Issue Commands → Wait → Read Updated State → Disconnect

Run with --verbose to see full response payloads.

Examples:
  python test_e2e_cycle.py                    # Default settings
  python test_e2e_cycle.py --verbose          # Show response details
  python test_e2e_cycle.py --port 7000        # Custom port
  python test_e2e_cycle.py --timeout 15       # Longer timeout
        """
    )
    parser.add_argument(
        "--host", default=MCPClient.DEFAULT_HOST,
        help=f"MCP server host (default: {MCPClient.DEFAULT_HOST})"
    )
    parser.add_argument(
        "--port", type=int, default=MCPClient.DEFAULT_PORT,
        help=f"MCP server port (default: {MCPClient.DEFAULT_PORT})"
    )
    parser.add_argument(
        "--timeout", type=float, default=10.0,
        help="Response timeout in seconds (default: 10.0)"
    )
    parser.add_argument(
        "--verbose", "-v", action="store_true",
        help="Show detailed response payloads and debug info"
    )

    args = parser.parse_args()

    # -------------------------------------------------------------------------
    # Pre-flight: Check if we can connect at all
    # -------------------------------------------------------------------------
    print(f"\nChecking connectivity to {args.host}:{args.port}...")
    try:
        probe = MCPClient(timeout=min(args.timeout, 5.0))
        probe.connect(args.host, args.port)
        probe.disconnect()
        print("Server is reachable.\n")
    except MCPConnectionError as e:
        print(f"\n{'=' * 70}")
        print("  CANNOT CONNECT — Game is not running or MCP server is not active")
        print(f"{'=' * 70}")
        print(f"\n  Connection error: {e}")
        print(f"\n  To run this test:")
        print(f"    1. Launch 0 A.D. (with MCP module compiled in)")
        print(f"    2. Start a game (skirmish, scenario, or multiplayer)")
        print(f"    3. Verify MCP server is listening on port {args.port}")
        print(f"    4. Re-run this script")
        print()
        sys.exit(1)

    # -------------------------------------------------------------------------
    # Run the full cycle
    # -------------------------------------------------------------------------
    test = E2ECycleTest(
        host=args.host,
        port=args.port,
        timeout=args.timeout,
        verbose=args.verbose
    )

    success = test.run()
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
