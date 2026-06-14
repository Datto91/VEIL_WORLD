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

#include "lib/precompiled.h"

#include "mcp/commands/MCPCommandExecutor.h"

#include <algorithm>
#include <chrono>

namespace MCP
{

MCPCommandExecutor::MCPCommandExecutor(MCPGameStateSnapshot& snapshot, MCPCommandQueue& commandQueue)
	: m_snapshot(snapshot), m_commandQueue(commandQueue)
{
}

// ─────────────────────────────────────────────────────────────────────────────
// Lookup helpers
// ─────────────────────────────────────────────────────────────────────────────

const BuildingState* MCPCommandExecutor::findBuilding(EntityId id, const GameStateData& state) const
{
	for (const auto& building : state.buildings)
	{
		if (building.id == id)
			return &building;
	}
	return nullptr;
}

const UnitState* MCPCommandExecutor::findUnit(EntityId id, const GameStateData& state) const
{
	for (const auto& unit : state.units)
	{
		if (unit.id == id)
			return &unit;
	}
	return nullptr;
}

const ResourceDeposit* MCPCommandExecutor::findResource(EntityId id, const GameStateData& state) const
{
	for (const auto& deposit : state.resourceDeposits)
	{
		if (deposit.id == id)
			return &deposit;
	}
	return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Ownership validation helpers
// ─────────────────────────────────────────────────────────────────────────────

CommandValidationResult MCPCommandExecutor::validateUnitOwnership(
	const std::vector<EntityId>& unitIds, PlayerId player, const GameStateData& state) const
{
	for (EntityId unitId : unitIds)
	{
		const UnitState* unit = findUnit(unitId, state);
		if (!unit)
			return CommandValidationResult::InvalidTarget;
		if (unit->owner != player)
			return CommandValidationResult::InvalidTarget;
	}
	return CommandValidationResult::Valid;
}

CommandValidationResult MCPCommandExecutor::validateBuildingOwnership(
	EntityId buildingId, PlayerId player, const GameStateData& state) const
{
	const BuildingState* building = findBuilding(buildingId, state);
	if (!building)
		return CommandValidationResult::InvalidTarget;
	if (building->owner != player)
		return CommandValidationResult::InvalidTarget;
	return CommandValidationResult::Valid;
}

// ─────────────────────────────────────────────────────────────────────────────
// Validation
// ─────────────────────────────────────────────────────────────────────────────

CommandValidationResult MCPCommandExecutor::validate(const MCPCommand& cmd, const GameStateData& state)
{
	json params;
	try
	{
		params = json::parse(cmd.parameters);
	}
	catch (const json::parse_error&)
	{
		return CommandValidationResult::UnknownCommand;
	}

	switch (cmd.type)
	{
	case MCPCommandType::TrainUnit:
	{
		// Validate building exists and is owned by issuing player
		if (!params.contains("buildingId") || !params.contains("unitType"))
			return CommandValidationResult::UnknownCommand;

		EntityId buildingId = params["buildingId"].get<EntityId>();
		CommandValidationResult ownershipResult =
			validateBuildingOwnership(buildingId, cmd.issuingPlayer, state);
		if (ownershipResult != CommandValidationResult::Valid)
			return ownershipResult;

		// Check building has production capability (constructionProgress == 1.0
		// indicates building is complete and can produce)
		const BuildingState* building = findBuilding(buildingId, state);
		if (building->constructionProgress < 1.0f)
			return CommandValidationResult::InvalidProduction;

		return CommandValidationResult::Valid;
	}

	case MCPCommandType::MoveUnits:
	{
		// Validate all units exist and are owned by player
		if (!params.contains("unitIds"))
			return CommandValidationResult::UnknownCommand;

		std::vector<EntityId> unitIds = params["unitIds"].get<std::vector<EntityId>>();
		if (unitIds.empty())
			return CommandValidationResult::InvalidTarget;

		return validateUnitOwnership(unitIds, cmd.issuingPlayer, state);
	}

	case MCPCommandType::AttackTarget:
	{
		// Validate attacking units exist and are owned by player
		if (!params.contains("unitIds") || !params.contains("targetId"))
			return CommandValidationResult::UnknownCommand;

		std::vector<EntityId> unitIds = params["unitIds"].get<std::vector<EntityId>>();
		if (unitIds.empty())
			return CommandValidationResult::InvalidTarget;

		CommandValidationResult unitResult =
			validateUnitOwnership(unitIds, cmd.issuingPlayer, state);
		if (unitResult != CommandValidationResult::Valid)
			return unitResult;

		// Validate target exists and is NOT owned by player (can attack)
		EntityId targetId = params["targetId"].get<EntityId>();

		// Check if target is a unit
		const UnitState* targetUnit = findUnit(targetId, state);
		if (targetUnit)
		{
			if (targetUnit->owner == cmd.issuingPlayer)
				return CommandValidationResult::InvalidTarget;
			return CommandValidationResult::Valid;
		}

		// Check if target is a building
		const BuildingState* targetBuilding = findBuilding(targetId, state);
		if (targetBuilding)
		{
			if (targetBuilding->owner == cmd.issuingPlayer)
				return CommandValidationResult::InvalidTarget;
			return CommandValidationResult::Valid;
		}

		// Target entity doesn't exist
		return CommandValidationResult::InvalidTarget;
	}

	case MCPCommandType::AttackMove:
	{
		// Validate units exist and are owned by player
		if (!params.contains("unitIds"))
			return CommandValidationResult::UnknownCommand;

		std::vector<EntityId> unitIds = params["unitIds"].get<std::vector<EntityId>>();
		if (unitIds.empty())
			return CommandValidationResult::InvalidTarget;

		return validateUnitOwnership(unitIds, cmd.issuingPlayer, state);
	}

	case MCPCommandType::BuildStructure:
	{
		// Validate builders exist and are owned by player
		if (!params.contains("builderIds") || !params.contains("buildingType") ||
			!params.contains("position"))
			return CommandValidationResult::UnknownCommand;

		std::vector<EntityId> builderIds = params["builderIds"].get<std::vector<EntityId>>();
		if (builderIds.empty())
			return CommandValidationResult::InvalidTarget;

		CommandValidationResult builderResult =
			validateUnitOwnership(builderIds, cmd.issuingPlayer, state);
		if (builderResult != CommandValidationResult::Valid)
			return builderResult;

		// Placement validation is complex (terrain, collision, etc.) —
		// mark as Valid if builders are valid. The simulation will perform
		// full placement validation when the command is injected.
		return CommandValidationResult::Valid;
	}

	case MCPCommandType::ResearchTech:
	{
		// Validate building exists and is owned by player
		if (!params.contains("buildingId") || !params.contains("techId"))
			return CommandValidationResult::UnknownCommand;

		EntityId buildingId = params["buildingId"].get<EntityId>();
		CommandValidationResult ownershipResult =
			validateBuildingOwnership(buildingId, cmd.issuingPlayer, state);
		if (ownershipResult != CommandValidationResult::Valid)
			return ownershipResult;

		// Check building is complete
		const BuildingState* building = findBuilding(buildingId, state);
		if (building->constructionProgress < 1.0f)
			return CommandValidationResult::InvalidProduction;

		// Check tech tree status — tech must not already be completed
		std::string techId = params["techId"].get<std::string>();
		auto techTreeIt = state.techTrees.find(cmd.issuingPlayer);
		if (techTreeIt != state.techTrees.end())
		{
			for (const auto& tech : techTreeIt->second)
			{
				if (tech.techId == techId)
				{
					// Already completed — nothing to research
					if (tech.status == TechStatus::Completed)
						return CommandValidationResult::UnmetPrerequisites;

					// Already being researched — building is occupied
					if (tech.status == TechStatus::Researching)
						return CommandValidationResult::BuildingOccupied;

					// Locked means prerequisites not met
					if (tech.status == TechStatus::Locked)
						return CommandValidationResult::UnmetPrerequisites;

					// Available is valid
					break;
				}
			}
		}

		return CommandValidationResult::Valid;
	}

	case MCPCommandType::SetGatherPoint:
	{
		// Validate building exists and is owned by player
		if (!params.contains("buildingId"))
			return CommandValidationResult::UnknownCommand;

		EntityId buildingId = params["buildingId"].get<EntityId>();
		return validateBuildingOwnership(buildingId, cmd.issuingPlayer, state);
	}

	case MCPCommandType::GatherResource:
	{
		// Validate units exist and are owned by player
		if (!params.contains("unitIds") || !params.contains("resourceId"))
			return CommandValidationResult::UnknownCommand;

		std::vector<EntityId> unitIds = params["unitIds"].get<std::vector<EntityId>>();
		if (unitIds.empty())
			return CommandValidationResult::InvalidGatherer;

		CommandValidationResult unitResult =
			validateUnitOwnership(unitIds, cmd.issuingPlayer, state);
		if (unitResult != CommandValidationResult::Valid)
			return CommandValidationResult::InvalidGatherer;

		// Validate resource deposit exists
		EntityId resourceId = params["resourceId"].get<EntityId>();
		const ResourceDeposit* deposit = findResource(resourceId, state);
		if (!deposit)
			return CommandValidationResult::InvalidTarget;

		// Check deposit still has resources
		if (deposit->remainingQuantity <= 0)
			return CommandValidationResult::InvalidTarget;

		return CommandValidationResult::Valid;
	}

	default:
		return CommandValidationResult::UnknownCommand;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Execution
// ─────────────────────────────────────────────────────────────────────────────

void MCPCommandExecutor::execute(MCPCommand cmd)
{
	cmd.enqueuedAt = std::chrono::steady_clock::now();
	m_commandQueue.enqueue(std::move(cmd));
}

// ─────────────────────────────────────────────────────────────────────────────
// Response helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string MCPCommandExecutor::validationResultToString(CommandValidationResult result)
{
	switch (result)
	{
	case CommandValidationResult::Valid:
		return "valid";
	case CommandValidationResult::InsufficientResources:
		return "Insufficient resources";
	case CommandValidationResult::InvalidTarget:
		return "Invalid target";
	case CommandValidationResult::InvalidPlacement:
		return "Invalid placement";
	case CommandValidationResult::UnmetPrerequisites:
		return "Unmet prerequisites";
	case CommandValidationResult::BuildingOccupied:
		return "Building is occupied";
	case CommandValidationResult::InvalidGatherer:
		return "Invalid gatherer capability";
	case CommandValidationResult::InvalidProduction:
		return "Invalid production capability";
	case CommandValidationResult::UnknownCommand:
		return "Unknown or malformed command";
	default:
		return "Unknown error";
	}
}

json MCPCommandExecutor::buildSuccessResponse(uint32_t tickNumber) const
{
	return json{
		{"status", "accepted"},
		{"tickQueued", tickNumber}
	};
}

json MCPCommandExecutor::buildErrorResponse(CommandValidationResult result) const
{
	return json{
		{"status", "error"},
		{"error", validationResultToString(result)}
	};
}

// ─────────────────────────────────────────────────────────────────────────────
// Handle methods
// ─────────────────────────────────────────────────────────────────────────────

json MCPCommandExecutor::handleTrain(PlayerId player, EntityId building, const std::string& unitType)
{
	MCPCommand cmd;
	cmd.type = MCPCommandType::TrainUnit;
	cmd.issuingPlayer = player;
	cmd.requestId = 0;
	cmd.parameters = json{
		{"buildingId", building},
		{"unitType", unitType}
	}.dump();

	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot)
		return buildErrorResponse(CommandValidationResult::UnknownCommand);

	CommandValidationResult result = validate(cmd, *snapshot);
	if (result != CommandValidationResult::Valid)
		return buildErrorResponse(result);

	execute(std::move(cmd));
	return buildSuccessResponse(snapshot->tickNumber);
}

json MCPCommandExecutor::handleMove(PlayerId player, std::vector<EntityId> units, Vec3 target,
	std::optional<std::string> formation)
{
	json params = {
		{"unitIds", units},
		{"targetPosition", {{"x", target.x}, {"y", target.y}, {"z", target.z}}}
	};
	if (formation.has_value())
		params["formation"] = formation.value();

	MCPCommand cmd;
	cmd.type = MCPCommandType::MoveUnits;
	cmd.issuingPlayer = player;
	cmd.requestId = 0;
	cmd.parameters = params.dump();

	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot)
		return buildErrorResponse(CommandValidationResult::UnknownCommand);

	CommandValidationResult result = validate(cmd, *snapshot);
	if (result != CommandValidationResult::Valid)
		return buildErrorResponse(result);

	execute(std::move(cmd));
	return buildSuccessResponse(snapshot->tickNumber);
}

json MCPCommandExecutor::handleAttack(PlayerId player, std::vector<EntityId> units, EntityId target)
{
	MCPCommand cmd;
	cmd.type = MCPCommandType::AttackTarget;
	cmd.issuingPlayer = player;
	cmd.requestId = 0;
	cmd.parameters = json{
		{"unitIds", units},
		{"targetId", target}
	}.dump();

	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot)
		return buildErrorResponse(CommandValidationResult::UnknownCommand);

	CommandValidationResult result = validate(cmd, *snapshot);
	if (result != CommandValidationResult::Valid)
		return buildErrorResponse(result);

	execute(std::move(cmd));
	return buildSuccessResponse(snapshot->tickNumber);
}

json MCPCommandExecutor::handleAttackMove(PlayerId player, std::vector<EntityId> units, Vec3 target)
{
	MCPCommand cmd;
	cmd.type = MCPCommandType::AttackMove;
	cmd.issuingPlayer = player;
	cmd.requestId = 0;
	cmd.parameters = json{
		{"unitIds", units},
		{"targetPosition", {{"x", target.x}, {"y", target.y}, {"z", target.z}}}
	}.dump();

	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot)
		return buildErrorResponse(CommandValidationResult::UnknownCommand);

