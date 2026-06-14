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

#ifndef INCLUDED_MCP_GAME_STATE_SNAPSHOT
#define INCLUDED_MCP_GAME_STATE_SNAPSHOT

#include "mcp/state/MCPGameStateData.h"

#include <memory>
#include <shared_mutex>

namespace MCP
{

/**
 * Thread-safe holder for an immutable game state snapshot.
 *
 * The simulation thread calls updateSnapshot() at the end of each tick
 * to publish a new immutable snapshot. The network thread calls
 * getSnapshot() to read the latest snapshot for serving AI agent queries.
 *
 * Uses a shared_mutex to allow multiple concurrent readers (network thread)
 * while ensuring exclusive access during writes (simulation thread).
 */
class MCPGameStateSnapshot
{
public:
	MCPGameStateSnapshot() = default;
	~MCPGameStateSnapshot() = default;

	// Non-copyable
	MCPGameStateSnapshot(const MCPGameStateSnapshot&) = delete;
	MCPGameStateSnapshot& operator=(const MCPGameStateSnapshot&) = delete;

	/**
	 * Replace the current snapshot with a new one.
	 * Called by the simulation thread at the end of each tick.
	 * Takes an exclusive lock — blocks readers briefly during pointer swap.
	 *
	 * @param newState Shared pointer to the new immutable game state data.
	 */
	void updateSnapshot(std::shared_ptr<const GameStateData> newState);

	/**
	 * Get the current snapshot for reading.
	 * Called by the network thread to serve AI agent queries.
	 * Takes a shared lock — multiple readers can access concurrently.
	 *
	 * @return Shared pointer to the current game state, or nullptr if no snapshot yet.
	 */
	std::shared_ptr<const GameStateData> getSnapshot() const;

private:
	std::shared_ptr<const GameStateData> m_currentSnapshot;
	mutable std::shared_mutex m_snapshotMutex;
};

} // namespace MCP

#endif // INCLUDED_MCP_GAME_STATE_SNAPSHOT
