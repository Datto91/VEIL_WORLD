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

#ifndef INCLUDED_MCP_COMMAND_EXECUTOR
#define INCLUDED_MCP_COMMAND_EXECUTOR

#include "mcp/MCPTypes.h"
#include "mcp/commands/MCPCommandQueue.h"
#include "mcp/state/MCPGameStateData.h"
#include "mcp/state/MCPGameStateSnapshot.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace MCP
{

using json = nlohmann::json;

/**
 * Validates and executes commands against the current game state.
 *
 * The executor performs "best effort" validation using the most recently
 * completed tick's snapshot. This catches obvious errors early and gives
 * immediate feedback to the AI agent. The actual game simulation also
 * validates commands when they are injected on the next tick.
 */
class MCPCommandExecutor
{
public:
	MCPCommandExecutor(MCPGameStateSnapshot& snapshot, MCPCommandQueue& commandQueue);
	~MCPCommandExecutor() = default;

	// Non-copyable
	MCPCommandExecutor(const MCPCommandExecutor&) = delete;
	MCPCommandExecutor& operator=(const MCPCommandExecutor&) = delete;

	/**
	 * Validate a command against the current game state snapshot.
	 *
	 * @param cmd The command to validate.
	 * @param state The game state to validate against.
	 * @return CommandValidationResult indicating whether the command is valid
	 *         or which specific validation check failed.
	 */
	CommandValidationResult validate(const MCPCommand& cmd, const GameStateData& state);

	/**
	 * Enqueue a validated command for execution on the next simulation tick.
	 *
	 * @param cmd The validated command to enqueue.
	 */
	void execute(MCPCommand cmd);

	/**
	 * Handle a train_unit tool call.
	 * Validates and enqueues unit production at a specified building.
	 */
	json handleTrain(PlayerId player, EntityId building, const std::string& unitType);

	/**
	 * Handle a move_units tool call.
	 * Validates and enqueues unit movement to a target position.
	 */
	json handleMove(PlayerId player, std::vector<EntityId> units, Vec3 target,
		std::optional<std::string> formation);

	/**
	 * Handle an attack_target tool call.
	 * Validates and enqueues units to attack a specific entity.
	 */
	json handleAttack(PlayerId player, std::vector<EntityId> units, EntityId target);

	/**
	 * Handle an attack_move tool call.
	 * Validates and enqueues units to attack-move to a position.
	 */
	json handleAttackMove(PlayerId player, std::vector<EntityId> units, Vec3 target);

	/**
	 * Handle a build_structure tool call.
	 * Validates and enqueues building placement and construction.
	 */
	json handleBuild(PlayerId player, const std::string& buildingType, Vec3 position,
		std::vector<EntityId> builders);

	/**
	 * Handle a research_tech tool call.
	 * Validates and enqueues technology research at a building.
	 */
	json handleResearch(PlayerId player, const std::string& techId, EntityId building);

	/**
	 * Handle a set_gather_point tool call.
	 * Validates and enqueues setting a building's gather/rally point.
	 */
	json handleSetGatherPoint(PlayerId player, EntityId building,
		std::variant<Vec3, EntityId> target);

	/**
	 * Handle a gather_resource tool call.
	 * Validates and enqueues units to gather from a resource deposit.
	 */
	json handleGather(PlayerId player, std::vector<EntityId> units, EntityId resource);

private:
	/**
	 * Build a success JSON response for a queued command.
	 */
	json buildSuccessResponse(uint32_t tickNumber) const;

	/**
	 * Build an error JSON response for a failed validation.
	 */
	json buildErrorResponse(CommandValidationResult result) const;

	/**
	 * Convert CommandValidationResult to a human-readable error string.
	 */
	static std::string validationResultToString(CommandValidationResult result);

	/**
	 * Validate that all specified units exist and are owned by the player.
	 */
	CommandValidationResult validateUnitOwnership(
		const std::vector<EntityId>& unitIds, PlayerId player,
		const GameStateData& state) const;

	/**
	 * Validate that a building exists and is owned by the player.
	 */
	CommandValidationResult validateBuildingOwnership(
		EntityId buildingId, PlayerId player, const GameStateData& state) const;

	/**
	 * Find a building by ID in the game state.
	 * Returns nullptr if not found.
	 */
	const BuildingState* findBuilding(EntityId id, const GameStateData& state) const;

	/**
	 * Find a unit by ID in the game state.
	 * Returns nullptr if not found.
	 */
	const UnitState* findUnit(EntityId id, const GameStateData& state) const;

	/**
	 * Find a resource deposit by ID in the game state.
	 * Returns nullptr if not found.
	 */
	const ResourceDeposit* findResource(EntityId id, const GameStateData& state) const;

	MCPGameStateSnapshot& m_snapshot;
	MCPCommandQueue& m_commandQueue;
};

} // namespace MCP

#endif // INCLUDED_MCP_COMMAND_EXECUTOR
