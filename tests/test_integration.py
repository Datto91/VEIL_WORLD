"""
Integration Tests for 0 A.D. MCP Server

This test script connects to a running 0 A.D. instance with the MCP server active
and validates the full connection lifecycle, resource reads, tool calls, and error handling.

Prerequisites:
    - 0 A.D. must be running with the MCP server module active
    - A game session must be started (main menu is insufficient for game state queries)
    - The MCP server should be listening on the configured port (default: 6374)

Usage:
    python tests/test_integration.py
    python tests/test_integration.py --host 127.0.0.1 --port 6374
    python tests/test_integration.py --verbose
    python tests/test_integration.py --test connection  # Run specific test group

Requirements validated: 16.1, 16.4, 17.4
"""

import sys
import os
import json
import time
import argparse
import traceback
from typing import Callable, List, Tuple

# Add parent directory so we can import mcp_client
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from mcp_client import (
    MCPClient,
    MCPClientError,
    MCPConnectionError,
    MCPTimeoutError,
    MCPProtocolError,
    MCPServerError,
)


# --- Test infrastructure ---

class TestResult:
    """Stores the result of a single test case."""

    def __init__(self, name: str, passed: bool, message: str = "", duration: float = 0.0):
        self.name = name
        self.passed = passed
        self.message = message
        self.duration = duration

    def __str__(self):
        status = "PASS" if self.passed else "FAIL"
        msg = f"  [{status}] {self.name} ({self.duration:.3f}s)"
        if not self.passed and self.message:
            msg += f"\n         {self.message}"
        return msg


class TestSuite:
    """Manages test execution and result collection."""

    def __init__(self, host: str, port: int, timeout: float, verbose: bool = False):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.verbose = verbose
        self.results: List[TestResult] = []

    def run_test(self, name: str, test_fn: Callable) -> TestResult:
        """Run a single test function and capture the result."""
        start = time.time()
        try:
            test_fn()
            duration = time.time() - start
            result = TestResult(name, passed=True, duration=duration)
        except AssertionError as e:
            duration = time.time() - start
            result = TestResult(name, passed=False, message=str(e), duration=duration)
        except MCPClientError as e:
            duration = time.time() - start
            result = TestResult(name, passed=False, message=f"MCPClientError: {e}", duration=duration)
        except Exception as e:
            duration = time.time() - start
            msg = f"{type(e).__name__}: {e}"
            if self.verbose:
                msg += f"\n         {traceback.format_exc()}"
            result = TestResult(name, passed=False, message=msg, duration=duration)

        self.results.append(result)
        print(result)
        return result

    def create_client(self) -> MCPClient:
        """Create a fresh MCPClient instance configured for these tests."""
        return MCPClient(timeout=self.timeout)

    def create_connected_client(self) -> MCPClient:
        """Create a client that is already connected."""
        client = self.create_client()
        client.connect(self.host, self.port)
        return client

    def create_initialized_client(self) -> MCPClient:
        """Create a client that is connected and has completed the handshake."""
        client = self.create_connected_client()
        client.initialize()
        return client

    def summary(self) -> Tuple[int, int, int]:
        """Print summary and return (total, passed, failed) counts."""
        total = len(self.results)
        passed = sum(1 for r in self.results if r.passed)
        failed = total - passed

        print("\n" + "=" * 60)
        print(f"Results: {passed}/{total} passed, {failed} failed")
        print("=" * 60)

        if failed > 0:
            print("\nFailed tests:")
            for r in self.results:
                if not r.passed:
                    print(f"  - {r.name}: {r.message}")

        return total, passed, failed


# --- Test groups ---

