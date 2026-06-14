"""
MCP Client for 0 A.D. Integration Testing

A standalone Python client that connects to the in-process MCP server
embedded within the 0 A.D. game engine. Uses Content-Length header framing
(LSP/MCP style) over TCP with JSON-RPC 2.0 protocol.

Usage:
    from mcp_client import MCPClient

    client = MCPClient()
    client.connect("127.0.0.1", 6374)
    client.initialize()
    units = client.read_resource("game://units")
    result = client.call_tool("move_units", {"unitIds": [1,2], "targetPosition": {"x":10,"y":0,"z":20}})
    client.disconnect()

No external dependencies required — uses only Python standard library.
"""

import socket
import json
import time
import sys
from typing import Any, Optional


class MCPClientError(Exception):
    """Base exception for MCP client errors."""
    pass


class MCPConnectionError(MCPClientError):
    """Raised when connection fails or is lost."""
    pass


class MCPTimeoutError(MCPClientError):
    """Raised when a response is not received within the timeout period."""
    pass


class MCPProtocolError(MCPClientError):
    """Raised when the server response violates the expected protocol."""
    pass


class MCPServerError(MCPClientError):
    """Raised when the server returns a JSON-RPC error response."""

    def __init__(self, code: int, message: str, data: Any = None):
        self.code = code
        self.error_message = message
        self.data = data
        super().__init__(f"MCP Server Error [{code}]: {message}")


