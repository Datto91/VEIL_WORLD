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

#include "mcp/protocol/MCPMessageFramer.h"

#include <algorithm>
#include <charconv>
#include <cstring>
#include <string_view>

namespace MCP
{

// The header/body delimiter in the Content-Length framing protocol.
static constexpr std::string_view HEADER_DELIMITER = "\r\n\r\n";

// The Content-Length header prefix (case-insensitive matching per LSP spec,
// but we produce this exact form for outgoing messages).
static constexpr std::string_view CONTENT_LENGTH_PREFIX = "Content-Length: ";

// --- MCPMessageFramer ---

std::vector<std::string> MCPMessageFramer::extractMessages(std::string& buffer)
{
	std::vector<std::string> messages;

	while (true)
	{
		// Look for the header/body delimiter (\r\n\r\n).
		size_t delimPos = buffer.find(HEADER_DELIMITER);
		if (delimPos == std::string::npos)
		{
			// Haven't received the full header yet. Wait for more data.
			break;
		}

		// Extract the header section (everything before the delimiter).
		std::string_view headerSection(buffer.data(), delimPos);

		// Parse Content-Length from the header section.
		// The header section may contain multiple headers separated by \r\n.
		// We need to find the Content-Length header (case-insensitive).
		long long contentLength = -1;

		size_t lineStart = 0;
		while (lineStart < headerSection.size())
		{
			size_t lineEnd = headerSection.find("\r\n", lineStart);
			if (lineEnd == std::string_view::npos)
				lineEnd = headerSection.size();

			std::string_view line = headerSection.substr(lineStart, lineEnd - lineStart);

			// Case-insensitive check for "Content-Length:" prefix.
			if (line.size() >= 16)
			{
				// Build a lowercase version of the first 16 chars for comparison.
				bool matches = true;
				const char* expected = "content-length: ";
				for (size_t i = 0; i < 16 && matches; ++i)
				{
					char c = line[i];
					// Convert to lowercase for comparison.
					if (c >= 'A' && c <= 'Z')
						c = static_cast<char>(c + ('a' - 'A'));
					if (c != expected[i])
						matches = false;
				}

				if (matches)
				{
					// Parse the numeric value after "Content-Length: ".
					std::string_view valueStr = line.substr(16);

					// Trim leading/trailing whitespace from the value.
					while (!valueStr.empty() && valueStr.front() == ' ')
						valueStr.remove_prefix(1);
					while (!valueStr.empty() && valueStr.back() == ' ')
						valueStr.remove_suffix(1);

					// Parse the integer value.
					auto [ptr, ec] = std::from_chars(
						valueStr.data(), valueStr.data() + valueStr.size(), contentLength);

					if (ec != std::errc{} || ptr != valueStr.data() + valueStr.size())
					{
						// Non-numeric or trailing garbage — treat as malformed.
						// Discard this frame up through the delimiter and continue.
						contentLength = -1;
					}

					break; // Found the Content-Length header, stop searching.
				}
			}

			lineStart = (lineEnd == headerSection.size()) ? lineEnd : lineEnd + 2;
		}

		// Validate Content-Length value.
		if (contentLength < 0)
		{
			// Malformed or missing Content-Length. Skip past the delimiter to try
			// to recover on the next frame boundary.
			buffer.erase(0, delimPos + HEADER_DELIMITER.size());
			continue;
		}

		if (static_cast<size_t>(contentLength) > MAX_MESSAGE_SIZE)
		{
			// Message too large — skip this frame to prevent memory exhaustion.
			buffer.erase(0, delimPos + HEADER_DELIMITER.size());
			continue;
		}

		// Check if the full body has arrived.
		size_t bodyStart = delimPos + HEADER_DELIMITER.size();
		size_t totalFrameSize = bodyStart + static_cast<size_t>(contentLength);

		if (buffer.size() < totalFrameSize)
		{
			// Body not fully received yet. Wait for more data.
			break;
		}

		// Extract the complete message body.
		std::string body = buffer.substr(bodyStart, static_cast<size_t>(contentLength));
		messages.push_back(std::move(body));

		// Remove the consumed frame from the buffer.
		buffer.erase(0, totalFrameSize);
	}

	return messages;
}

std::string MCPMessageFramer::frameMessage(const nlohmann::json& msg)
{
	std::string body = msg.dump();
	std::string framed;
	framed.reserve(CONTENT_LENGTH_PREFIX.size() + 20 + HEADER_DELIMITER.size() + body.size());
	framed.append(CONTENT_LENGTH_PREFIX);
	framed.append(std::to_string(body.size()));
	framed.append(HEADER_DELIMITER);
	framed.append(body);
	return framed;
}

// --- MCPJsonRpcParser ---

ParseResult MCPJsonRpcParser::parseRequest(const std::string& rawJson)
{
	ParseResult result;

	// Attempt to parse the raw JSON string.
	nlohmann::json parsed;
	try
	{
		parsed = nlohmann::json::parse(rawJson);
	}
	catch (const nlohmann::json::parse_error&)
	{
		// Malformed JSON — return parse error.
		result.success = false;
		result.errorResponse = buildErrorResponse(
			JsonRpcError::ParseError,
			"Parse error: invalid JSON",
			nullptr);
		return result;
	}

	// Validate JSON-RPC 2.0 structure.
	if (!isValidJsonRpc(parsed))
	{
		// Extract the ID if present (for error response correlation).
		nlohmann::json id = nullptr;
		if (parsed.is_object() && parsed.contains("id"))
			id = parsed["id"];

		result.success = false;
		result.errorResponse = buildErrorResponse(
			JsonRpcError::InvalidRequest,
			"Invalid Request: not a valid JSON-RPC 2.0 request",
			id);
		return result;
	}

	// Successfully parsed and validated.
	result.success = true;
	result.parsed = std::move(parsed);
	return result;
}

bool MCPJsonRpcParser::isValidJsonRpc(const nlohmann::json& msg)
{
	// Must be a JSON object.
	if (!msg.is_object())
		return false;

	// Must have "jsonrpc" field with value "2.0".
	if (!msg.contains("jsonrpc") || !msg["jsonrpc"].is_string() || msg["jsonrpc"] != "2.0")
		return false;

	// Must have "method" field that is a string.
	if (!msg.contains("method") || !msg["method"].is_string())
		return false;

	// "method" must not be empty.
	if (msg["method"].get<std::string>().empty())
		return false;

	// If "id" is present, it must be a string, number, or null.
	if (msg.contains("id"))
	{
		const auto& id = msg["id"];
		if (!id.is_string() && !id.is_number() && !id.is_null())
			return false;
	}

	// If "params" is present, it must be an object or array.
	if (msg.contains("params"))
	{
		const auto& params = msg["params"];
		if (!params.is_object() && !params.is_array())
			return false;
	}

	return true;
}

nlohmann::json MCPJsonRpcParser::buildErrorResponse(int code, const std::string& message,
	const nlohmann::json& id)
{
	nlohmann::json response;
	response["jsonrpc"] = "2.0";
	response["id"] = id;
	response["error"] = {
		{"code", code},
		{"message", message}
	};
	return response;
}

} // namespace MCP