	CommandValidationResult result = validate(cmd, *snapshot);
	if (result != CommandValidationResult::Valid)
		return buildErrorResponse(result);

	execute(std::move(cmd));
	return buildSuccessResponse(snapshot->tickNumber);
}

json MCPCommandExecutor::handleBuild(PlayerId player, const std::string& buildingType, Vec3 position,
	std::vector<EntityId> builders)
{
	MCPCommand cmd;
	cmd.type = MCPCommandType::BuildStructure;
	cmd.issuingPlayer = player;
	cmd.requestId = 0;
	cmd.parameters = json{
		{"buildingType", buildingType},
		{"position", {{"x", position.x}, {"y", position.y}, {"z", position.z}}},
		{"builderIds", builders}
	}.dump();

	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot)
		return buildErrorResponse(CommandValidationResult::UnknownCommand);

	CommandValidationResult result = validate(cmd, *snapshot);
	if (result != CommandValidationResult::Valid)
		return buildErrorResponse(result);

	execute(std::move(cmd));
	return buildSuccessResponse(snapshot->tickNumber);
}

json MCPCommandExecutor::handleResearch(PlayerId player, const std::string& techId, EntityId building)
{
	MCPCommand cmd;
	cmd.type = MCPCommandType::ResearchTech;
	cmd.issuingPlayer = player;
	cmd.requestId = 0;
	cmd.parameters = json{
		{"techId", techId},
		{"buildingId", building}
	}.dump();

	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot)
		return buildErrorResponse(CommandValidationResult::UnknownCommand);

	CommandValidationResult result = validate(cmd, *snapshot);
	if (result != CommandValidationResult::Valid)
		return buildErrorResponse(result);

	execute(std::move(cmd));
	return buildSuccessResponse(snapshot->tickNumber);
}