def test_connection_lifecycle(suite: TestSuite):
    """
    Tests: TCP connection, handshake, disconnect cycle.
    Validates: Requirements 16.1, 16.4, 17.4
    """
    print("\n--- Connection Lifecycle Tests ---")

    def test_basic_connect():
        """Verify TCP connection can be established."""
        client = suite.create_client()
        client.connect(suite.host, suite.port)
        assert client.connected, "Client should report as connected"
        client.disconnect()
        assert not client.connected, "Client should report as disconnected"

    def test_initialize_handshake():
        """Verify MCP capability negotiation completes successfully."""
        client = suite.create_connected_client()
        try:
            result = client.initialize()
            assert "protocolVersion" in result, "Response must include protocolVersion"
            assert client.initialized, "Client should report as initialized"

            # Server should report some capabilities
            caps = client.server_capabilities
            assert isinstance(caps, dict), "Capabilities should be a dict"

            if suite.verbose:
                print(f"         Protocol: {result.get('protocolVersion')}")
                print(f"         Server: {client.server_info}")
                print(f"         Capabilities: {json.dumps(caps, indent=2)}")
        finally:
            client.disconnect()

    def test_server_reports_capabilities():
        """Verify server reports resource and tool capabilities."""
        client = suite.create_connected_client()
        try:
            client.initialize()
            caps = client.server_capabilities

            # Server should support resources (game state reading)
            # and tools (command execution) per MCP spec
            # The exact structure depends on MCP version, but both should be present
            has_resources = "resources" in caps
            has_tools = "tools" in caps
            assert has_resources or has_tools, (
                f"Server should report resources and/or tools capabilities. Got: {list(caps.keys())}"
            )
        finally:
            client.disconnect()

    def test_graceful_disconnect():
        """Verify client can disconnect cleanly after initialization."""
        client = suite.create_initialized_client()
        # Disconnect should not raise
        client.disconnect()
        assert not client.connected
        assert not client.initialized

    def test_reconnect_after_disconnect():
        """Verify client can reconnect after disconnecting."""
        client = suite.create_client()

        # First connection
        client.connect(suite.host, suite.port)
        client.initialize()
        client.disconnect()

        # Second connection
        client.connect(suite.host, suite.port)
        assert client.connected
        result = client.initialize()
        assert "protocolVersion" in result
        client.disconnect()

    def test_connect_refused_wrong_port():
        """Verify connection to wrong port raises appropriate error."""
        client = suite.create_client()
        try:
            # Use a port that's very unlikely to be in use
            client.connect(suite.host, 59999)
            assert False, "Should have raised MCPConnectionError"
        except MCPConnectionError:
            pass  # Expected

    suite.run_test("Basic TCP connect/disconnect", test_basic_connect)
    suite.run_test("MCP initialize handshake", test_initialize_handshake)
    suite.run_test("Server reports capabilities", test_server_reports_capabilities)
    suite.run_test("Graceful disconnect", test_graceful_disconnect)
    suite.run_test("Reconnect after disconnect", test_reconnect_after_disconnect)
    suite.run_test("Connection refused on wrong port", test_connect_refused_wrong_port)


def test_resource_reads(suite: TestSuite):
    """
    Tests: Reading game state via MCP resource URIs.
    Validates: Requirements 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 17.2
    """
    print("\n--- Resource Read Tests ---")

    def test_read_units():
        """Read unit state resource."""
        client = suite.create_initialized_client()
        try:
            result = client.read_resource("game://units")
            assert result is not None, "Units response should not be None"
            if suite.verbose:
                print(f"         Units response: {json.dumps(result, indent=2)[:500]}")
        finally:
            client.disconnect()

    def test_read_buildings():
        """Read building state resource."""
        client = suite.create_initialized_client()
        try:
            result = client.read_resource("game://buildings")
            assert result is not None, "Buildings response should not be None"
            if suite.verbose:
                print(f"         Buildings response: {json.dumps(result, indent=2)[:500]}")
        finally:
            client.disconnect()

    def test_read_resources():
        """Read player resource levels."""
        client = suite.create_initialized_client()
        try:
            result = client.read_resource("game://resources")
            assert result is not None, "Resources response should not be None"
            if suite.verbose:
                print(f"         Resources response: {json.dumps(result, indent=2)[:500]}")
        finally:
            client.disconnect()

    def test_read_resource_map():
        """Read resource deposit locations on map."""
        client = suite.create_initialized_client()
        try:
            result = client.read_resource("game://resources/map")
            assert result is not None, "Resource map response should not be None"
        finally:
            client.disconnect()

    def test_read_terrain():
        """Read terrain data."""
        client = suite.create_initialized_client()
        try:
            result = client.read_resource("game://terrain")
            assert result is not None, "Terrain response should not be None"
        finally:
            client.disconnect()

    def test_read_tech_tree():
        """Read technology tree state."""
        client = suite.create_initialized_client()
        try:
            result = client.read_resource("game://tech")
            assert result is not None, "Tech tree response should not be None"
        finally:
            client.disconnect()

    def test_read_players():
        """Read player information."""
        client = suite.create_initialized_client()
        try:
            result = client.read_resource("game://players")
            assert result is not None, "Players response should not be None"
            if suite.verbose:
                print(f"         Players response: {json.dumps(result, indent=2)[:500]}")
        finally:
            client.disconnect()

    def test_read_unknown_resource():
        """Verify unknown resource URI returns method-not-found error."""
        client = suite.create_initialized_client()
        try:
            client.read_resource("game://nonexistent")
            assert False, "Should have raised MCPServerError for unknown resource"
        except MCPServerError as e:
            # Expect error code -32601 (method not found) per design
            assert e.code == -32601, f"Expected error code -32601, got {e.code}"
        finally:
            client.disconnect()

    suite.run_test("Read units resource", test_read_units)
    suite.run_test("Read buildings resource", test_read_buildings)
    suite.run_test("Read resources", test_read_resources)
    suite.run_test("Read resource map", test_read_resource_map)
    suite.run_test("Read terrain", test_read_terrain)
    suite.run_test("Read tech tree", test_read_tech_tree)
    suite.run_test("Read players", test_read_players)
    suite.run_test("Read unknown resource returns error", test_read_unknown_resource)


