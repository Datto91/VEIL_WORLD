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

#include "mcp/network/MCPNetworkTransport.h"

#include "ps/CLogger.h"

#include <nlohmann/json.hpp>

// Winsock2 must be included after precompiled.h to avoid conflicts.
#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <stdexcept>

namespace MCP
{

// Size of the read buffer for each recv() call.
static constexpr int RECV_BUFFER_SIZE = 8192;

// Maximum number of pending connections in the listen backlog.
static constexpr int LISTEN_BACKLOG = 8;

// Timeout for select() in milliseconds. Kept short so the shutdown flag
// is checked frequently.
static constexpr long SELECT_TIMEOUT_MS = 100;

MCPNetworkTransport::MCPNetworkTransport() = default;

MCPNetworkTransport::~MCPNetworkTransport()
{
	if (m_running.load())
		stop();
}

void MCPNetworkTransport::start(uint16_t port)
{
	if (m_running.load())
	{
		LOGWARNING("MCP Network: Transport already running, ignoring start().");
		return;
	}

	m_port = port;
	m_shutdownFlag.store(false);

	// Initialize Winsock2.
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		throw std::runtime_error(
			std::string("MCP Network: WSAStartup failed with error ") + std::to_string(result));
	}

	// Create TCP socket.
	SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		WSACleanup();
		throw std::runtime_error(
			std::string("MCP Network: socket() failed with error ") + std::to_string(err));
	}

	// Allow address reuse so we can restart quickly.
	int optVal = 1;
	setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR,
		reinterpret_cast<const char*>(&optVal), sizeof(optVal));

	// Bind to the specified port on all interfaces.
	sockaddr_in bindAddr{};
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_addr.s_addr = INADDR_ANY;
	bindAddr.sin_port = htons(port);

	result = bind(listenSock, reinterpret_cast<sockaddr*>(&bindAddr), sizeof(bindAddr));
	if (result == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		closesocket(listenSock);
		WSACleanup();
		throw std::runtime_error(
			std::string("MCP Network: bind() failed on port ") + std::to_string(port) +
			" with error " + std::to_string(err));
	}

	// Start listening.
	result = listen(listenSock, LISTEN_BACKLOG);
	if (result == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		closesocket(listenSock);
		WSACleanup();
		throw std::runtime_error(
			std::string("MCP Network: listen() failed with error ") + std::to_string(err));
	}

	// Set the listen socket to non-blocking mode.
	u_long nonBlocking = 1;
	ioctlsocket(listenSock, FIONBIO, &nonBlocking);

	m_listenSocket = static_cast<uintptr_t>(listenSock);
	m_running.store(true);

	LOGMESSAGE("MCP Network: Listening on port %d.", port);

	// Spawn the network thread.
	m_networkThread = std::thread(&MCPNetworkTransport::networkThreadFunc, this);
}

void MCPNetworkTransport::stop()
{
	if (!m_running.load())
	{
		LOGMESSAGE("MCP Network: Transport not running, nothing to stop.");
		return;
	}

	LOGMESSAGE("MCP Network: Stopping transport...");

	// Signal the network thread to exit.
	m_shutdownFlag.store(true);

	// Wait for the network thread to finish.
	if (m_networkThread.joinable())
		m_networkThread.join();

	// Close all client connections.
	{
		std::lock_guard<std::mutex> lock(m_connectionsMutex);
		for (auto& conn : m_connections)
		{
			if (conn.active)
			{
				closesocket(static_cast<SOCKET>(conn.socket));
				conn.active = false;
			}
		}
		m_connections.clear();
	}

	// Close the listen socket.
	if (m_listenSocket != static_cast<uintptr_t>(INVALID_SOCKET))
	{
		closesocket(static_cast<SOCKET>(m_listenSocket));
		m_listenSocket = static_cast<uintptr_t>(INVALID_SOCKET);
	}

	// Clean up Winsock.
	WSACleanup();

	m_running.store(false);
	LOGMESSAGE("MCP Network: Transport stopped.");
}

void MCPNetworkTransport::sendResponse(SessionId session, const nlohmann::json& response)
{
	std::string payload = response.dump();

	std::lock_guard<std::mutex> lock(m_connectionsMutex);
	for (auto& conn : m_connections)
	{
		if (conn.active && conn.sessionId == session)
		{
			SOCKET sock = static_cast<SOCKET>(conn.socket);
			int totalSent = 0;
			int remaining = static_cast<int>(payload.size());
			const char* data = payload.c_str();

			while (remaining > 0)
			{
				int sent = send(sock, data + totalSent, remaining, 0);
				if (sent == SOCKET_ERROR)
				{
					int err = WSAGetLastError();
					if (err == WSAEWOULDBLOCK)
					{
						// Socket buffer is full. In a production implementation
						// we'd queue the remaining data. For now, yield briefly
						// and retry.
						Sleep(1);
						continue;
					}
					LOGWARNING("MCP Network: send() failed for session %u with error %d.",
						session, err);
					break;
				}
				totalSent += sent;
				remaining -= sent;
			}
			return;
		}
	}

	LOGWARNING("MCP Network: sendResponse() could not find session %u.", session);
}

void MCPNetworkTransport::setDataHandler(std::function<void(SessionId, std::string&)> handler)
{
	std::lock_guard<std::mutex> lock(m_handlerMutex);
	m_dataHandler = std::move(handler);
}

void MCPNetworkTransport::setConnectHandler(std::function<void(SessionId)> handler)
{
	std::lock_guard<std::mutex> lock(m_handlerMutex);
	m_connectHandler = std::move(handler);
}

void MCPNetworkTransport::setDisconnectHandler(std::function<void(SessionId)> handler)
{
	std::lock_guard<std::mutex> lock(m_handlerMutex);
	m_disconnectHandler = std::move(handler);
}

