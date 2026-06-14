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

#ifndef INCLUDED_MCP_NETWORK_TRANSPORT
#define INCLUDED_MCP_NETWORK_TRANSPORT

#include "mcp/MCPTypes.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace MCP
{

/**
 * Represents a single connected TCP client with its socket and receive buffer.
 *
 * Each connected AI agent gets a TCPConnection entry that tracks the socket
 * handle, accumulated receive buffer (for message framing), and the assigned
 * session identifier.
 */
struct TCPConnection
{
	/** Winsock SOCKET handle for this connection (stored as uintptr_t to avoid Winsock header dependency). */
	uintptr_t socket = ~static_cast<uintptr_t>(0); // INVALID_SOCKET equivalent

	/** Accumulated receive buffer for incoming data (message framing handled by higher layer). */
	std::string recvBuffer;

	/** Session identifier assigned to this connection. 0 means not yet assigned. */
	SessionId sessionId = 0;

	/** Whether this connection is still active. */
	bool active = false;
};

/**
 * TCP network transport for the MCP server.
 *
 * Manages a non-blocking TCP server socket using Winsock2, accepts incoming
 * AI agent connections, and reads/writes data on a dedicated network thread.
 * Uses select() for multiplexing across multiple connections.
 *
 * The transport layer handles raw TCP I/O. Message framing and protocol parsing
 * are handled by higher-level components that register a request handler via
 * setRequestHandler().
 *
 * Threading: The network thread runs independently from the simulation thread.
 * The request handler callback is invoked on the network thread.
 */
class MCPNetworkTransport
{
public:
	MCPNetworkTransport();
	~MCPNetworkTransport();

	/**
	 * Start the network transport, binding to the specified port.
	 *
	 * Initializes Winsock2, creates a non-blocking TCP socket, binds to the
	 * given port, begins listening, and spawns the network thread.
	 *
	 * @param port TCP port number to listen on.
	 * @throws std::runtime_error if socket creation or binding fails.
	 */
	void start(uint16_t port);

	/**
	 * Stop the network transport gracefully.
	 *
	 * Signals the network thread to exit, closes all client connections,
	 * closes the listen socket, joins the network thread, and calls WSACleanup.
	 */
	void stop();

	/**
	 * Send a JSON response to a specific session.
	 *
	 * Thread-safe. Can be called from any thread. The data will be queued
	 * and sent on the network thread.
	 *
	 * @param session The session to send to.
	 * @param response The JSON response payload.
	 */
	void sendResponse(SessionId session, const nlohmann::json& response);

	/**
	 * Register the handler called when complete data is received from a connection.
	 *
	 * The handler receives the session ID and a reference to the connection's
	 * receive buffer. The handler is responsible for extracting complete messages
	 * from the buffer and removing consumed bytes.
	 *
	 * @param handler Callback invoked on the network thread when data arrives.
	 */
	void setDataHandler(std::function<void(SessionId, std::string&)> handler);

	/**
	 * Register a callback invoked when a new connection is accepted.
	 *
	 * @param handler Callback receiving the new session ID.
	 */
	void setConnectHandler(std::function<void(SessionId)> handler);

	/**
	 * Register a callback invoked when a connection is closed.
	 *
	 * @param handler Callback receiving the disconnected session ID.
	 */
	void setDisconnectHandler(std::function<void(SessionId)> handler);

	/**
	 * Check if the transport is currently running.
	 *
	 * @return true if the network thread is active and accepting connections.
	 */
	bool isRunning() const;

	// Non-copyable, non-movable
	MCPNetworkTransport(const MCPNetworkTransport&) = delete;
	MCPNetworkTransport& operator=(const MCPNetworkTransport&) = delete;
	MCPNetworkTransport(MCPNetworkTransport&&) = delete;
	MCPNetworkTransport& operator=(MCPNetworkTransport&&) = delete;

private:
	/** Main loop for the network thread. */
	void networkThreadFunc();

	/** Accept new incoming connections on the listen socket. */
	void acceptNewConnections();

	/** Read available data from all connected clients. */
	void readFromClients();

	/** Close a specific connection and notify disconnect handler. */
	void closeConnection(size_t index);

	/** Assign the next available session ID. */
	SessionId nextSessionId();

	/** The listen socket handle (stored as uintptr_t). */
	uintptr_t m_listenSocket = ~static_cast<uintptr_t>(0);

	/** Port the server is bound to. */
	uint16_t m_port = 0;

	/** Network thread running the accept/read loop. */
	std::thread m_networkThread;

	/** Flag signaling the network thread to stop. */
	std::atomic<bool> m_shutdownFlag{false};

	/** Flag indicating the transport is running. */
	std::atomic<bool> m_running{false};

	/** All active client connections. Protected by m_connectionsMutex. */
	std::vector<TCPConnection> m_connections;
	std::mutex m_connectionsMutex;

	/** Counter for assigning session IDs. */
	std::atomic<SessionId> m_nextSessionId{1};

	/** Callback invoked when data is received. */
	std::function<void(SessionId, std::string&)> m_dataHandler;

	/** Callback invoked when a new client connects. */
	std::function<void(SessionId)> m_connectHandler;

	/** Callback invoked when a client disconnects. */
	std::function<void(SessionId)> m_disconnectHandler;

	/** Mutex for handler callbacks. */
	std::mutex m_handlerMutex;
};

} // namespace MCP

#endif // INCLUDED_MCP_NETWORK_TRANSPORT