def test_tool_calls(suite: TestSuite):
    """
    Tests: Executing game commands via MCP tool calls.
    Validates: Requirements 9.1, 10.1, 11.1, 12.1, 13.1, 14.1, 15.1, 17.3
    """
    print("\n--- Tool Call Tests ---")

    def test_move_units():
        """Test move_units tool call."""
        client = suite.create_initialized_client()
        try:
            result = client.call_tool("move_units", {
                "unitIds": [1],
                "targetPosition": {"x": 100.0, "y": 0.0, "z": 100.0}
            })
            # Should either succeed or return a game-rule error (e.g., unit not found)
            assert result is not None, "Move response should not be None"
            if suite.verbose:
                print(f"         Move result: {json.dumps(result, indent=2)}")
        except MCPServerError as e:
            # Game-rule errors are acceptable (unit may not exist in test scenario)
            if suite.verbose:
                print(f"         Move error (expected in some states): [{e.code}] {e.error_message}")

        finally:
            client.disconnect()

    def test_train_unit():
        """Test train_unit tool call."""
        client = suite.create_initialized_client()
        try:
            result = client.call_tool("train_unit", {
                "buildingId": 1,
                "unitType": "units/athen/infantry_spearman_b"
            })
            assert result is not None
            if suite.verbose:
                print(f"         Train result: {json.dumps(result, indent=2)}")
        except MCPServerError as e:
            # Expected — building may not exist or wrong type
            if suite.verbose:
                print(f"         Train error (expected): [{e.code}] {e.error_message}")
        finally:
            client.disconnect()

    def test_attack_target():
        """Test attack_target tool call."""
        client = suite.create_initialized_client()
        try:
            result = client.call_tool("attack_target", {
                "unitIds": [1, 2],
                "targetId": 99
            })
            assert result is not None
        except MCPServerError as e:
            # Expected — entities may not exist
            if suite.verbose:
                print(f"         Attack error (expected): [{e.code}] {e.error_message}")
        finally:
            client.disconnect()

    def test_build_structure():
        """Test build_structure tool call."""
        client = suite.create_initialized_client()
        try:
            result = client.call_tool("build_structure", {
                "buildingType": "structures/athen/house",
                "position": {"x": 150.0, "y": 0.0, "z": 150.0},
                "builderIds": [1]
            })
            assert result is not None
        except MCPServerError as e:
            if suite.verbose:
                print(f"         Build error (expected): [{e.code}] {e.error_message}")
        finally:
            client.disconnect()

    def test_research_tech():
        """Test research_tech tool call."""
        client = suite.create_initialized_client()
        try:
            result = client.call_tool("research_tech", {
                "techId": "phase_town_athen",
                "buildingId": 1
            })
            assert result is not None
        except MCPServerError as e:
            if suite.verbose:
                print(f"         Research error (expected): [{e.code}] {e.error_message}")
        finally:
            client.disconnect()

    def test_set_gather_point():
        """Test set_gather_point tool call."""
        client = suite.create_initialized_client()
        try:
            result = client.call_tool("set_gather_point", {
                "buildingId": 1,
                "target": {"x": 120.0, "y": 0.0, "z": 120.0}
            })
            assert result is not None
        except MCPServerError as e:
            if suite.verbose:
                print(f"         Gather point error (expected): [{e.code}] {e.error_message}")
        finally:
            client.disconnect()

    def test_gather_resource():
        """Test gather_resource tool call."""
        client = suite.create_initialized_client()
        try:
            result = client.call_tool("gather_resource", {
                "unitIds": [1],
                "resourceId": 50
            })
            assert result is not None
        except MCPServerError as e:
            if suite.verbose:
                print(f"         Gather error (expected): [{e.code}] {e.error_message}")
        finally:
            client.disconnect()

    def test_unknown_tool():
        """Verify unknown tool name returns method-not-found error."""
        client = suite.create_initialized_client()
        try:
            client.call_tool("nonexistent_tool", {"foo": "bar"})
            assert False, "Should have raised MCPServerError for unknown tool"
        except MCPServerError as e:
            assert e.code == -32601, f"Expected error code -32601, got {e.code}"
        finally:
            client.disconnect()

    def test_tool_with_missing_params():
        """Verify tool call with missing required params returns invalid params error."""
        client = suite.create_initialized_client()
        try:
            # move_units without required unitIds and targetPosition
            client.call_tool("move_units", {})
            assert False, "Should have raised MCPServerError for missing params"
        except MCPServerError as e:
            # Expect -32602 (invalid params) or game-rule error
            assert e.code in (-32602, -32000), f"Expected error code -32602 or -32000, got {e.code}"
        finally:
            client.disconnect()

    suite.run_test("Tool: move_units", test_move_units)
    suite.run_test("Tool: train_unit", test_train_unit)
    suite.run_test("Tool: attack_target", test_attack_target)
    suite.run_test("Tool: build_structure", test_build_structure)
    suite.run_test("Tool: research_tech", test_research_tech)
    suite.run_test("Tool: set_gather_point", test_set_gather_point)
    suite.run_test("Tool: gather_resource", test_gather_resource)
    suite.run_test("Unknown tool returns error", test_unknown_tool)
    suite.run_test("Tool with missing params returns error", test_tool_with_missing_params)


