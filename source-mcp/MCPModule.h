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

#ifndef INCLUDED_MCP_MODULE
#define INCLUDED_MCP_MODULE

#include "mcp/MCPConfig.h"

#include <memory>

namespace MCP
{

// Forward declarations for subsystem components
class MCPNetworkTransport;
class MCPSessionManager;
class MCPStateReader;
class MCPCommandExecutor;
class MCPGameStateSnapshot;
class MCPCommandQueue;

/**
 * Singleton lifecycle manager for the MCP server module.
 *
 * Responsible for initializing all MCP subsystems on engine startup
 * and shutting them down gracefully on engine exit. The module hooks
 * into the engine startup/shutdown sequence and manages the lifetime
 * of the network transport, session manager, state reader, command
 * executor, game state snapshot, and command queue.
 */
class MCPModule
{
public:
	/**
	 * Get the singleton instance of the MCP module.
	 */
	static MCPModule& getInstance();

	/**
	 * Initialize the MCP server with the given configuration.
	 * Starts the network transport, creates subsystem instances,
	 * and begins accepting AI agent connections.
	 *
	 * If the configured port is unavailable, logs an error and
	 * leaves the module inactive (isActive() returns false).
	 *
	 * @param config Configuration values for the MCP server.
	 */
	void initialize(const MCPConfig& config);

	/**
	 * Shut down the MCP server gracefully.
	 * Disconnects all active sessions, stops the network transport,
	 * and releases all resources.
	 */
	void shutdown();

	/**
	 * Check if the MCP server is currently active and accepting connections.
	 *
	 * @return true if the server initialized successfully and is running.
	 */
	bool isActive() const;

	/**
	 * Get the game state snapshot holder.
	 * Used by MCPSimulationHook and MCPStateReader.
	 *
	 * @return Pointer to the snapshot, or nullptr if not initialized.
	 */
	MCPGameStateSnapshot* getSnapshot() const { return m_snapshot.get(); }

	/**
	 * Get the command queue.
	 * Used by MCPSimulationHook and MCPCommandExecutor.
	 *
	 * @return Pointer to the command queue, or nullptr if not initialized.
	 */
	MCPCommandQueue* getCommandQueue() const { return m_commandQueue.get(); }

	// Non-copyable, non-movable singleton
	MCPModule(const MCPModule&) = delete;
	MCPModule& operator=(const MCPModule&) = delete;
	MCPModule(MCPModule&&) = delete;
	MCPModule& operator=(MCPModule&&) = delete;

private:
	MCPModule() = default;
	~MCPModule() = default;

	std::unique_ptr<MCPNetworkTransport> m_transport;
	std::unique_ptr<MCPSessionManager> m_sessionManager;
	std::unique_ptr<MCPStateReader> m_stateReader;
	std::unique_ptr<MCPCommandExecutor> m_commandExecutor;
	std::unique_ptr<MCPGameStateSnapshot> m_snapshot;
	std::unique_ptr<MCPCommandQueue> m_commandQueue;
	MCPConfig m_config;
	bool m_active = false;
};

} // namespace MCP

#endif // INCLUDED_MCP_MODULE
