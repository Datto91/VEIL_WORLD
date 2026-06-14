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

#ifndef INCLUDED_MCP_CONFIG
#define INCLUDED_MCP_CONFIG

#include <cstdint>
#include <cstddef>
#include <string>

namespace MCP
{

/**
 * Configuration for the MCP server module.
 *
 * Values are loaded from a JSON configuration file at startup.
 * Missing keys fall back to documented defaults.
 *
 * Configuration file format (mcp_config.json):
 * {
 *     "port": 6374,
 *     "maxSessions": 4,
 *     "logVerbosity": 1
 * }
 */
struct MCPConfig
{
	/** TCP port the MCP server listens on for AI agent connections. */
	uint16_t port = 6374;

	/** Maximum number of concurrent AI agent sessions allowed. */
	size_t maxSessions = 4;

	/** Logging verbosity level: 0=error, 1=info, 2=debug. */
	int logVerbosity = 1;

	/** Path to the configuration file that was loaded. */
	std::string configFilePath = "mcp_config.json";

	/**
	 * Load configuration from a JSON file.
	 * Missing keys in the file will retain their default values.
	 * If the file does not exist or cannot be parsed, returns defaults.
	 *
	 * @param path Filesystem path to the JSON configuration file.
	 * @return MCPConfig populated with file values and defaults for missing keys.
	 */
	static MCPConfig loadFromFile(const std::string& path);

	/**
	 * Return a configuration with all default values.
	 *
	 * @return MCPConfig with port=6374, maxSessions=4, logVerbosity=1.
	 */
	static MCPConfig defaults();
};

} // namespace MCP

#endif // INCLUDED_MCP_CONFIG
