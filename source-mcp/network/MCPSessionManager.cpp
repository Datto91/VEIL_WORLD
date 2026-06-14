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

#include "mcp/network/MCPSessionManager.h"

#include "ps/CLogger.h"

namespace MCP
{

MCPSessionManager::MCPSessionManager(size_t maxSessions)
	: m_maxSessions(maxSessions)
{
	LOGMESSAGE("MCP Session: Manager initialized with max %zu sessions.", m_maxSessions);
}

MCPSessionManager::~MCPSessionManager()
{
	std::lock_guard<std::mutex> lock(m_sessionMutex);
	if (!m_sessions.empty())
	{
		LOGWARNING("MCP Session: Manager destroyed with %zu active sessions remaining.",
			m_sessions.size());
		m_sessions.clear();
	}
}

SessionId MCPSessionManager::createSession(PlayerId player)
{
	std::lock_guard<std::mutex> lock(m_sessionMutex);

	// Enforce the maximum concurrent sessions limit.
	if (m_sessions.size() >= m_maxSessions)
	{
		LOGWARNING("MCP Session: Cannot create session — at maximum capacity (%zu/%zu).",
			m_sessions.size(), m_maxSessions);
		return 0;
	}

	// Assign a unique session ID.
	SessionId id = nextSessionId();

	// Build the session record.
	MCPSession session;
	session.id = id;
	session.assignedPlayer = player;
	session.state = ConnectionState::Connected;
	session.connectedAt = std::chrono::steady_clock::now();

	m_sessions.emplace(id, session);

	LOGMESSAGE("MCP Session: Created session %u for player %d (%zu/%zu active).",
		id, player, m_sessions.size(), m_maxSessions);

	return id;
}

void MCPSessionManager::destroySession(SessionId id)
{
	std::lock_guard<std::mutex> lock(m_sessionMutex);

	auto it = m_sessions.find(id);
	if (it == m_sessions.end())
	{
		LOGWARNING("MCP Session: Cannot destroy session %u — not found.", id);
		return;
	}

	PlayerId player = it->second.assignedPlayer;
	m_sessions.erase(it);

	LOGMESSAGE("MCP Session: Destroyed session %u (player %d). %zu sessions remaining.",
		id, player, m_sessions.size());
}

MCPSession* MCPSessionManager::getSession(SessionId id)
{
	std::lock_guard<std::mutex> lock(m_sessionMutex);

	auto it = m_sessions.find(id);
	if (it == m_sessions.end())
		return nullptr;

	return &it->second;
}

size_t MCPSessionManager::activeSessionCount() const
{
	std::lock_guard<std::mutex> lock(m_sessionMutex);
	return m_sessions.size();
}

size_t MCPSessionManager::maxSessions() const
{
	return m_maxSessions;
}

void MCPSessionManager::handleUnexpectedDisconnection(SessionId id)
{
	std::lock_guard<std::mutex> lock(m_sessionMutex);

	auto it = m_sessions.find(id);
	if (it == m_sessions.end())
	{
		LOGWARNING("MCP Session: Unexpected disconnection for unknown session %u.", id);
		return;
	}

	PlayerId player = it->second.assignedPlayer;

	LOGWARNING("MCP Session: Unexpected disconnection for session %u (player %d). Cleaning up.",
		id, player);

	// Mark state as disconnected before removal (useful if any observer checks state).
	it->second.state = ConnectionState::Disconnected;

	// Remove the session.
	m_sessions.erase(it);

	LOGMESSAGE("MCP Session: Cleaned up session %u after unexpected disconnection. %zu sessions remaining.",
		id, m_sessions.size());
}

void MCPSessionManager::handleGracefulDisconnect(SessionId id)
{
	std::lock_guard<std::mutex> lock(m_sessionMutex);

	auto it = m_sessions.find(id);
	if (it == m_sessions.end())
	{
		LOGWARNING("MCP Session: Graceful disconnect requested for unknown session %u.", id);
		return;
	}

	PlayerId player = it->second.assignedPlayer;

	LOGMESSAGE("MCP Session: Graceful disconnect for session %u (player %d).", id, player);

	// Transition through Disconnecting state.
	it->second.state = ConnectionState::Disconnecting;

	// Remove the session and release resources.
	m_sessions.erase(it);

	LOGMESSAGE("MCP Session: Session %u disconnected gracefully. %zu sessions remaining.",
		id, m_sessions.size());
}

SessionId MCPSessionManager::nextSessionId()
{
	// Must be called with m_sessionMutex held.
	return m_nextId++;
}

} // namespace MCP
