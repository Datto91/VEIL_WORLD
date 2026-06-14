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

#ifndef INCLUDED_MCP_PROTOCOL_HANDLER
#define INCLUDED_MCP_PROTOCOL_HANDLER

#include "mcp/MCPTypes.h"
#include "mcp/protocol/MCPMessageFramer.h"

#include <nlohmann/json.hpp>

#include <string>

namespace MCP
{

class MCPCommandExecutor;
class MCPSessionManager;
class MCPStateReader;

/**
 * Handles MCP protocol-level request routing and capability negotiation.
 *
 * This class is responsible for:
 * - Processing the MCP `initialize` handshake and returning server capabilities
 * - Handling the `initialized` notification to transition sessions to Active state
 * - Routing `resources/read` and `tools/call` requests to appropriate handlers
 * - Returning proper JSON-RPC error responses for unknown methods
 *
 * The protocol handler sits between the network transport (which handles framing
 * and JSON-RPC parsing) and the domain-specific handlers (state reader, command
 * executor) that implement actual game logic.
 */
class MCPProtocolHandler
{
public:
	/**
	 * Construct a protocol handler with references to session manager, state reader,
	 * and command executor.
	 *
	 * @param sessionManager Reference to the session manager for state transitions.
	 * @param stateReader Reference to the state reader for game state queries.
	 * @param commandExecutor Reference to the command executor for tool call routing.
	 */
	MCPProtocolHandler(MCPSessionManager& sessionManager, MCPStateReader& stateReader,
		MCPCommandExecutor& commandExecutor);

	~MCPProtocolHandler();

	/**
	 * Handle an incoming JSON-RPC request or notification for a given session.
	 *
	 * Routes the message based on its "method" field:
	 * - "initialize" → capability negotiation response
	 * - "initialized" → session state transition (no response)
	 * - "resources/read" → state reader stub
	 * - "tools/call" → command executor stub
	 * - Unknown → JSON-RPC -32601 Method Not Found error
	 *
	 * @param session The session ID of the requesting agent.
	 * @param request The parsed JSON-RPC request object.
	 * @return JSON response to send back, or null JSON for notifications.
	 */
	nlohmann::json handleRequest(SessionId session, const nlohmann::json& request);

	/**
	 * Build the full MCP capabilities response object.
	 *
	 * Contains server info (name, version) and capabilities listing all
	 * supported resource URIs and tool definitions with parameter schemas.
	 *
	 * @return The capabilities response suitable for the "initialize" result.
	 */
	nlohmann::json buildCapabilitiesResponse() const;

	// Non-copyable, non-movable
	MCPProtocolHandler(const MCPProtocolHandler&) = delete;
	MCPProtocolHandler& operator=(const MCPProtocolHandler&) = delete;
	MCPProtocolHandler(MCPProtocolHandler&&) = delete;
	MCPProtocolHandler& operator=(MCPProtocolHandler&&) = delete;

private:
	/**
	 * Handle the MCP "initialize" method.
	 *
	 * Returns server info and capabilities listing all supported resources
	 * and tools with their parameter schemas.
	 *
	 * @param session The session requesting initialization.
	 * @param id The JSON-RPC request ID to echo in the response.
	 * @return JSON-RPC success response with capabilities.
	 */
	nlohmann::json handleInitialize(SessionId session, const nlohmann::json& id);

	/**
	 * Handle the MCP "initialized" notification.
	 *
	 * Transitions the session to Active state. Per JSON-RPC notification
	 * semantics, no response is sent.
	 *
	 * @param session The session that completed initialization.
	 */
	void handleInitialized(SessionId session);

	/**
	 * Handle a "resources/read" request.
	 *
	 * Routes the requested resource URI to the appropriate MCPStateReader
	 * query method and returns the result in MCP resource response format.
	 *
	 * Supported URIs:
	 * - game://units, game://buildings, game://resources, game://resources/map,
	 *   game://terrain, game://tech, game://players
	 *
	 * @param session The requesting session.
	 * @param params The request parameters containing the resource URI.
	 * @param id The JSON-RPC request ID.
	 * @return JSON-RPC response with resource contents or error.
	 */
	nlohmann::json handleResourceRead(SessionId session, const nlohmann::json& params,
		const nlohmann::json& id);

	/**
	 * Handle a "tools/call" request.
	 *
	 * Routes the tool name to the appropriate MCPCommandExecutor method,
	 * validates parameters, and wraps the result in MCP tools/call response format.
	 *
	 * Supported tools: train_unit, move_units, attack_target, attack_move,
	 * build_structure, research_tech, set_gather_point, gather_resource.
	 *
	 * @param session The requesting session.
	 * @param params The request parameters containing tool name and arguments.
	 * @param id The JSON-RPC request ID.
	 * @return JSON-RPC response with tool execution result or error.
	 */
	nlohmann::json handleToolCall(SessionId session, const nlohmann::json& params,
		const nlohmann::json& id);

	/**
	 * Build the list of supported resource definitions for capabilities.
	 *
	 * @return JSON array of resource objects with uri and description.
	 */
	nlohmann::json buildResourceList() const;

	/**
	 * Build the list of supported tool definitions for capabilities.
	 *
	 * Each tool includes name, description, and inputSchema describing
	 * the expected parameters.
	 *
	 * @return JSON array of tool definition objects.
	 */
	nlohmann::json buildToolList() const;

	/** Reference to the session manager for state transitions. */
	MCPSessionManager& m_sessionManager;

	/** Reference to the state reader for game state queries. */
	MCPStateReader& m_stateReader;

	/** Reference to the command executor for tool call routing. */
	MCPCommandExecutor& m_commandExecutor;
};

} // namespace MCP

#endif // INCLUDED_MCP_PROTOCOL_HANDLER
