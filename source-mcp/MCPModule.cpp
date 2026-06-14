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

// Pull in the headers from the default precompiled header,
// even though mcp doesn't use precompiled headers.
#include "lib/precompiled.h"

#include "mcp/MCPModule.h"

#include "mcp/commands/MCPCommandQueue.h"
#include "mcp/state/MCPGameStateSnapshot.h"
#include "ps/CLogger.h"

namespace MCP
{

MCPModule& MCPModule::getInstance()
{
	static MCPModule instance;
	return instance;
}

void MCPModule::initialize(const MCPConfig& config)
{
	if (m_active)
	{
		LOGWARNING("MCP: Module already initialized, skipping re-initialization.");
		return;
	}

	m_config = config;

	LOGMESSAGE("MCP: Initializing module (port=%d, maxSessions=%zu, logVerbosity=%d).",
		m_config.port, m_config.maxSessions, m_config.logVerbosity);

	// TODO: Initialize subsystems once their implementations exist.
	// Each subsystem creation is wrapped in a try/catch so that a failure
	// (e.g., port unavailable for network transport) results in a graceful
	// fallback rather than a crash.
	try
	{
		m_commandQueue = std::make_unique<MCPCommandQueue>();
		m_snapshot = std::make_unique<MCPGameStateSnapshot>();
		// m_stateReader = std::make_unique<MCPStateReader>(*m_snapshot);
		// m_commandExecutor = std::make_unique<MCPCommandExecutor>(*m_snapshot, *m_commandQueue);
		// m_sessionManager = std::make_unique<MCPSessionManager>(m_config.maxSessions);
		// m_transport = std::make_unique<MCPNetworkTransport>();
		// m_transport->start(m_config.port);

		m_active = true;
		LOGMESSAGE("MCP: Module initialized successfully on port %d.", m_config.port);
	}
	catch (const std::exception& e)
	{
		LOGERROR("MCP: Failed to initialize module: %s. MCP will be inactive.", e.what());
		// Clean up any partially-created subsystems.
		m_transport.reset();
		m_sessionManager.reset();
		m_commandExecutor.reset();
		m_stateReader.reset();
		m_snapshot.reset();
		m_commandQueue.reset();
		m_active = false;
	}
}

void MCPModule::shutdown()
{
	if (!m_active)
	{
		LOGMESSAGE("MCP: Module not active, nothing to shut down.");
		return;
	}

	LOGMESSAGE("MCP: Shutting down module...");

	// Shutdown subsystems in reverse order of initialization.
	// Network transport first to stop accepting new connections.
	if (m_transport)
	{
		// m_transport->stop();
		m_transport.reset();
	}

	if (m_sessionManager)
		m_sessionManager.reset();

	if (m_commandExecutor)
		m_commandExecutor.reset();

	if (m_stateReader)
		m_stateReader.reset();

	if (m_snapshot)
		m_snapshot.reset();

	if (m_commandQueue)
		m_commandQueue.reset();

	m_active = false;

	LOGMESSAGE("MCP: Module shut down successfully.");
}

bool MCPModule::isActive() const
{
	return m_active;
}

} // namespace MCP