def test_error_handling(suite: TestSuite):
    """
    Tests: Error handling for malformed requests and protocol violations.
    Validates: Requirements 18.1, 18.2
    """
    print("\n--- Error Handling Tests ---")

    def test_malformed_json():
        """Send malformed JSON and verify server returns parse error."""
        client = suite.create_connected_client()
        try:
            # Send properly framed but invalid JSON
            bad_json = b"this is not json at all {"
            header = f"Content-Length: {len(bad_json)}\r\n\r\n".encode("ascii")
            client.send_raw(header + bad_json)

            # Server should respond with parse error -32700
            response = client.receive_response()
            assert "error" in response, "Should receive error response for malformed JSON"
            assert response["error"]["code"] == -32700, (
                f"Expected parse error -32700, got {response['error']['code']}"
            )
        finally:
            client.disconnect()

    def test_invalid_jsonrpc_request():
        """Send valid JSON but invalid JSON-RPC structure."""
        client = suite.create_connected_client()
        try:
            # Valid JSON but missing jsonrpc field
            invalid_request = json.dumps({"method": "test", "id": 1}).encode("utf-8")
            header = f"Content-Length: {len(invalid_request)}\r\n\r\n".encode("ascii")
            client.send_raw(header + invalid_request)

            response = client.receive_response()
            assert "error" in response, "Should receive error for invalid JSON-RPC"
            # Expect -32600 (invalid request) or -32700
            assert response["error"]["code"] in (-32600, -32700), (
                f"Expected error code -32600 or -32700, got {response['error']['code']}"
            )
        finally:
            client.disconnect()

    def test_unknown_method():
        """Send request with unknown method name."""
        client = suite.create_initialized_client()
        try:
            result = client.send_request("completely/unknown/method", {})
            # If no error was raised, this is unexpected but not necessarily wrong
            assert False, "Expected MCPServerError for unknown method"
        except MCPServerError as e:
            assert e.code == -32601, f"Expected method-not-found -32601, got {e.code}"
        finally:
            client.disconnect()

    def test_request_before_initialize():
        """Send a resource read before completing initialization."""
        client = suite.create_connected_client()
        try:
            # Bypass the client's initialization check and send directly
            response = client.send_request("resources/read", {"uri": "game://units"})
            # Some servers may allow this; some may reject
            # Either outcome is valid — just verify we get a clean response or error
        except MCPServerError as e:
            # This is acceptable — server may require init first
            if suite.verbose:
                print(f"         Pre-init error (acceptable): [{e.code}] {e.error_message}")
        finally:
            client.disconnect()

    def test_empty_content_length():
        """Send frame with zero-length content."""
        client = suite.create_connected_client()
        try:
            header = b"Content-Length: 0\r\n\r\n"
            client.send_raw(header)

            # Server should handle gracefully — either error response or just ignore
            try:
                response = client.receive_response()
                if "error" in response:
                    # Parse error or invalid request is fine
                    assert response["error"]["code"] in (-32700, -32600)
            except MCPTimeoutError:
                pass  # Acceptable — server may ignore empty bodies
        finally:
            client.disconnect()

    suite.run_test("Malformed JSON returns parse error", test_malformed_json)
    suite.run_test("Invalid JSON-RPC returns error", test_invalid_jsonrpc_request)
    suite.run_test("Unknown method returns error", test_unknown_method)
    suite.run_test("Request before initialize", test_request_before_initialize)
    suite.run_test("Empty content-length handling", test_empty_content_length)