bool MCPNetworkTransport::isRunning() const
{
	return m_running.load();
}

void MCPNetworkTransport::networkThreadFunc()
{
	LOGMESSAGE("MCP Network: Network thread started.");

	while (!m_shutdownFlag.load())
	{
		// Build the fd_set for select().
		fd_set readSet;
		FD_ZERO(&readSet);

		SOCKET listenSock = static_cast<SOCKET>(m_listenSocket);
		FD_SET(listenSock, &readSet);

		// Add all active client sockets.
		{
			std::lock_guard<std::mutex> lock(m_connectionsMutex);
			for (const auto& conn : m_connections)
			{
				if (conn.active)
					FD_SET(static_cast<SOCKET>(conn.socket), &readSet);
			}
		}

		// select() timeout — check shutdown flag periodically.
		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = SELECT_TIMEOUT_MS * 1000;

		// Note: On Windows the first parameter to select() is ignored,
		// but we pass 0 for correctness.
		int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);

		if (selectResult == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			// If we're shutting down, the error may be expected.
			if (m_shutdownFlag.load())
				break;

			LOGWARNING("MCP Network: select() returned error %d.", err);
			// Brief sleep to avoid busy-loop on repeated errors.
			Sleep(10);
			continue;
		}

		if (selectResult == 0)
		{
			// Timeout — no activity. Loop back to check shutdown flag.
			continue;
		}

		// Check for new incoming connections on the listen socket.
		if (FD_ISSET(listenSock, &readSet))
		{
			acceptNewConnections();
		}

		// Check for data on client connections.
		readFromClients();
	}

	LOGMESSAGE("MCP Network: Network thread exiting.");
}

void MCPNetworkTransport::acceptNewConnections()
{
	SOCKET listenSock = static_cast<SOCKET>(m_listenSocket);

	while (true)
	{
		sockaddr_in clientAddr{};
		int addrLen = sizeof(clientAddr);

		SOCKET clientSock = accept(listenSock, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
		if (clientSock == INVALID_SOCKET)
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				// No more pending connections.
				break;
			}
			LOGWARNING("MCP Network: accept() failed with error %d.", err);
			break;
		}

		// Set the new client socket to non-blocking.
		u_long nonBlocking = 1;
		ioctlsocket(clientSock, FIONBIO, &nonBlocking);

		// Assign a session ID.
		SessionId sid = nextSessionId();

		// Build the connection record.
		TCPConnection conn;
		conn.socket = static_cast<uintptr_t>(clientSock);
		conn.sessionId = sid;
		conn.active = true;
		conn.recvBuffer.reserve(RECV_BUFFER_SIZE);

		{
			std::lock_guard<std::mutex> lock(m_connectionsMutex);
			m_connections.push_back(std::move(conn));
		}

		// Log the connection.
		char addrStr[INET_ADDRSTRLEN] = {};
		inet_ntop(AF_INET, &clientAddr.sin_addr, addrStr, sizeof(addrStr));
		LOGMESSAGE("MCP Network: Accepted connection from %s:%d (session %u).",
			addrStr, ntohs(clientAddr.sin_port), sid);

		// Notify the connect handler.
		{
			std::lock_guard<std::mutex> lock(m_handlerMutex);
			if (m_connectHandler)
				m_connectHandler(sid);
		}
	}
}

void MCPNetworkTransport::readFromClients()
{
	char buffer[RECV_BUFFER_SIZE];

	std::lock_guard<std::mutex> lock(m_connectionsMutex);

	for (size_t i = 0; i < m_connections.size(); /* no increment */)
	{
		auto& conn = m_connections[i];
		if (!conn.active)
		{
			++i;
			continue;
		}

		SOCKET sock = static_cast<SOCKET>(conn.socket);

		int bytesRead = recv(sock, buffer, RECV_BUFFER_SIZE, 0);
		if (bytesRead > 0)
		{
			// Append received data to the connection's buffer.
			conn.recvBuffer.append(buffer, static_cast<size_t>(bytesRead));

			// Notify the data handler so higher layers can extract messages.
			{
				std::lock_guard<std::mutex> handlerLock(m_handlerMutex);
				if (m_dataHandler)
					m_dataHandler(conn.sessionId, conn.recvBuffer);
			}

			++i;
		}
		else if (bytesRead == 0)
		{
			// Graceful disconnect by the client.
			LOGMESSAGE("MCP Network: Client session %u disconnected gracefully.", conn.sessionId);
			closeConnection(i);
			// Don't increment i — the vector has been modified.
		}
		else
		{
			// bytesRead < 0 — error.
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				// No data available right now, that's fine.
				++i;
			}
			else
			{
				// Real error — connection lost.
				LOGWARNING("MCP Network: recv() error %d on session %u, closing connection.",
					err, conn.sessionId);
				closeConnection(i);
			}
		}
	}
}

void MCPNetworkTransport::closeConnection(size_t index)
{
	// Must be called with m_connectionsMutex held.
	auto& conn = m_connections[index];
	SessionId sid = conn.sessionId;

	closesocket(static_cast<SOCKET>(conn.socket));
	conn.active = false;

	// Remove the connection from the vector.
	m_connections.erase(m_connections.begin() + static_cast<ptrdiff_t>(index));

	// Notify the disconnect handler (release connections lock first to avoid deadlock,
	// but we already hold it — notify inline since the handler should be quick).
	{
		std::lock_guard<std::mutex> handlerLock(m_handlerMutex);
		if (m_disconnectHandler)
			m_disconnectHandler(sid);
	}
}

SessionId MCPNetworkTransport::nextSessionId()
{
	return m_nextSessionId.fetch_add(1, std::memory_order_relaxed);
}

} // namespace MCP