class MCPClient:
    """
    MCP (Model Context Protocol) client for connecting to the 0 A.D. game engine.

    Implements:
    - TCP connection with Content-Length header framing
    - JSON-RPC 2.0 request/response protocol
    - MCP initialize/initialized handshake
    - Resource reading (game state queries)
    - Tool calls (game command execution)
    - Graceful disconnect
    """

    DEFAULT_HOST = "127.0.0.1"
    DEFAULT_PORT = 6374
    DEFAULT_TIMEOUT = 10.0  # seconds
    HEADER_ENCODING = "ascii"
    CONTENT_ENCODING = "utf-8"
    HEADER_SEPARATOR = b"\r\n\r\n"

    def __init__(self, timeout: float = DEFAULT_TIMEOUT):
        """
        Initialize the MCP client.

        Args:
            timeout: Maximum seconds to wait for a response before raising MCPTimeoutError.
        """
        self._socket: Optional[socket.socket] = None
        self._timeout = timeout
        self._request_id = 0
        self._connected = False
        self._initialized = False
        self._recv_buffer = b""
        self._server_capabilities: dict = {}
        self._server_info: dict = {}

    @property
    def connected(self) -> bool:
        """Whether the client is currently connected to the server."""
        return self._connected

    @property
    def initialized(self) -> bool:
        """Whether the MCP handshake has been completed."""
        return self._initialized

    @property
    def server_capabilities(self) -> dict:
        """Server capabilities returned during initialization."""
        return self._server_capabilities

    @property
    def server_info(self) -> dict:
        """Server info returned during initialization."""
        return self._server_info

    def _next_id(self) -> int:
        """Generate the next unique request ID."""
        self._request_id += 1
        return self._request_id

    def connect(self, host: str = DEFAULT_HOST, port: int = DEFAULT_PORT) -> None:
        """
        Establish a TCP connection to the MCP server.

        Args:
            host: Server hostname or IP address.
            port: Server port number.

        Raises:
            MCPConnectionError: If connection cannot be established.
        """
        if self._connected:
            raise MCPConnectionError("Already connected. Call disconnect() first.")

        try:
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._socket.settimeout(self._timeout)
            self._socket.connect((host, port))
            self._connected = True
            self._recv_buffer = b""
        except socket.timeout:
            self._cleanup_socket()
            raise MCPConnectionError(
                f"Connection to {host}:{port} timed out after {self._timeout}s"
            )
        except OSError as e:
            self._cleanup_socket()
            raise MCPConnectionError(
                f"Failed to connect to {host}:{port}: {e}"
            )

    def disconnect(self) -> None:
        """
        Gracefully disconnect from the MCP server.

        Sends a shutdown notification if initialized, then closes the socket.
        """
        if not self._connected:
            return

        try:
            # Send shutdown notification (best effort)
            if self._initialized:
                self._send_notification("notifications/cancelled", {"reason": "client_disconnect"})
        except (OSError, MCPClientError):
            pass  # Best effort — don't fail on disconnect
        finally:
            self._cleanup_socket()
            self._connected = False
            self._initialized = False
            self._recv_buffer = b""

    def initialize(self, client_name: str = "0ad-test-harness", client_version: str = "1.0.0") -> dict:
        """
        Perform the MCP capability negotiation handshake.

        Sends the 'initialize' request and then the 'initialized' notification.

        Args:
            client_name: Name to identify this client to the server.
            client_version: Version string for this client.

        Returns:
            The server's capabilities response.

        Raises:
            MCPConnectionError: If not connected.
            MCPProtocolError: If handshake fails.
            MCPTimeoutError: If server doesn't respond in time.
        """
        if not self._connected:
            raise MCPConnectionError("Not connected. Call connect() first.")

        if self._initialized:
            raise MCPProtocolError("Already initialized. Disconnect and reconnect to re-initialize.")

        # Send initialize request
        params = {
            "protocolVersion": "2024-11-05",
            "capabilities": {
                "roots": {"listChanged": False}
            },
            "clientInfo": {
                "name": client_name,
                "version": client_version
            }
        }

        response = self.send_request("initialize", params)

        # Validate response structure
        if "protocolVersion" not in response:
            raise MCPProtocolError(
                "Server initialize response missing 'protocolVersion'"
            )

        self._server_capabilities = response.get("capabilities", {})
        self._server_info = response.get("serverInfo", {})

        # Send initialized notification (no response expected)
        self._send_notification("notifications/initialized", {})

        self._initialized = True
        return response

    def read_resource(self, uri: str) -> dict:
        """
        Read a game state resource from the MCP server.

        Args:
            uri: The resource URI to read (e.g., "game://units", "game://buildings").

        Returns:
            The resource content from the server response.

        Raises:
            MCPConnectionError: If not connected.
            MCPProtocolError: If not initialized.
            MCPServerError: If the server returns an error.
            MCPTimeoutError: If server doesn't respond in time.
        """
        self._ensure_ready()

        params = {
            "uri": uri
        }

        return self.send_request("resources/read", params)

    def call_tool(self, name: str, arguments: Optional[dict] = None) -> dict:
        """
        Call a tool (game command) on the MCP server.

        Args:
            name: The tool name (e.g., "move_units", "train_unit").
            arguments: Tool-specific parameters as a dictionary.

        Returns:
            The tool execution result from the server.

        Raises:
            MCPConnectionError: If not connected.
            MCPProtocolError: If not initialized.
            MCPServerError: If the server returns an error (e.g., invalid command).
            MCPTimeoutError: If server doesn't respond in time.
        """
        self._ensure_ready()

        params = {
            "name": name,
            "arguments": arguments or {}
        }

        return self.send_request("tools/call", params)

    def send_request(self, method: str, params: Optional[dict] = None, request_id: Optional[int] = None) -> dict:
        """
        Send a JSON-RPC 2.0 request and wait for the response.

        Args:
            method: The JSON-RPC method name.
            params: Optional parameters for the method.
            request_id: Optional explicit request ID. Auto-generates if not specified.

        Returns:
            The 'result' field from the JSON-RPC response.

        Raises:
            MCPConnectionError: If not connected or connection lost.
            MCPServerError: If the server returns an error response.
            MCPTimeoutError: If no response is received within timeout.
        """
        if not self._connected:
            raise MCPConnectionError("Not connected.")

        if request_id is None:
            request_id = self._next_id()

        message = {
            "jsonrpc": "2.0",
            "id": request_id,
            "method": method,
        }
        if params is not None:
            message["params"] = params

        self._send_frame(message)
        response = self.receive_response()

        # Check for error response
        if "error" in response:
            err = response["error"]
            raise MCPServerError(
                code=err.get("code", -1),
                message=err.get("message", "Unknown error"),
                data=err.get("data")
            )

        return response.get("result", {})

    def receive_response(self) -> dict:
        """
        Read and parse a single framed JSON-RPC response from the server.

        Returns:
            The parsed JSON-RPC response as a dictionary.

        Raises:
            MCPConnectionError: If connection is lost during read.
            MCPTimeoutError: If no complete message received within timeout.
            MCPProtocolError: If the framing or JSON is invalid.
        """
        if not self._connected:
            raise MCPConnectionError("Not connected.")

        deadline = time.time() + self._timeout

        while True:
            # Try to parse a complete message from buffer
            message = self._try_parse_message()
            if message is not None:
                return message

            # Check timeout
            remaining = deadline - time.time()
            if remaining <= 0:
                raise MCPTimeoutError(
                    f"No response received within {self._timeout}s"
                )

            # Read more data from socket
            try:
                self._socket.settimeout(min(remaining, 1.0))
                data = self._socket.recv(4096)
                if not data:
                    self._connected = False
                    raise MCPConnectionError("Connection closed by server.")
                self._recv_buffer += data
            except socket.timeout:
                continue  # Will check deadline on next iteration
            except OSError as e:
                self._connected = False
                raise MCPConnectionError(f"Socket error during receive: {e}")

    def send_raw(self, raw_bytes: bytes) -> None:
        """
        Send raw bytes to the server (for testing malformed input handling).

        Args:
            raw_bytes: The raw bytes to send.

        Raises:
            MCPConnectionError: If not connected or send fails.
        """
        if not self._connected:
            raise MCPConnectionError("Not connected.")

        try:
            self._socket.sendall(raw_bytes)
        except OSError as e:
            self._connected = False
            raise MCPConnectionError(f"Send failed: {e}")

    # --- Private methods ---

    def _ensure_ready(self) -> None:
        """Ensure the client is connected and initialized."""
        if not self._connected:
            raise MCPConnectionError("Not connected. Call connect() first.")
        if not self._initialized:
            raise MCPProtocolError("Not initialized. Call initialize() first.")

    def _send_frame(self, message: dict) -> None:
        """
        Serialize and send a JSON-RPC message with Content-Length framing.

        Format:
            Content-Length: <length>\r\n\r\n<json_body>
        """
        body = json.dumps(message).encode(self.CONTENT_ENCODING)
        header = f"Content-Length: {len(body)}\r\n\r\n".encode(self.HEADER_ENCODING)
        frame = header + body

        try:
            self._socket.sendall(frame)
        except OSError as e:
            self._connected = False
            raise MCPConnectionError(f"Send failed: {e}")

    def _send_notification(self, method: str, params: Optional[dict] = None) -> None:
        """
        Send a JSON-RPC notification (no response expected, no 'id' field).
        """
        message = {
            "jsonrpc": "2.0",
            "method": method,
        }
        if params is not None:
            message["params"] = params

        self._send_frame(message)

    def _try_parse_message(self) -> Optional[dict]:
        """
        Attempt to parse a complete Content-Length framed message from the buffer.

        Returns:
            Parsed JSON dict if a complete message is available, None otherwise.
        """
        # Look for the header separator
        sep_idx = self._recv_buffer.find(self.HEADER_SEPARATOR)
        if sep_idx == -1:
            return None

        # Parse Content-Length from headers
        header_bytes = self._recv_buffer[:sep_idx]
        content_length = self._parse_content_length(header_bytes)
        if content_length is None:
            raise MCPProtocolError(
                f"Invalid frame header (missing Content-Length): {header_bytes!r}"
            )

        # Check if we have the full body
        body_start = sep_idx + len(self.HEADER_SEPARATOR)
        body_end = body_start + content_length

        if len(self._recv_buffer) < body_end:
            return None  # Incomplete message, need more data

        # Extract and parse the body
        body_bytes = self._recv_buffer[body_start:body_end]
        self._recv_buffer = self._recv_buffer[body_end:]

        try:
            return json.loads(body_bytes.decode(self.CONTENT_ENCODING))
        except (json.JSONDecodeError, UnicodeDecodeError) as e:
            raise MCPProtocolError(f"Invalid JSON in response body: {e}")

    def _parse_content_length(self, header_bytes: bytes) -> Optional[int]:
        """
        Parse the Content-Length value from header bytes.

        Handles case-insensitive header name matching.
        """
        try:
            header_str = header_bytes.decode(self.HEADER_ENCODING)
        except UnicodeDecodeError:
            return None

        for line in header_str.split("\r\n"):
            parts = line.split(":", 1)
            if len(parts) == 2:
                name = parts[0].strip().lower()
                if name == "content-length":
                    try:
                        return int(parts[1].strip())
                    except ValueError:
                        return None
        return None

    def _cleanup_socket(self) -> None:
        """Close and clean up the socket."""
        if self._socket:
            try:
                self._socket.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            try:
                self._socket.close()
            except OSError:
                pass
            self._socket = None


def main():
    """Simple interactive test — connect and print server capabilities."""
    import argparse

    parser = argparse.ArgumentParser(description="MCP Client for 0 A.D.")
    parser.add_argument("--host", default=MCPClient.DEFAULT_HOST, help="Server host")
    parser.add_argument("--port", type=int, default=MCPClient.DEFAULT_PORT, help="Server port")
    parser.add_argument("--timeout", type=float, default=10.0, help="Response timeout (seconds)")
    args = parser.parse_args()

    client = MCPClient(timeout=args.timeout)

    print(f"Connecting to {args.host}:{args.port}...")
    try:
        client.connect(args.host, args.port)
        print("Connected.")

        print("Performing MCP handshake...")
        result = client.initialize()
        print(f"Server info: {client.server_info}")
        print(f"Server capabilities: {json.dumps(client.server_capabilities, indent=2)}")
        print(f"Protocol version: {result.get('protocolVersion')}")

        print("\nConnection lifecycle test passed. Disconnecting...")
        client.disconnect()
        print("Disconnected cleanly.")

    except MCPClientError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