json MCPCommandExecutor::handleSetGatherPoint(PlayerId player, EntityId building,
	std::variant<Vec3, EntityId> target)
{
	json params = {{"buildingId", building}};

	if (std::holds_alternative<Vec3>(target))
	{
		Vec3 pos = std::get<Vec3>(target);
		params["targetPosition"] = {{"x", pos.x}, {"y", pos.y}, {"z", pos.z}};
	}
	else
	{
		params["targetEntityId"] = std::get<EntityId>(target);
	}

	MCPCommand cmd;
	cmd.type = MCPCommandType::SetGatherPoint;
	cmd.issuingPlayer = player;
	cmd.requestId = 0;
	cmd.parameters = params.dump();

	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot)
		return buildErrorResponse(CommandValidationResult::UnknownCommand);

	CommandValidationResult result = validate(cmd, *snapshot);
	if (result != CommandValidationResult::Valid)
		return buildErrorResponse(result);

	execute(std::move(cmd));
	return buildSuccessResponse(snapshot->tickNumber);
}

json MCPCommandExecutor::handleGather(PlayerId player, std::vector<EntityId> units, EntityId resource)
{
	MCPCommand cmd;
	cmd.type = MCPCommandType::GatherResource;
	cmd.issuingPlayer = player;
	cmd.requestId = 0;
	cmd.parameters = json{
		{"unitIds", units},
		{"resourceId", resource}
	}.dump();

	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot)
		return buildErrorResponse(CommandValidationResult::UnknownCommand);

	CommandValidationResult result = validate(cmd, *snapshot);
	if (result != CommandValidationResult::Valid)
		return buildErrorResponse(result);

	execute(std::move(cmd));
	return buildSuccessResponse(snapshot->tickNumber);
}

} // namespace MCP
