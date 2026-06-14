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

#include "lib/precompiled.h"

#include "mcp/protocol/MCPProtocolHandler.h"

#include "mcp/commands/MCPCommandExecutor.h"
#include "mcp/network/MCPSessionManager.h"
#include "mcp/state/MCPStateReader.h"

namespace MCP
{

MCPProtocolHandler::MCPProtocolHandler(MCPSessionManager& sessionManager, MCPStateReader& stateReader,
	MCPCommandExecutor& commandExecutor)
	: m_sessionManager(sessionManager)
	, m_stateReader(stateReader)
	, m_commandExecutor(commandExecutor)
{
}

MCPProtocolHandler::~MCPProtocolHandler() = default;

nlohmann::json MCPProtocolHandler::handleRequest(SessionId session, const nlohmann::json& request)
{
	const std::string method = request.value("method", "");
	const nlohmann::json id = request.contains("id") ? request["id"] : nullptr;
	const nlohmann::json params = request.value("params", nlohmann::json::object());

	// Route based on method name
	if (method == "initialize")
	{
		return handleInitialize(session, id);
	}
	else if (method == "initialized")
	{
		// Notification: no response expected
		handleInitialized(session);
		return nullptr;
	}
	else if (method == "resources/read")
	{
		return handleResourceRead(session, params, id);
	}
	else if (method == "tools/call")
	{
		return handleToolCall(session, params, id);
	}
	else
	{
		// Unknown method
		return MCPJsonRpcParser::buildErrorResponse(
			JsonRpcError::MethodNotFound,
			"Method not found: " + method,
			id
		);
	}
}

nlohmann::json MCPProtocolHandler::handleInitialize(SessionId session, const nlohmann::json& id)
{
	// Transition session to Negotiating state
	MCPSession* sessionPtr = m_sessionManager.getSession(session);
	if (sessionPtr)
	{
		sessionPtr->state = ConnectionState::Negotiating;
	}

	// Build the initialize response per MCP specification
	nlohmann::json result = buildCapabilitiesResponse();

	return {
		{"jsonrpc", "2.0"},
		{"id", id},
		{"result", result}
	};
}

void MCPProtocolHandler::handleInitialized(SessionId session)
{
	// Transition session to Active state — fully initialized
	MCPSession* sessionPtr = m_sessionManager.getSession(session);
	if (sessionPtr)
	{
		sessionPtr->state = ConnectionState::Active;
	}
}

nlohmann::json MCPProtocolHandler::handleResourceRead(SessionId session,
	const nlohmann::json& params, const nlohmann::json& id)
{
	// Validate that URI parameter is present
	if (!params.contains("uri") || !params["uri"].is_string())
	{
		return MCPJsonRpcParser::buildErrorResponse(
			JsonRpcError::InvalidParams,
			"Missing required parameter: uri",
			id
		);
	}

	const std::string uri = params["uri"].get<std::string>();

	// Look up the session to get the assigned player
	MCPSession* sessionPtr = m_sessionManager.getSession(session);
	if (!sessionPtr)
	{
		return MCPJsonRpcParser::buildErrorResponse(
			JsonRpcError::InternalError,
			"Session not found",
			id
		);
	}

	PlayerId playerId = sessionPtr->assignedPlayer;

	// Route based on resource URI
	nlohmann::json stateData;

	if (uri == "game://units")
	{
		stateData = m_stateReader.queryUnits(playerId);
	}
	else if (uri == "game://buildings")
	{
		stateData = m_stateReader.queryBuildings(playerId);
	}
	else if (uri == "game://resources")
	{
		stateData = m_stateReader.queryResources(playerId);
	}
	else if (uri == "game://resources/map")
	{
		stateData = m_stateReader.queryResourceLocations(playerId);
	}
	else if (uri == "game://terrain")
	{
		int resolution = 4; // Default resolution
		if (params.contains("resolution") && params["resolution"].is_number_integer())
		{
			resolution = params["resolution"].get<int>();
		}
		stateData = m_stateReader.queryTerrain(playerId, resolution);
	}
	else if (uri == "game://tech")
	{
		stateData = m_stateReader.queryTechTree(playerId);
	}
	else if (uri == "game://players")
	{
		stateData = m_stateReader.queryPlayerInfo();
	}
	else
	{
		return MCPJsonRpcParser::buildErrorResponse(
			JsonRpcError::MethodNotFound,
			"Unknown resource URI: " + uri,
			id
		);
	}

	// Build the MCP resource read response
	return {
		{"jsonrpc", "2.0"},
		{"id", id},
		{"result", {
			{"contents", nlohmann::json::array({
				{
					{"uri", uri},
					{"mimeType", "application/json"},
					{"text", stateData.dump()}
				}
			})}
		}}
	};
}

nlohmann::json MCPProtocolHandler::handleToolCall(SessionId session,
	const nlohmann::json& params, const nlohmann::json& id)
{
	// Validate required "name" parameter
	if (!params.contains("name") || !params["name"].is_string())
	{
		return MCPJsonRpcParser::buildErrorResponse(
			JsonRpcError::InvalidParams,
			"Missing required parameter: name",
			id
		);
	}

	const std::string toolName = params["name"].get<std::string>();

	// Validate required "arguments" parameter
	if (!params.contains("arguments") || !params["arguments"].is_object())
	{
		return MCPJsonRpcParser::buildErrorResponse(
			JsonRpcError::InvalidParams,
			"Missing required parameter: arguments",
			id
		);
	}

	const nlohmann::json& args = params["arguments"];

	// Look up the session to get the assigned player
	MCPSession* sessionPtr = m_sessionManager.getSession(session);
	if (!sessionPtr)
	{
		return MCPJsonRpcParser::buildErrorResponse(
			JsonRpcError::InternalError,
			"Session not found",
			id
		);
	}

	PlayerId playerId = sessionPtr->assignedPlayer;

	// Route based on tool name and execute the command
	nlohmann::json result;

	if (toolName == "train_unit")
	{
		if (!args.contains("buildingId") || !args["buildingId"].is_number_integer() ||
			!args.contains("unitType") || !args["unitType"].is_string())
		{
			return MCPJsonRpcParser::buildErrorResponse(
				JsonRpcError::InvalidParams,
				"train_unit requires integer buildingId and string unitType",
				id
			);
		}

		result = m_commandExecutor.handleTrain(
			playerId,
			args["buildingId"].get<EntityId>(),
			args["unitType"].get<std::string>()
		);
	}
	else if (toolName == "move_units")
	{
		if (!args.contains("unitIds") || !args["unitIds"].is_array() ||
			!args.contains("targetPosition") || !args["targetPosition"].is_object())
		{
			return MCPJsonRpcParser::buildErrorResponse(
				JsonRpcError::InvalidParams,
				"move_units requires array unitIds and object targetPosition",
				id
			);
		}

		const nlohmann::json& posJson = args["targetPosition"];
		if (!posJson.contains("x") || !posJson.contains("y") || !posJson.contains("z"))
		{
			return MCPJsonRpcParser::buildErrorResponse(
				JsonRpcError::InvalidParams,
				"targetPosition requires x, y, z fields",
				id
			);
		}

		std::vector<EntityId> unitIds;
		for (const auto& uid : args["unitIds"])
		{
			if (!uid.is_number_integer())
			{
				return MCPJsonRpcParser::buildErrorResponse(
					JsonRpcError::InvalidParams,
					"unitIds must contain integers",
					id
				);
			}
			unitIds.push_back(uid.get<EntityId>());
		}

		Vec3 targetPos{
			posJson["x"].get<float>(),
			posJson["y"].get<float>(),
			posJson["z"].get<float>()
		};

		std::optional<std::string> formation;
		if (args.contains("formation") && args["formation"].is_string())
		{
			formation = args["formation"].get<std::string>();
		}

		result = m_commandExecutor.handleMove(playerId, std::move(unitIds), targetPos, formation);
	}
	else if (toolName == "attack_target")
	{
		if (!args.contains("unitIds") || !args["unitIds"].is_array() ||
			!args.contains("targetId") || !args["targetId"].is_number_integer())
		{
			return MCPJsonRpcParser::buildErrorResponse(
				JsonRpcError::InvalidParams,
				"attack_target requires array unitIds and integer targetId",
				id
			);
		}

		std::vector<EntityId> unitIds;
		for (const auto& uid : args["unitIds"])
		{
			if (!uid.is_number_integer())
			{
				return MCPJsonRpcParser::buildErrorResponse(
					JsonRpcError::InvalidParams,
					"unitIds must contain integers",
					id
				);
			}
			unitIds.push_back(uid.get<EntityId>());
		}

		result = m_commandExecutor.handleAttack(
			playerId,
			std::move(unitIds),
			args["targetId"].get<EntityId>()
		);
	}
	else if (toolName == "attack_move")
	{
		if (!args.contains("unitIds") || !args["unitIds"].is_array() ||
			!args.contains("targetPosition") || !args["targetPosition"].is_object())
		{
			return MCPJsonRpcParser::buildErrorResponse(
				JsonRpcError::InvalidParams,
				"attack_move requires array unitIds and object targetPosition",
				id
			);
		}

		const nlohmann::json& posJson = args["targetPosition"];
		if (!posJson.contains("x") || !posJson.contains("y") || !posJson.contains("z"))
		{
			return MCPJsonRpcParser::buildErrorResponse(
				JsonRpcError::InvalidParams,
				"targetPosition requires x, y, z fields",
				id
			);
		}

		std::vector<EntityId> unitIds;
		for (const auto& uid : args["unitIds"])
		{
			if (!uid.is_number_integer())
			{
				return MCPJsonRpcParser::buildErrorResponse(
					JsonRpcError::InvalidParams,
					"unitIds must contain integers",
					id
				);
			}
			unitIds.push_back(uid.get<EntityId>());
		}

		Vec3 targetPos{
			posJson["x"].get<float>(),
			posJson["y"].get<float>(),
			posJson["z"].get<float>()
		};

		result = m_commandExecutor.handleAttackMove(playerId, std::move(unitIds), targetPos);
	}
	else if (toolName == "build_structure")
	{
		if (!args.contains("buildingType") || !args["buildingType"].is_string() ||
			!args.contains("position") || !args["position"].is_object() ||
			!args.contains("builderIds") || !args["builderIds"].is_array())
		{
			return MCPJsonRpcParser::buildErrorResponse(
				JsonRpcError::InvalidParams,
				"build_structure requires string buildingType, object position, and array builderIds",
				id
			);
		}

		const nlohmann::json& posJson = args["position"];
		if (!posJson.contains("x") || !posJson.contains("y") || !posJson.contains("z"))
		{
			return MCPJsonRpcParser::buildErrorResponse(
				JsonRpcError::InvalidParams,
				"position requires x, y, z fields",
				id
			);
		}

		Vec3 position{
			posJson["x"].get<float>(),
			posJson["y"].get<float>(),
			posJson["z"].get<float>()
		};

		std::vector<EntityId> builderIds;
		for (const auto& bid : args["builderIds"])
		{
			if (!bid.is_number_integer())
			{
				return MCPJsonRpcParser::buildErrorResponse(
					JsonRpcError::InvalidParams,
					"builderIds must contain integers",
					id
				);
			}
			builderIds.push_back(bid.get<EntityId>());
		}

		result = m_commandExecutor.handleBuild(
			playerId,
			args["buildingType"].get<std::string>(),
			position,
			std::move(builderIds)
		);
	}
	else if (toolName == "research_tech")
	{
		if (!args.contains("techId") || !args["techId"].is_string() ||
			!args.contains("buildingId") || !args["buildingId"].is_number_integer())
		{
			return MCPJsonRpcParser::buildErrorResponse(
				JsonRpcError::InvalidParams,
				"research_tech requires string techId and integer buildingId",
				id
			);
		}

		result = m_commandExecutor.handleResearch(
			playerId,
			args["techId"].get<std::string>(),
			args["buildingId"].get<EntityId>()
		);
	}
	else if (toolName == "set_gather_point")
	{
		if (!args.contains("buildingId") || !args["buildingId"].is_number_integer())
		{
			return MCPJsonRpcParser::buildErrorResponse(
				JsonRpcError::InvalidParams,
				"set_gather_point requires integer buildingId",
				id
			);
		}

		// Target can be either a position or an entity ID
		std::variant<Vec3, EntityId> target;

		if (args.contains("targetEntityId") && args["targetEntityId"].is_number_integer())
		{
			target = args["targetEntityId"].get<EntityId>();
		}
		else if (args.contains("targetPosition") && args["targetPosition"].is_object())
		{
			const nlohmann::json& posJson = args["targetPosition"];
			if (!posJson.contains("x") || !posJson.contains("y") || !posJson.contains("z"))
			{
				return MCPJsonRpcParser::buildErrorResponse(
					JsonRpcError::InvalidParams,
					"targetPosition requires x, y, z fields",
					id
				);
			}
			target = Vec3{
				posJson["x"].get<float>(),
				posJson["y"].get<float>(),
				posJson["z"].get<float>()
			};
		}
		else
		{
			return MCPJsonRpcParser::buildErrorResponse(
				JsonRpcError::InvalidParams,
				"set_gather_point requires either targetPosition or targetEntityId",
				id
			);
		}

		result = m_commandExecutor.handleSetGatherPoint(
			playerId,
			args["buildingId"].get<EntityId>(),
			target
		);
	}
	else if (toolName == "gather_resource")
	{
		if (!args.contains("unitIds") || !args["unitIds"].is_array() ||
			!args.contains("resourceId") || !args["resourceId"].is_number_integer())
		{
			return MCPJsonRpcParser::buildErrorResponse(
				JsonRpcError::InvalidParams,
				"gather_resource requires array unitIds and integer resourceId",
				id
			);
		}

		std::vector<EntityId> unitIds;
		for (const auto& uid : args["unitIds"])
		{
			if (!uid.is_number_integer())
			{
				return MCPJsonRpcParser::buildErrorResponse(
					JsonRpcError::InvalidParams,
					"unitIds must contain integers",
					id
				);
			}
			unitIds.push_back(uid.get<EntityId>());
		}

		result = m_commandExecutor.handleGather(
			playerId,
			std::move(unitIds),
			args["resourceId"].get<EntityId>()
		);
	}
	else
	{
		// Unknown tool name
		return MCPJsonRpcParser::buildErrorResponse(
			JsonRpcError::MethodNotFound,
			"Unknown tool: " + toolName,
			id
		);
	}

	// Wrap the executor result in the appropriate JSON-RPC response
	if (result.contains("status") && result["status"] == "error")
	{
		std::string errorMessage = result.value("message", "Command validation failed");
		return {
			{"jsonrpc", "2.0"},
			{"id", id},
			{"error", {
				{"code", -32000},
				{"message", errorMessage},
				{"data", result}
			}}
		};
	}

	// Success: wrap in MCP tools/call response format
	return {
		{"jsonrpc", "2.0"},
		{"id", id},
		{"result", {
			{"content", nlohmann::json::array({
				{
					{"type", "text"},
					{"text", result.dump()}
				}
			})}
		}}
	};
}

nlohmann::json MCPProtocolHandler::buildCapabilitiesResponse() const
{
	return {
		{"serverInfo", {
			{"name", "0ad-mcp-server"},
			{"version", "1.0.0"}
		}},
		{"capabilities", {
			{"resources", buildResourceList()},
			{"tools", buildToolList()}
		}}
	};
}

nlohmann::json MCPProtocolHandler::buildResourceList() const
{
	return nlohmann::json::array({
		{{"uri", "game://units"}, {"description", "All visible units with position, health, owner, type, and current action"}},
		{{"uri", "game://buildings"}, {"description", "All visible buildings with position, health, owner, type, construction progress, and production queue"}},
		{{"uri", "game://resources"}, {"description", "Player resource levels including food, wood, stone, metal, and gathering rates"}},
		{{"uri", "game://resources/map"}, {"description", "Visible resource deposits with position, remaining quantity, and resource type"}},
		{{"uri", "game://terrain"}, {"description", "Map terrain data with type, elevation, passability, and fog-of-war state"}},
		{{"uri", "game://tech"}, {"description", "Technology tree state with research status and progress"}},
		{{"uri", "game://players"}, {"description", "All player info including civilization, team, diplomacy, and population"}}
	});
}

nlohmann::json MCPProtocolHandler::buildToolList() const
{
	return nlohmann::json::array({
		// train_unit
		{
			{"name", "train_unit"},
			{"description", "Queue unit production at a building"},
			{"inputSchema", {
				{"type", "object"},
				{"properties", {
					{"buildingId", {{"type", "integer"}, {"description", "Entity ID of the production building"}}},
					{"unitType", {{"type", "string"}, {"description", "Template name of the unit to train"}}}
				}},
				{"required", nlohmann::json::array({"buildingId", "unitType"})}
			}}
		},
		// move_units
		{
			{"name", "move_units"},
			{"description", "Move units to a target position"},
			{"inputSchema", {
				{"type", "object"},
				{"properties", {
					{"unitIds", {{"type", "array"}, {"items", {{"type", "integer"}}}, {"description", "Entity IDs of units to move"}}},
					{"targetPosition", {{"type", "object"}, {"properties", {
						{"x", {{"type", "number"}}},
						{"y", {{"type", "number"}}},
						{"z", {{"type", "number"}}}
					}}, {"description", "Target world position"}}},
					{"formation", {{"type", "string"}, {"description", "Optional formation type for the group"}}}
				}},
				{"required", nlohmann::json::array({"unitIds", "targetPosition"})}
			}}
		},
		// attack_target
		{
			{"name", "attack_target"},
			{"description", "Order units to attack a specific target entity"},
			{"inputSchema", {
				{"type", "object"},
				{"properties", {
					{"unitIds", {{"type", "array"}, {"items", {{"type", "integer"}}}, {"description", "Entity IDs of attacking units"}}},
					{"targetId", {{"type", "integer"}, {"description", "Entity ID of the target to attack"}}}
				}},
				{"required", nlohmann::json::array({"unitIds", "targetId"})}
			}}
		},
		// attack_move
		{
			{"name", "attack_move"},
			{"description", "Move units to a position while engaging enemies along the path"},
			{"inputSchema", {
				{"type", "object"},
				{"properties", {
					{"unitIds", {{"type", "array"}, {"items", {{"type", "integer"}}}, {"description", "Entity IDs of units to attack-move"}}},
					{"targetPosition", {{"type", "object"}, {"properties", {
						{"x", {{"type", "number"}}},
						{"y", {{"type", "number"}}},
						{"z", {{"type", "number"}}}
					}}, {"description", "Target world position"}}}
				}},
				{"required", nlohmann::json::array({"unitIds", "targetPosition"})}
			}}
		},
		// build_structure
		{
			{"name", "build_structure"},
			{"description", "Place a building foundation and assign builders to construct it"},
			{"inputSchema", {
				{"type", "object"},
				{"properties", {
					{"buildingType", {{"type", "string"}, {"description", "Template name of the building to construct"}}},
					{"position", {{"type", "object"}, {"properties", {
						{"x", {{"type", "number"}}},
						{"y", {{"type", "number"}}},
						{"z", {{"type", "number"}}}
					}}, {"description", "World position for building placement"}}},
					{"builderIds", {{"type", "array"}, {"items", {{"type", "integer"}}}, {"description", "Entity IDs of builder units"}}}
				}},
				{"required", nlohmann::json::array({"buildingType", "position", "builderIds"})}
			}}
		},
		// research_tech
		{
			{"name", "research_tech"},
			{"description", "Begin researching a technology at a building"},
			{"inputSchema", {
				{"type", "object"},
				{"properties", {
					{"techId", {{"type", "string"}, {"description", "Identifier of the technology to research"}}},
					{"buildingId", {{"type", "integer"}, {"description", "Entity ID of the building performing research"}}}
				}},
				{"required", nlohmann::json::array({"techId", "buildingId"})}
			}}
		},
		// set_gather_point
		{
			{"name", "set_gather_point"},
			{"description", "Set a building's rally/gather point to a position or entity"},
			{"inputSchema", {
				{"type", "object"},
				{"properties", {
					{"buildingId", {{"type", "integer"}, {"description", "Entity ID of the building"}}},
					{"targetPosition", {{"type", "object"}, {"properties", {
						{"x", {{"type", "number"}}},
						{"y", {{"type", "number"}}},
						{"z", {{"type", "number"}}}
					}}, {"description", "Target position (use this or targetEntityId)"}}},
					{"targetEntityId", {{"type", "integer"}, {"description", "Target entity ID (use this or targetPosition)"}}}
				}},
				{"required", nlohmann::json::array({"buildingId"})}
			}}
		},
		// gather_resource
		{
			{"name", "gather_resource"},
			{"description", "Order units to gather from a resource entity"},
			{"inputSchema", {
				{"type", "object"},
				{"properties", {
					{"unitIds", {{"type", "array"}, {"items", {{"type", "integer"}}}, {"description", "Entity IDs of gathering units"}}},
					{"resourceId", {{"type", "integer"}, {"description", "Entity ID of the resource deposit to gather from"}}}
				}},
				{"required", nlohmann::json::array({"unitIds", "resourceId"})}
			}}
		}
	});
}

} // namespace MCP
