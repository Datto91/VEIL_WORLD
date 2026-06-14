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

#ifndef INCLUDED_MCP_GAME_STATE_DATA
#define INCLUDED_MCP_GAME_STATE_DATA

#include "mcp/MCPTypes.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace MCP
{

/**
 * State of a single unit at a given simulation tick.
 */
struct UnitState
{
	EntityId id;
	Vec3 position;
	int health;
	int maxHealth;
	PlayerId owner;
	std::string templateName;  // unit type identifier
	std::string currentAction; // idle, moving, attacking, gathering, building
};

/**
 * An entry in a building's production queue.
 */
struct ProductionQueueEntry
{
	std::string unitType;
	float progress;      // 0.0 to 1.0
	int timeRemaining;   // in simulation ticks
};

/**
 * State of a single building at a given simulation tick.
 */
struct BuildingState
{
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

/**
 * Resource counts and gathering rates for a single player.
 */
struct PlayerResources
{
	int food = 0;
	int wood = 0;
	int stone = 0;
	int metal = 0;
	float foodRate = 0.0f;
	float woodRate = 0.0f;
	float stoneRate = 0.0f;
	float metalRate = 0.0f;
};

/**
 * A resource deposit on the map (e.g. tree, stone mine, berry bush).
 */
struct ResourceDeposit
{
	EntityId id;
	Vec3 position;
	std::string resourceType; // food, wood, stone, metal
	int remainingQuantity;
};

/**
 * A single cell of the terrain grid.
 */
struct TerrainCell
{
	std::string terrainType;
	float elevation;
	bool passable;
	FogOfWarState visibility;
};

/**
 * Technology research state for a single technology.
 */
struct TechnologyState
{
	std::string techId;
	std::string name;
	std::vector<std::string> prerequisites;
	PlayerResources cost;
	TechStatus status;
	float researchProgress; // 0.0 to 1.0 when RESEARCHING
};

/**
 * Information about a player in the game.
 */
struct PlayerInfo
{
	PlayerId id;
	std::string civilization;
	int team;
	std::string diplomacyStance; // ally, neutral, enemy
	int currentPopulation;
	int maxPopulation;
};

/**
 * Complete snapshot of the game state at a given simulation tick.
 * This is an immutable data structure captured at tick boundaries.
 */
struct GameStateData
{
	uint32_t tickNumber = 0;
	std::vector<UnitState> units;
	std::vector<BuildingState> buildings;
	std::unordered_map<PlayerId, PlayerResources> playerResources;
	std::vector<ResourceDeposit> resourceDeposits;
	std::vector<std::vector<TerrainCell>> terrainGrid;
	int terrainResolution = 0;
	std::unordered_map<PlayerId, std::vector<TechnologyState>> techTrees;
	std::vector<PlayerInfo> players;

	/**
	 * Per-player entity visibility captured from ICmpRangeManager at tick boundary.
	 * Maps PlayerId → (EntityId → FogOfWarState).
	 * Used by MCPStateReader to filter entities by fog-of-war visibility.
	 */
	std::unordered_map<PlayerId, std::unordered_map<EntityId, FogOfWarState>> entityVisibility;

	/**
	 * Per-player terrain visibility grid.
	 * Each entry is a 2D grid (same dimensions as terrainGrid) with per-cell
	 * FogOfWarState for that player. If a player is not present in this map,
	 * terrain defaults to Visible (e.g. when fog-of-war is disabled).
	 */
	std::unordered_map<PlayerId, std::vector<std::vector<FogOfWarState>>> terrainVisibility;
};

} // namespace MCP

#endif // INCLUDED_MCP_GAME_STATE_DATA
