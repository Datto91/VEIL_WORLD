#!/usr/bin/env node
/**
 * 0 A.D. MCP Bridge — stdio-to-TCP transport adapter
 *
 * Kiro speaks MCP over stdio (stdin/stdout).
 * The 0 A.D. game engine speaks MCP over TCP (port 6374).
 *
 * This bridge:
 *   1. Kiro launches this script as a child process
 *   2. This script connects to 0 A.D.'s TCP MCP server
 *   3. Messages from Kiro (stdin) are forwarded to TCP
 *   4. Messages from TCP are forwarded to Kiro (stdout)
 *
 * Both sides use Content-Length header framing (LSP/MCP style).
 */

const net = require('net');

const HOST = process.env.MCP_0AD_HOST || '127.0.0.1';
const PORT = parseInt(process.env.MCP_0AD_PORT || '6374', 10);

let tcpSocket = null;
let stdinBuffer = '';
let tcpBuffer = '';

// --- Content-Length framing helpers ---

function extractMessages(buffer) {
  const messages = [];
  while (true) {
    const delimIdx = buffer.indexOf('\r\n\r\n');
    if (delimIdx === -1) break;

    const header = buffer.substring(0, delimIdx);
    const match = header.match(/Content-Length:\s*(\d+)/i);
    if (!match) {
      // Skip malformed header
      buffer = buffer.substring(delimIdx + 4);
      continue;
    }

    const contentLength = parseInt(match[1], 10);
    const bodyStart = delimIdx + 4;
    const totalLength = bodyStart + contentLength;

    if (buffer.length < totalLength) break; // Incomplete body

    const body = buffer.substring(bodyStart, totalLength);
    messages.push(body);
    buffer = buffer.substring(totalLength);
  }
  return { messages, remaining: buffer };
}

function frameMessage(body) {
  return `Content-Length: ${Buffer.byteLength(body, 'utf-8')}\r\n\r\n${body}`;
}

// --- TCP connection to 0 A.D. ---

function connectToGame() {
  tcpSocket = net.createConnection({ host: HOST, port: PORT }, () => {
    process.stderr.write(`[0ad-mcp-bridge] Connected to 0 A.D. at ${HOST}:${PORT}\n`);
  });

  tcpSocket.setEncoding('utf-8');

  tcpSocket.on('data', (data) => {
    // Forward TCP data → stdout (to Kiro)
    tcpBuffer += data;
    const { messages, remaining } = extractMessages(tcpBuffer);
    tcpBuffer = remaining;

    for (const msg of messages) {
      const framed = frameMessage(msg);
      process.stdout.write(framed);
    }
  });

  tcpSocket.on('error', (err) => {
    process.stderr.write(`[0ad-mcp-bridge] TCP error: ${err.message}\n`);
    process.exit(1);
  });

  tcpSocket.on('close', () => {
    process.stderr.write(`[0ad-mcp-bridge] TCP connection closed\n`);
    process.exit(0);
  });
}

// --- stdin from Kiro ---

process.stdin.setEncoding('utf-8');

process.stdin.on('data', (data) => {
  // Forward stdin data → TCP (to 0 A.D.)
  stdinBuffer += data;
  const { messages, remaining } = extractMessages(stdinBuffer);
  stdinBuffer = remaining;

  for (const msg of messages) {
    if (tcpSocket && !tcpSocket.destroyed) {
      const framed = frameMessage(msg);
      tcpSocket.write(framed);
    }
  }
});

process.stdin.on('end', () => {
  process.stderr.write(`[0ad-mcp-bridge] stdin closed, shutting down\n`);
  if (tcpSocket) tcpSocket.destroy();
  process.exit(0);
});

// --- Start ---

connectToGame();
