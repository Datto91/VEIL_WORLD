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

#ifndef INCLUDED_MCP_MESSAGE_FRAMER
#define INCLUDED_MCP_MESSAGE_FRAMER

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace MCP
{

/**
 * JSON-RPC 2.0 standard error codes.
 */
namespace JsonRpcError
{
	static constexpr int ParseError = -32700;
	static constexpr int InvalidRequest = -32600;
	static constexpr int MethodNotFound = -32601;
	static constexpr int InvalidParams = -32602;
	static constexpr int InternalError = -32603;
} // namespace JsonRpcError

/**
 * Result of attempting to parse a JSON-RPC request.
 *
 * On success, contains the parsed JSON object.
 * On failure, contains a JSON-RPC error response ready to send back.
 */
struct ParseResult
{
	bool success = false;
	nlohmann::json parsed;       // The parsed request (if success == true)
	nlohmann::json errorResponse; // The error response to send (if success == false)
};

/**
 * Handles MCP/LSP-style message framing (Content-Length header protocol).
 *
 * Messages are framed as:
 *   Content-Length: <byte-count>\r\n
 *   \r\n
 *   <JSON body of exactly byte-count bytes>
 *
 * The framer extracts complete messages from a byte buffer, handling:
 * - Partial headers (not enough bytes for the full header yet)
 * - Partial bodies (Content-Length received but body not fully arrived)
 * - Multiple complete messages in a single buffer
 * - Malformed Content-Length values (negative, non-numeric, too large)
 */
class MCPMessageFramer
{
public:
	/**
	 * Maximum allowed message body size (4 MB) to prevent memory exhaustion.
	 */
	static constexpr size_t MAX_MESSAGE_SIZE = 4 * 1024 * 1024;

	/**
	 * Extract zero or more complete framed messages from the buffer.
	 *
	 * Consumes bytes from the front of the buffer for each complete message
	 * found. Leaves any partial message data in the buffer for future reads.
	 *
	 * @param buffer The receive buffer (modified in place — consumed bytes removed).
	 * @return Vector of raw JSON body strings extracted from complete frames.
	 */
	std::vector<std::string> extractMessages(std::string& buffer);

	/**
	 * Wrap a JSON message with the Content-Length header for sending.
	 *
	 * @param msg The JSON payload to frame.
	 * @return The framed message string (header + body) ready to send over the wire.
	 */
	static std::string frameMessage(const nlohmann::json& msg);
};

/**
 * Parses and validates JSON-RPC 2.0 request messages.
 *
 * Handles:
 * - Parsing raw JSON strings into structured objects
 * - Validating JSON-RPC 2.0 message structure
 * - Building standard error responses
 */
class MCPJsonRpcParser
{
public:
	/**
	 * Parse a raw JSON string into a validated JSON-RPC 2.0 request.
	 *
	 * @param rawJson The raw JSON string from a framed message.
	 * @return ParseResult indicating success (with parsed JSON) or failure (with error response).
	 */
	static ParseResult parseRequest(const std::string& rawJson);

	/**
	 * Validate that a JSON object conforms to JSON-RPC 2.0 request structure.
	 *
	 * A valid request must have:
	 * - "jsonrpc" field with value "2.0"
	 * - "method" field that is a string
	 * - Optional "id" field (string, number, or null)
	 * - Optional "params" field (object or array)
	 *
	 * @param msg The JSON object to validate.
	 * @return true if the message is a valid JSON-RPC 2.0 request.
	 */
	static bool isValidJsonRpc(const nlohmann::json& msg);

	/**
	 * Build a JSON-RPC 2.0 error response.
	 *
	 * @param code The error code (e.g., -32700 for parse error).
	 * @param message Human-readable error description.
	 * @param id The request ID to echo back (null if the ID could not be determined).
	 * @return A complete JSON-RPC 2.0 error response object.
	 */
	static nlohmann::json buildErrorResponse(int code, const std::string& message,
		const nlohmann::json& id = nullptr);
};

} // namespace MCP

#endif // INCLUDED_MCP_MESSAGE_FRAMER
