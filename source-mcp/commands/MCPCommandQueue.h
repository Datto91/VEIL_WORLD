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

#ifndef INCLUDED_MCP_COMMAND_QUEUE
#define INCLUDED_MCP_COMMAND_QUEUE

#include "mcp/MCPTypes.h"

#include <chrono>
#include <mutex>
#include <string>
#include <vector>

namespace MCP
{

/**
 * A command to be injected into the simulation on the next tick.
 * Commands are enqueued by the network thread (after validation)
 * and consumed by the simulation thread at the start of each tick.
 */
struct MCPCommand
{
	MCPCommandType type;
	PlayerId issuingPlayer;
	uint64_t requestId;  // JSON-RPC request ID for response correlation
	std::string parameters; // JSON-serialized type-specific parameters
	std::chrono::steady_clock::time_point enqueuedAt;
};

/**
 * Thread-safe command queue for passing commands from the network
 * thread to the simulation thread.
 *
 * The network thread calls enqueue() after validating a command.
 * The simulation thread calls drainAll() at the start of each tick
 * to retrieve all pending commands.
 *
 * Implementation uses a mutex + vector. A true lock-free queue
 * can be substituted later as an optimization if profiling warrants it.
 */
class MCPCommandQueue
{
public:
	MCPCommandQueue() = default;
	~MCPCommandQueue() = default;

	// Non-copyable
	MCPCommandQueue(const MCPCommandQueue&) = delete;
	MCPCommandQueue& operator=(const MCPCommandQueue&) = delete;

	/**
	 * Add a command to the queue.
	 * Called by the network thread after command validation.
	 * Thread-safe.
	 *
	 * @param cmd The validated command to enqueue.
	 */
	void enqueue(MCPCommand cmd);

	/**
	 * Remove and return all pending commands.
	 * Called by the simulation thread at the start of each tick.
	 * Thread-safe. Returns commands in FIFO order.
	 *
	 * @return Vector of all pending commands (empty if none queued).
	 */
	std::vector<MCPCommand> drainAll();

private:
	std::mutex m_mutex;
	std::vector<MCPCommand> m_queue;
};

} // namespace MCP

#endif // INCLUDED_MCP_COMMAND_QUEUE
