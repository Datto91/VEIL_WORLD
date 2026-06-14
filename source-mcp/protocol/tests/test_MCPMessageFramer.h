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

#include "lib/self_test.h"

#include "mcp/protocol/MCPMessageFramer.h"

#include <nlohmann/json.hpp>

class TestMCPMessageFramer : public CxxTest::TestSuite
{
public:
	// --- MCPMessageFramer::extractMessages tests ---

	void test_extract_single_complete_message()
	{
		MCP::MCPMessageFramer framer;
		std::string buffer = "Content-Length: 17\r\n\r\n{\"hello\":\"world\"}";

		auto messages = framer.extractMessages(buffer);

		TS_ASSERT_EQUALS(messages.size(), 1u);
		TS_ASSERT_EQUALS(messages[0], "{\"hello\":\"world\"}");
		TS_ASSERT(buffer.empty());
	}

	void test_extract_multiple_messages()
	{
		MCP::MCPMessageFramer framer;
		std::string body1 = "{\"a\":1}";
		std::string body2 = "{\"b\":2}";
		std::string buffer =
			"Content-Length: " + std::to_string(body1.size()) + "\r\n\r\n" + body1 +
			"Content-Length: " + std::to_string(body2.size()) + "\r\n\r\n" + body2;

		auto messages = framer.extractMessages(buffer);

		TS_ASSERT_EQUALS(messages.size(), 2u);
		TS_ASSERT_EQUALS(messages[0], body1);
		TS_ASSERT_EQUALS(messages[1], body2);
		TS_ASSERT(buffer.empty());
	}

	void test_partial_header_leaves_buffer_intact()
	{
		MCP::MCPMessageFramer framer;
		std::string buffer = "Content-Length: 10\r\n";

		auto messages = framer.extractMessages(buffer);

		TS_ASSERT_EQUALS(messages.size(), 0u);
		TS_ASSERT_EQUALS(buffer, "Content-Length: 10\r\n");
	}

	void test_partial_body_leaves_buffer_intact()
	{
		MCP::MCPMessageFramer framer;
		std::string buffer = "Content-Length: 20\r\n\r\n{\"partial\":tru";

		auto messages = framer.extractMessages(buffer);

		TS_ASSERT_EQUALS(messages.size(), 0u);
		TS_ASSERT_EQUALS(buffer, "Content-Length: 20\r\n\r\n{\"partial\":tru");
	}

	void test_one_complete_one_partial()
	{
		MCP::MCPMessageFramer framer;
		std::string body1 = "{\"complete\":true}";
		std::string buffer =
			"Content-Length: " + std::to_string(body1.size()) + "\r\n\r\n" + body1 +
			"Content-Length: 50\r\n\r\n{\"inc";

		auto messages = framer.extractMessages(buffer);

		TS_ASSERT_EQUALS(messages.size(), 1u);
		TS_ASSERT_EQUALS(messages[0], body1);
		// The partial message remains in the buffer.
		TS_ASSERT_EQUALS(buffer, "Content-Length: 50\r\n\r\n{\"inc");
	}

	void test_empty_buffer()
	{
		MCP::MCPMessageFramer framer;
		std::string buffer;

		auto messages = framer.extractMessages(buffer);

		TS_ASSERT_EQUALS(messages.size(), 0u);
		TS_ASSERT(buffer.empty());
	}

	void test_negative_content_length_skipped()
	{
		MCP::MCPMessageFramer framer;
		// Negative content-length followed by a valid message.
		std::string valid = "{\"ok\":1}";
		std::string buffer =
			"Content-Length: -5\r\n\r\n" +
			std::string("Content-Length: ") + std::to_string(valid.size()) + "\r\n\r\n" + valid;

		auto messages = framer.extractMessages(buffer);

		TS_ASSERT_EQUALS(messages.size(), 1u);
		TS_ASSERT_EQUALS(messages[0], valid);
		TS_ASSERT(buffer.empty());
	}

	void test_non_numeric_content_length_skipped()
	{
		MCP::MCPMessageFramer framer;
		std::string valid = "{\"ok\":1}";
		std::string buffer =
			"Content-Length: abc\r\n\r\n" +
			std::string("Content-Length: ") + std::to_string(valid.size()) + "\r\n\r\n" + valid;

		auto messages = framer.extractMessages(buffer);

		TS_ASSERT_EQUALS(messages.size(), 1u);
		TS_ASSERT_EQUALS(messages[0], valid);
	}

	void test_oversized_content_length_skipped()
	{
		MCP::MCPMessageFramer framer;
		std::string valid = "{\"ok\":1}";
		// 10MB is over the 4MB limit.
		std::string buffer =
			"Content-Length: 10485760\r\n\r\n" +
			std::string("Content-Length: ") + std::to_string(valid.size()) + "\r\n\r\n" + valid;

		auto messages = framer.extractMessages(buffer);

		TS_ASSERT_EQUALS(messages.size(), 1u);
		TS_ASSERT_EQUALS(messages[0], valid);
	}

