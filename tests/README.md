# 0 A.D. MCP Server Integration Tests

External integration test harness for the MCP (Model Context Protocol) server embedded in the 0 A.D. game engine.

## Overview

These tests validate the MCP server's network protocol, session management, game state reading, and command execution from an external client perspective. The test harness is written in Python (standard library only — no `pip install` required).

## Files

| File | Purpose |
|------|---------|
| `mcp_client.py` | Reusable MCP client class with Content-Length framing, JSON-RPC 2.0, and full handshake support |
| `test_integration.py` | Integration test suite covering connection lifecycle, resource reads, tool calls, and error handling |

## Prerequisites

1. **Python 3.8+** installed and available on PATH
2. **0 A.D. running** with the MCP server module active
3. **Game session started** (at minimum, past the main menu into a game — resource/tool queries need a live simulation)
4. **MCP server listening** on port 6374 (default) or your configured port

## Running the Tests

### Full test suite

```bash
python tests/test_integration.py
```

### With verbose output (shows response payloads)

```bash
python tests/test_integration.py --verbose
```

### Run a specific test group

```bash
python tests/test_integration.py --test connection
python tests/test_integration.py --test resources
python tests/test_integration.py --test tools
python tests/test_integration.py --test errors
```

### Custom host/port

```bash
python tests/test_integration.py --host 127.0.0.1 --port 7000
```

### Adjust timeout (useful for slow machines)

```bash
python tests/test_integration.py --timeout 20
```

## Test Groups

### Connection Lifecycle (`--test connection`)
- Basic TCP connect/disconnect
- MCP initialize/initialized handshake
- Server capability reporting
- Graceful disconnect
- Reconnection after disconnect
- Connection refused on wrong port

### Resource Reads (`--test resources`)
- Read units (`game://units`)
- Read buildings (`game://buildings`)
- Read resources (`game://resources`)
- Read resource map (`game://resources/map`)
- Read terrain (`game://terrain`)
- Read tech tree (`game://tech`)
- Read player info (`game://players`)
- Unknown resource URI error handling

### Tool Calls (`--test tools`)
- `move_units` — move entities to position
- `train_unit` — queue unit production
- `attack_target` — attack an entity
- `build_structure` — place building
- `research_tech` — begin research
- `set_gather_point` — set rally point
- `gather_resource` — gather from deposit
- Unknown tool error handling
- Missing parameters error handling

### Error Handling (`--test errors`)
- Malformed JSON handling (parse error -32700)
- Invalid JSON-RPC structure
- Unknown method name (method not found -32601)
- Request before initialization
- Empty content handling

## Using the Client Library

The `MCPClient` class can be imported and used directly for custom scripts or AI agent development:

```python
from mcp_client import MCPClient, MCPServerError

client = MCPClient(timeout=10.0)
client.connect("127.0.0.1", 6374)
client.initialize()

# Read game state
units = client.read_resource("game://units")
print(f"Visible units: {units}")

# Issue a command
try:
    result = client.call_tool("move_units", {
        "unitIds": [42, 43],
        "targetPosition": {"x": 200.0, "y": 0.0, "z": 150.0},
        "formation": "line"
    })
    print(f"Command accepted: {result}")
except MCPServerError as e:
    print(f"Command rejected: [{e.code}] {e.error_message}")

client.disconnect()
```

## Protocol Details

- **Transport**: TCP socket
- **Framing**: Content-Length header (LSP/MCP style)
  ```
  Content-Length: 42\r\n\r\n{"jsonrpc":"2.0",...}
  ```
- **Protocol**: JSON-RPC 2.0
- **Handshake**: `initialize` request → response → `notifications/initialized` notification
- **Default port**: 6374

## Troubleshooting

| Issue | Solution |
|-------|----------|
| "Cannot connect to MCP server" | Ensure 0 A.D. is running with MCP active; check port in `mcp_config.json` |
| Timeout on handshake | Increase `--timeout`; check firewall settings |
| Resource reads return empty | Make sure a game is in progress (not just main menu) |
| Tool calls all fail | Verify the AI agent is assigned to a valid player slot with units |
| Connection reset | Check if MCP server hit max sessions; restart the game |

## Requirements Coverage

These tests validate:
- **Req 16.1**: Session establishment and player slot assignment
- **Req 16.4**: Graceful session termination
- **Req 17.1**: MCP message format compliance
- **Req 17.2**: Game state exposed as MCP resources
- **Req 17.3**: Commands exposed as MCP tools
- **Req 17.4**: Capability negotiation handshake
- **Req 18.1**: Malformed request error handling