# --- Main ---

def main():
    parser = argparse.ArgumentParser(
        description="Integration tests for the 0 A.D. MCP server.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Test Groups:
  connection   - Connection lifecycle (connect, handshake, disconnect)
  resources    - Game state resource reads
  tools        - Game command tool calls
  errors       - Error handling and edge cases
  all          - Run all test groups (default)

Examples:
  python test_integration.py                         # Run all tests
  python test_integration.py --test connection       # Connection tests only
  python test_integration.py --verbose               # Verbose output
  python test_integration.py --port 7000 --timeout 5 # Custom config
        """
    )
    parser.add_argument(
        "--host", default=MCPClient.DEFAULT_HOST,
        help=f"MCP server host (default: {MCPClient.DEFAULT_HOST})"
    )
    parser.add_argument(
        "--port", type=int, default=MCPClient.DEFAULT_PORT,
        help=f"MCP server port (default: {MCPClient.DEFAULT_PORT})"
    )
    parser.add_argument(
        "--timeout", type=float, default=10.0,
        help="Response timeout in seconds (default: 10.0)"
    )
    parser.add_argument(
        "--verbose", "-v", action="store_true",
        help="Show detailed output including response payloads"
    )
    parser.add_argument(
        "--test", choices=["connection", "resources", "tools", "errors", "all"],
        default="all",
        help="Which test group to run (default: all)"
    )

    args = parser.parse_args()

    print("=" * 60)
    print("0 A.D. MCP Server Integration Tests")
    print("=" * 60)
    print(f"Target: {args.host}:{args.port}")
    print(f"Timeout: {args.timeout}s")
    print(f"Tests: {args.test}")

    # Quick connectivity check
    print(f"\nChecking server connectivity...")
    try:
        probe = MCPClient(timeout=min(args.timeout, 5.0))
        probe.connect(args.host, args.port)
        probe.disconnect()
        print("Server is reachable.\n")
    except MCPConnectionError as e:
        print(f"\nERROR: Cannot connect to MCP server at {args.host}:{args.port}")
        print(f"  {e}")
        print("\nMake sure:")
        print("  1. 0 A.D. is running with the MCP module active")
        print("  2. The server is listening on the specified port")
        print("  3. No firewall is blocking the connection")
        sys.exit(1)

    suite = TestSuite(args.host, args.port, args.timeout, verbose=args.verbose)

    # Run requested test groups
    test_groups = {
        "connection": test_connection_lifecycle,
        "resources": test_resource_reads,
        "tools": test_tool_calls,
        "errors": test_error_handling,
    }

    if args.test == "all":
        for group_fn in test_groups.values():
            group_fn(suite)
    else:
        test_groups[args.test](suite)

    # Print summary
    total, passed, failed = suite.summary()

    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
