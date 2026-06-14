/* Copyright (C) 2024 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_MCP_SIMULATION_HOOK
#define INCLUDED_MCP_SIMULATION_HOOK

#include "mcp/commands/MCPCommandQueue.h"
#include "mcp/state/MCPGameStateSnapshot.h"

#include "scriptinterface/ScriptForward.h"

// Forward declaration — avoid pulling in the full simulation headers
class CSimulation2;

namespace MCP
{

/**
 * Hook that bridges the MCP module with the game simulation tick loop.
 *
 * Registered into CSimulation2 when the MCP module is active.
 * - At tick start: drains the command queue and injects commands.
 * - At tick end: captures a snapshot of the game state.
 *
 * This ensures the network thread always reads a consistent, complete
 * snapshot from the most recently completed tick (Requirement 19.3),
 * and commands are processed at deterministic tick boundaries (Requirement 3.2).
 */
class MCPSimulationHook
{
public:
	/**
	 * Construct a simulation hook.
	 *
	 * @param snapshot Reference to the shared snapshot holder.
	 * @param commandQueue Reference to the shared command queue.
	 */
	MCPSimulationHook(MCPGameStateSnapshot& snapshot, MCPCommandQueue& commandQueue);
	~MCPSimulationHook() = default;

	// Non-copyable
	MCPSimulationHook(const MCPSimulationHook&) = delete;
	MCPSimulationHook& operator=(const MCPSimulationHook&) = delete;

	/**
	 * Called at the start of each simulation tick, before component updates.
	 * Drains the command queue and injects commands into the simulation.
	 *
	 * @param sim Reference to the simulation instance.
	 */
	void onTickStart(CSimulation2& sim);

	/**
	 * Called at the end of each simulation tick, after all component updates.
	 * Captures the current game state into an immutable snapshot.
	 *
	 * @param sim Reference to the simulation instance.
	 */
	void onTickEnd(CSimulation2& sim);

private:
	/**
	 * Capture the full game state from the simulation and publish
	 * it as a new snapshot. Delegates to specialized capture methods
	 * for each data category.
	 *
	 * @param sim Reference to the simulation instance.
	 */
	void captureSnapshot(CSimulation2& sim);

	/**
	 * Capture player resource counts and gathering rates.
	 * Queries ICmpPlayer script data for each player.
	 *
	 * @param sim Reference to the simulation instance.
	 * @param state The state being built (output).
	 */
	void capturePlayerResources(CSimulation2& sim, GameStateData& state);

	/**
	 * Capture resource deposit entities (trees, mines, etc.).
	 * Iterates entities with ICmpResourceSupply component.
	 *
	 * @param sim Reference to the simulation instance.
	 * @param state The state being built (output).
	 */
	void captureResourceDeposits(CSimulation2& sim, GameStateData& state);

	/**
	 * Capture terrain grid data (elevation, type, passability).
	 * Reads from ICmpTerrain and ICmpPathfinder at a fixed resolution.
	 *
	 * @param sim Reference to the simulation instance.
	 * @param state The state being built (output).
	 */
	void captureTerrain(CSimulation2& sim, GameStateData& state);

	/**
	 * Capture technology research state per player.
	 * Queries ICmpTechnologyManager (JS) for completed and in-progress techs.
	 *
	 * @param sim Reference to the simulation instance.
	 * @param state The state being built (output).
	 */
	void captureTechTrees(CSimulation2& sim, GameStateData& state);

	/**
	 * Capture player info (civilization, team, population, diplomacy).
	 * Queries Player component script data.
	 *
	 * @param sim Reference to the simulation instance.
	 * @param state The state being built (output).
	 */
	void capturePlayerInfo(CSimulation2& sim, GameStateData& state);

	/**
	 * Capture all units and buildings by iterating positioned entities.
	 * Classifies entities as units (have UnitAI) or buildings (have
	 * ProductionQueue/GarrisonHolder) and reads their properties.
	 *
	 * @param sim Reference to the simulation instance.
	 * @param state The state being built (output).
	 */
	void captureUnitsAndBuildings(CSimulation2& sim, GameStateData& state);

	/**
	 * Capture entity visibility data from ICmpRangeManager for all players.
	 * For each entity in the snapshot (units, buildings, resource deposits),
	 * queries LOS visibility per player and stores it in the snapshot.
	 * Also captures per-player terrain visibility grid.
	 *
	 * @param sim Reference to the simulation instance.
	 * @param state The state being built (output).
	 */
	void captureVisibility(CSimulation2& sim, GameStateData& state);

	/**
	 * Inject validated commands into the simulation.
	 * Uses the same path as the built-in AI (ICmpCommandQueue::PushLocalCommand)
	 * to ensure commands go through proper game rule validation.
	 *
	 * @param sim Reference to the simulation instance.
	 * @param commands The commands drained from the queue this tick.
	 */
	void injectCommands(CSimulation2& sim, const std::vector<MCPCommand>& commands);

	/**
	 * Convert an MCPCommand into a JS value suitable for PushLocalCommand.
	 * Parses the JSON parameters string and sets the "type" property to the
	 * appropriate 0 A.D. command type string.
	 *
	 * @param rq The active ScriptRequest context.
	 * @param cmd The MCP command to convert.
	 * @param out Output JS value representing the command.
	 * @return true if conversion succeeded, false on error.
	 */
	bool convertToJSCommand(const ScriptRequest& rq, const MCPCommand& cmd,
		JS::MutableHandleValue out);

	MCPGameStateSnapshot& m_snapshot;
	MCPCommandQueue& m_commandQueue;
	uint32_t m_tickCounter = 0;
};

} // namespace MCP

#endif // INCLUDED_MCP_SIMULATION_HOOK
