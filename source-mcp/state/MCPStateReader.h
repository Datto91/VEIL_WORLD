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

#ifndef INCLUDED_MCP_STATE_READER
#define INCLUDED_MCP_STATE_READER

#include "mcp/MCPTypes.h"
#include "mcp/state/MCPGameStateSnapshot.h"

#include <nlohmann/json.hpp>

namespace MCP
{

/**
 * Reads game state from the most recent snapshot and formats it
 * as JSON for serving to AI agent queries via the MCP protocol.
 *
 * All methods are const and thread-safe -- they only read from the
 * immutable snapshot. The snapshot itself is protected by a shared_mutex
 * inside MCPGameStateSnapshot, allowing multiple concurrent readers.
 *
 * Each query method applies fog-of-war filtering based on the requesting
 * player's visibility, ensuring AI agents can only see what their player
 * would normally observe. (Full filtering implemented in task 6.4;
 * currently permissive.)
 */
class MCPStateReader
{
public:
	explicit MCPStateReader(MCPGameStateSnapshot& snapshot);
	~MCPStateReader() = default;

	// Non-copyable
	MCPStateReader(const MCPStateReader&) = delete;
	MCPStateReader& operator=(const MCPStateReader&) = delete;

	/**
	 * Query all units visible to the given player.
	 * Validates: Requirements 3.1, 4.1
	 */
	nlohmann::json queryUnits(PlayerId viewer) const;

	/**
	 * Query all buildings visible to the given player.
	 * Validates: Requirements 4.1, 4.2
	 */
	nlohmann::json queryBuildings(PlayerId viewer) const;

	/**
	 * Query the resource levels and gathering rates for a player.
	 * Returns food, wood, stone, metal counts and rates.
	 * Validates: Requirements 5.1, 5.2
	 */
	nlohmann::json queryResources(PlayerId viewer) const;

	/**
	 * Query all visible resource deposits on the map.
	 * Returns positions, types, and remaining quantities.
	 * Validates: Requirements 5.3
	 */
	nlohmann::json queryResourceLocations(PlayerId viewer) const;

	/**
	 * Query terrain data as a grid at the specified resolution.
	 * Returns terrain type, elevation, passability, and visibility per cell.
	 * Validates: Requirements 6.1, 6.2
	 */
	nlohmann::json queryTerrain(PlayerId viewer, int resolution) const;

	/**
	 * Query the technology tree state for a player.
	 * Returns techs with IDs, names, prerequisites, costs, status, progress.
	 * Validates: Requirements 7.1, 7.2
	 */
	nlohmann::json queryTechTree(PlayerId viewer) const;

	/**
	 * Query information about all players in the game.
	 * Returns civilization, team, diplomacy, population per player.
	 * Validates: Requirements 8.1, 8.2
	 */
	nlohmann::json queryPlayerInfo() const;

private:
	/**
	 * Check whether a given entity is visible to the specified player.
	 * Looks up the entity's visibility in the snapshot's pre-computed
	 * visibility map (captured from ICmpRangeManager at tick boundary).
	 * Returns true if the entity is Visible or Explored (fogged but known).
	 */
	bool isEntityVisible(EntityId entity, PlayerId viewer) const;

	MCPGameStateSnapshot& m_snapshot;
};

} // namespace MCP

#endif // INCLUDED_MCP_STATE_READER
