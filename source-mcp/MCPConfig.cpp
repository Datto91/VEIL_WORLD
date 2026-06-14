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

#include "mcp/MCPConfig.h"

#include "ps/CLogger.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

namespace MCP
{

MCPConfig MCPConfig::defaults()
{
	// Return a default-constructed MCPConfig.
	// All fields have their defaults defined in the header:
	// port=6374, maxSessions=4, logVerbosity=1
	return MCPConfig{};
}

MCPConfig MCPConfig::loadFromFile(const std::string& path)
{
	MCPConfig config = defaults();
	config.configFilePath = path;

	// Try to open the configuration file.
	std::ifstream file(path);
	if (!file.is_open())
	{
		LOGMESSAGE("MCP: Config file not found at '%s', using defaults.", path.c_str());
		return config;
	}

	// Parse the JSON content.
	nlohmann::json doc;
	try
	{
		doc = nlohmann::json::parse(file);
	}
	catch (const nlohmann::json::parse_error& e)
	{
		LOGWARNING("MCP: Failed to parse config file '%s': %s. Using defaults.", path.c_str(), e.what());
		return config;
	}

	// The root must be a JSON object for us to extract keys.
	if (!doc.is_object())
	{
		LOGWARNING("MCP: Config file '%s' root is not a JSON object. Using defaults.", path.c_str());
		return config;
	}

	// Read "port" if present and valid.
	if (doc.contains("port"))
	{
		try
		{
			if (doc["port"].is_number_unsigned() || doc["port"].is_number_integer())
			{
				auto portVal = doc["port"].get<int>();
				if (portVal > 0 && portVal <= 65535)
					config.port = static_cast<uint16_t>(portVal);
				else
					LOGWARNING("MCP: Config 'port' value %d out of valid range (1-65535), using default.", portVal);
			}
			else
			{
				LOGWARNING("MCP: Config 'port' is not a valid integer, using default.");
			}
		}
		catch (const nlohmann::json::type_error&)
		{
			LOGWARNING("MCP: Config 'port' has invalid type, using default.");
		}
	}

	// Read "maxSessions" if present and valid.
	if (doc.contains("maxSessions"))
	{
		try
		{
			if (doc["maxSessions"].is_number_unsigned() || doc["maxSessions"].is_number_integer())
			{
				auto sessVal = doc["maxSessions"].get<int>();
				if (sessVal > 0)
					config.maxSessions = static_cast<size_t>(sessVal);
				else
					LOGWARNING("MCP: Config 'maxSessions' must be positive, using default.");
			}
			else
			{
				LOGWARNING("MCP: Config 'maxSessions' is not a valid integer, using default.");
			}
		}
		catch (const nlohmann::json::type_error&)
		{
			LOGWARNING("MCP: Config 'maxSessions' has invalid type, using default.");
		}
	}

	// Read "logVerbosity" if present and valid.
	if (doc.contains("logVerbosity"))
	{
		try
		{
			if (doc["logVerbosity"].is_number_unsigned() || doc["logVerbosity"].is_number_integer())
			{
				auto verbVal = doc["logVerbosity"].get<int>();
				if (verbVal >= 0 && verbVal <= 2)
					config.logVerbosity = verbVal;
				else
					LOGWARNING("MCP: Config 'logVerbosity' value %d out of range (0-2), using default.", verbVal);
			}
			else
			{
				LOGWARNING("MCP: Config 'logVerbosity' is not a valid integer, using default.");
			}
		}
		catch (const nlohmann::json::type_error&)
		{
			LOGWARNING("MCP: Config 'logVerbosity' has invalid type, using default.");
		}
	}

	LOGMESSAGE("MCP: Loaded config from '%s' (port=%d, maxSessions=%zu, logVerbosity=%d).",
		path.c_str(), config.port, config.maxSessions, config.logVerbosity);

	return config;
}

} // namespace MCP