	void test_case_insensitive_header()
	{
		MCP::MCPMessageFramer framer;
		std::string buffer = "content-length: 17\r\n\r\n{\"hello\":\"world\"}";

		auto messages = framer.extractMessages(buffer);

		TS_ASSERT_EQUALS(messages.size(), 1u);
		TS_ASSERT_EQUALS(messages[0], "{\"hello\":\"world\"}");
	}

	void test_mixed_case_header()
	{
		MCP::MCPMessageFramer framer;
		std::string buffer = "CONTENT-LENGTH: 17\r\n\r\n{\"hello\":\"world\"}";

		auto messages = framer.extractMessages(buffer);

		TS_ASSERT_EQUALS(messages.size(), 1u);
		TS_ASSERT_EQUALS(messages[0], "{\"hello\":\"world\"}");
	}

	// --- MCPMessageFramer::frameMessage tests ---

	void test_frame_message_format()
	{
		nlohmann::json msg = {{"jsonrpc", "2.0"}, {"id", 1}, {"method", "test"}};
		std::string framed = MCP::MCPMessageFramer::frameMessage(msg);

		// Should start with Content-Length header.
		TS_ASSERT(framed.find("Content-Length: ") == 0);
		// Should have the delimiter.
		TS_ASSERT(framed.find("\r\n\r\n") != std::string::npos);

		// The body after the delimiter should be valid JSON.
		size_t bodyStart = framed.find("\r\n\r\n") + 4;
		std::string body = framed.substr(bodyStart);
		nlohmann::json parsed = nlohmann::json::parse(body);
		TS_ASSERT_EQUALS(parsed["jsonrpc"], "2.0");
		TS_ASSERT_EQUALS(parsed["id"], 1);
		TS_ASSERT_EQUALS(parsed["method"], "test");
	}

	void test_frame_message_round_trip()
	{
		MCP::MCPMessageFramer framer;
		nlohmann::json original = {
			{"jsonrpc", "2.0"},
			{"id", 42},
			{"method", "initialize"},
			{"params", nlohmann::json::object()}
		};

		std::string framed = MCP::MCPMessageFramer::frameMessage(original);

		// Extract it back.
		auto messages = framer.extractMessages(framed);
		TS_ASSERT_EQUALS(messages.size(), 1u);

		nlohmann::json recovered = nlohmann::json::parse(messages[0]);
		TS_ASSERT_EQUALS(recovered, original);
	}

	// --- MCPJsonRpcParser tests ---

	void test_parse_valid_request()
	{
		std::string raw = R"({"jsonrpc":"2.0","id":1,"method":"test","params":{}})";

		auto result = MCP::MCPJsonRpcParser::parseRequest(raw);

		TS_ASSERT(result.success);
		TS_ASSERT_EQUALS(result.parsed["jsonrpc"], "2.0");
		TS_ASSERT_EQUALS(result.parsed["id"], 1);
		TS_ASSERT_EQUALS(result.parsed["method"], "test");
	}

	void test_parse_notification_no_id()
	{
		std::string raw = R"({"jsonrpc":"2.0","method":"initialized"})";

		auto result = MCP::MCPJsonRpcParser::parseRequest(raw);

		TS_ASSERT(result.success);
		TS_ASSERT_EQUALS(result.parsed["method"], "initialized");
		TS_ASSERT(!result.parsed.contains("id"));
	}

	void test_parse_malformed_json()
	{
		std::string raw = "{not valid json at all";

		auto result = MCP::MCPJsonRpcParser::parseRequest(raw);

		TS_ASSERT(!result.success);
		TS_ASSERT_EQUALS(result.errorResponse["error"]["code"], MCP::JsonRpcError::ParseError);
		TS_ASSERT_EQUALS(result.errorResponse["jsonrpc"], "2.0");
		TS_ASSERT(result.errorResponse["id"].is_null());
	}

	void test_parse_missing_jsonrpc_field()
	{
		std::string raw = R"({"id":1,"method":"test"})";

		auto result = MCP::MCPJsonRpcParser::parseRequest(raw);

		TS_ASSERT(!result.success);
		TS_ASSERT_EQUALS(result.errorResponse["error"]["code"], MCP::JsonRpcError::InvalidRequest);
	}

	void test_parse_wrong_jsonrpc_version()
	{
		std::string raw = R"({"jsonrpc":"1.0","id":1,"method":"test"})";

		auto result = MCP::MCPJsonRpcParser::parseRequest(raw);

		TS_ASSERT(!result.success);
		TS_ASSERT_EQUALS(result.errorResponse["error"]["code"], MCP::JsonRpcError::InvalidRequest);
	}

