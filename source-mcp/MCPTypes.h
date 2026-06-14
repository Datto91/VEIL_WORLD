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

#ifndef INCLUDED_MCP_TYPES
#define INCLUDED_MCP_TYPES

#include <cstdint>
#include <string>
#include <vector>
#include <variant>

namespace MCP
{

/**
 * Unique identifier for an active AI agent session.
 */
using SessionId = uint32_t;

/**
 * Player identifier, matching 0 A.D.'s player_id_t convention.
 * Player 0 is Gaia, players 1+ are human/AI players.
 */
using PlayerId = int32_t;

/**
 * Entity identifier, matching 0 A.D.'s entity_id_t convention.
 */
using EntityId = uint32_t;

/**
 * 3D position vector used for world-space coordinates.
 */
struct Vec3
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

/**
 * Connection state of an AI agent session.
 */
enum class ConnectionState
{
	Connecting,
	Connected,
	Negotiating,
	Active,
	Disconnecting,
	Disconnected
};

/**
 * Fog-of-war visibility state for terrain cells and entities.
 */
enum class FogOfWarState
{
	Unexplored,
	Explored,
	Visible
};

/**
 * Research status of a technology in the tech tree.
 */
enum class TechStatus
{
	Locked,
	Available,
	Researching,
	Completed
};

/**
 * Types of commands that can be issued through the MCP interface.
 */
enum class MCPCommandType
{
	TrainUnit,
	MoveUnits,
	AttackTarget,
	AttackMove,
	BuildStructure,
	ResearchTech,
	SetGatherPoint,
	GatherResource
};

/**
 * Result of validating a command against the current game state.
 */
enum class CommandValidationResult
{
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

} // namespace MCP

#endif // INCLUDED_MCP_TYPES
