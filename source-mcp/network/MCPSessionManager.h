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

#ifndef INCLUDED_MCP_SESSION_MANAGER
#define INCLUDED_MCP_SESSION_MANAGER

#include "mcp/MCPTypes.h"

#include <chrono>
#include <cstddef>
#include <mutex>
#include <unordered_map>

namespace MCP
{

/**
 * Represents an active AI agent session connected to the MCP server.
 *
 * Each session tracks the assigned player, connection state, and timing
 * information for the connected AI agent.
 */
struct MCPSession
{
	/** Unique session identifier assigned at creation. */
	SessionId id = 0;

	/** The player slot this session is assigned to control. */
	PlayerId assignedPlayer = 0;

	/** Current connection state of this session. */
	ConnectionState state = ConnectionState::Connecting;

	/** Timestamp when the session was established. */
	std::chrono::steady_clock::time_point connectedAt;
};

/**
 * Manages active AI agent sessions for the MCP server.
 *
 * Handles session lifecycle (creation, lookup, destruction), enforces the
 * maximum concurrent session limit from configuration, and provides
 * thread-safe access to the session map via mutex protection.
 *
 * Thread safety: All public methods are thread-safe and may be called from
 * any thread (network thread, main thread, etc.).
 */
class MCPSessionManager
{
public:
	/**
	 * Construct a session manager with the specified maximum session limit.
	 *
	 * @param maxSessions Maximum number of concurrent sessions allowed.
	 *                    When this limit is reached, createSession() returns 0.
	 */
	explicit MCPSessionManager(size_t maxSessions);

	~MCPSessionManager();

	/**
	 * Create a new session for an AI agent connecting as the given player.
	 *
	 * Assigns a unique SessionId and records the session in the active session
	 * map. If the maximum session limit has been reached, returns 0 to indicate
	 * the session could not be created.
	 *
	 * @param player The PlayerId to assign to this session.
	 * @return The new SessionId, or 0 if at maximum capacity.
	 */
	SessionId createSession(PlayerId player);

	/**
	 * Destroy an active session and clean up its resources.
	 *
	 * Removes the session from the active map and logs the destruction event.
	 * If the session does not exist, logs a warning and returns silently.
	 *
	 * @param id The SessionId of the session to destroy.
	 */
	void destroySession(SessionId id);

	/**
	 * Look up an active session by its identifier.
	 *
	 * @param id The SessionId to look up.
	 * @return Pointer to the MCPSession if found, nullptr if no such session exists.
	 *
	 * @warning The returned pointer is only valid while the caller holds no
	 *          assumption about session lifetime. If the session could be
	 *          destroyed concurrently, copy the data you need immediately.
	 */
	MCPSession* getSession(SessionId id);

	/**
	 * Get the number of currently active sessions.
	 *
	 * @return Count of active sessions.
	 */
	size_t activeSessionCount() const;

	/**
	 * Get the configured maximum session limit.
	 *
	 * @return Maximum number of concurrent sessions allowed.
	 */
	size_t maxSessions() const;

	/**
	 * Handle an unexpected disconnection for a session.
	 *
	 * Updates the session state to Disconnected and then destroys it,
	 * logging the unexpected disconnection event.
	 *
	 * @param id The SessionId that disconnected unexpectedly.
	 */
	void handleUnexpectedDisconnection(SessionId id);

	/**
	 * Handle a graceful disconnect request from an AI agent.
	 *
	 * Transitions the session state to Disconnecting, then destroys the
	 * session and releases resources.
	 *
	 * @param id The SessionId requesting disconnection.
	 */
	void handleGracefulDisconnect(SessionId id);

	// Non-copyable, non-movable
	MCPSessionManager(const MCPSessionManager&) = delete;
	MCPSessionManager& operator=(const MCPSessionManager&) = delete;
	MCPSessionManager(MCPSessionManager&&) = delete;
	MCPSessionManager& operator=(MCPSessionManager&&) = delete;

private:
	/** Assign the next unique session identifier. */
	SessionId nextSessionId();

	/** Map of active sessions keyed by SessionId. Protected by m_sessionMutex. */
	std::unordered_map<SessionId, MCPSession> m_sessions;

	/** Maximum number of concurrent sessions allowed. */
	size_t m_maxSessions;

	/** Mutex protecting all access to m_sessions. */
	mutable std::mutex m_sessionMutex;

	/** Counter for generating unique session IDs. */
	SessionId m_nextId = 1;
};

} // namespace MCP

#endif // INCLUDED_MCP_SESSION_MANAGER