	void test_parse_missing_method()
	{
		std::string raw = R"({"jsonrpc":"2.0","id":1})";

		auto result = MCP::MCPJsonRpcParser::parseRequest(raw);

		TS_ASSERT(!result.success);
		TS_ASSERT_EQUALS(result.errorResponse["error"]["code"], MCP::JsonRpcError::InvalidRequest);
	}

	void test_parse_non_string_method()
	{
		std::string raw = R"({"jsonrpc":"2.0","id":1,"method":123})";

		auto result = MCP::MCPJsonRpcParser::parseRequest(raw);

		TS_ASSERT(!result.success);
		TS_ASSERT_EQUALS(result.errorResponse["error"]["code"], MCP::JsonRpcError::InvalidRequest);
	}

	void test_parse_invalid_id_type()
	{
		std::string raw = R"({"jsonrpc":"2.0","id":[],"method":"test"})";

		auto result = MCP::MCPJsonRpcParser::parseRequest(raw);

		TS_ASSERT(!result.success);
		TS_ASSERT_EQUALS(result.errorResponse["error"]["code"], MCP::JsonRpcError::InvalidRequest);
	}

	void test_parse_invalid_params_type()
	{
		std::string raw = R"({"jsonrpc":"2.0","id":1,"method":"test","params":"string"})";

		auto result = MCP::MCPJsonRpcParser::parseRequest(raw);

		TS_ASSERT(!result.success);
		TS_ASSERT_EQUALS(result.errorResponse["error"]["code"], MCP::JsonRpcError::InvalidRequest);
	}

	void test_parse_array_params_valid()
	{
		std::string raw = R"({"jsonrpc":"2.0","id":1,"method":"test","params":[1,2,3]})";

		auto result = MCP::MCPJsonRpcParser::parseRequest(raw);

		TS_ASSERT(result.success);
	}

	void test_parse_string_id_valid()
	{
		std::string raw = R"({"jsonrpc":"2.0","id":"abc","method":"test"})";

		auto result = MCP::MCPJsonRpcParser::parseRequest(raw);

		TS_ASSERT(result.success);
		TS_ASSERT_EQUALS(result.parsed["id"], "abc");
	}

	void test_parse_null_id_valid()
	{
		std::string raw = R"({"jsonrpc":"2.0","id":null,"method":"test"})";

		auto result = MCP::MCPJsonRpcParser::parseRequest(raw);

		TS_ASSERT(result.success);
		TS_ASSERT(result.parsed["id"].is_null());
	}

	// --- MCPJsonRpcParser::buildErrorResponse tests ---

	void test_build_error_response_structure()
	{
		auto response = MCP::MCPJsonRpcParser::buildErrorResponse(-32700, "Parse error", nullptr);

		TS_ASSERT_EQUALS(response["jsonrpc"], "2.0");
		TS_ASSERT(response["id"].is_null());
		TS_ASSERT_EQUALS(response["error"]["code"], -32700);
		TS_ASSERT_EQUALS(response["error"]["message"], "Parse error");
	}

	void test_build_error_response_with_id()
	{
		auto response = MCP::MCPJsonRpcParser::buildErrorResponse(-32600, "Invalid Request", 42);

		TS_ASSERT_EQUALS(response["id"], 42);
		TS_ASSERT_EQUALS(response["error"]["code"], -32600);
	}

	void test_build_error_response_with_string_id()
	{
		auto response = MCP::MCPJsonRpcParser::buildErrorResponse(-32601, "Method not found", "req-1");

		TS_ASSERT_EQUALS(response["id"], "req-1");
		TS_ASSERT_EQUALS(response["error"]["code"], -32601);
	}

	// --- isValidJsonRpc tests ---

	void test_is_valid_jsonrpc_minimal()
	{
		nlohmann::json msg = {{"jsonrpc", "2.0"}, {"method", "test"}};
		TS_ASSERT(MCP::MCPJsonRpcParser::isValidJsonRpc(msg));
	}

	void test_is_valid_jsonrpc_full()
	{
		nlohmann::json msg = {
			{"jsonrpc", "2.0"},
			{"id", 1},
			{"method", "tools/call"},
			{"params", {{"name", "test"}}}
		};
		TS_ASSERT(MCP::MCPJsonRpcParser::isValidJsonRpc(msg));
	}

	void test_is_valid_jsonrpc_non_object()
	{
		nlohmann::json msg = nlohmann::json::array({1, 2, 3});
		TS_ASSERT(!MCP::MCPJsonRpcParser::isValidJsonRpc(msg));
	}

	void test_is_valid_jsonrpc_empty_method()
	{
		nlohmann::json msg = {{"jsonrpc", "2.0"}, {"method", ""}};
		TS_ASSERT(!MCP::MCPJsonRpcParser::isValidJsonRpc(msg));
	}
};
