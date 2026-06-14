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

#include "mcp/state/MCPStateReader.h"

namespace MCP
{

using json = nlohmann::json;

MCPStateReader::MCPStateReader(MCPGameStateSnapshot& snapshot)
	: m_snapshot(snapshot)
{
}

// ---------------------------------------------------------------------------
// Units query (task 6.2)
// ---------------------------------------------------------------------------

json MCPStateReader::queryUnits(PlayerId viewer) const
{
	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot)
		return json::object();

	json units = json::array();
	for (const auto& unit : snapshot->units)
	{
		if (unit.owner != viewer && !isEntityVisible(unit.id, viewer))
			continue;

		units.push_back({
			{"id", unit.id},
			{"position", {{"x", unit.position.x}, {"y", unit.position.y}, {"z", unit.position.z}}},
			{"health", unit.health},
			{"maxHealth", unit.maxHealth},
			{"owner", unit.owner},
			{"type", unit.templateName},
			{"currentAction", unit.currentAction}
		});
	}

	return {{"units", units}, {"tickNumber", snapshot->tickNumber}};
}

// ---------------------------------------------------------------------------
// Buildings query (task 6.2)
// ---------------------------------------------------------------------------

json MCPStateReader::queryBuildings(PlayerId viewer) const
{
	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot)
		return json::object();

	json buildings = json::array();
	for (const auto& bldg : snapshot->buildings)
	{
		if (bldg.owner != viewer && !isEntityVisible(bldg.id, viewer))
			continue;

		json queue = json::array();
		for (const auto& entry : bldg.productionQueue)
		{
			queue.push_back({
				{"unitType", entry.unitType},
				{"progress", entry.progress},
				{"timeRemaining", entry.timeRemaining}
			});
		}

		json garrison = json::array();
		for (EntityId gId : bldg.garrisonedUnits)
			garrison.push_back(gId);

		buildings.push_back({
			{"id", bldg.id},
			{"position", {{"x", bldg.position.x}, {"y", bldg.position.y}, {"z", bldg.position.z}}},
			{"health", bldg.health},
			{"maxHealth", bldg.maxHealth},
			{"owner", bldg.owner},
			{"type", bldg.templateName},
			{"constructionProgress", bldg.constructionProgress},
			{"productionQueue", queue},
			{"garrisonedUnits", garrison},
			{"supportsGarrison", bldg.supportsGarrison}
		});
	}

	return {{"buildings", buildings}, {"tickNumber", snapshot->tickNumber}};
}

// ---------------------------------------------------------------------------
// Resources query (task 6.3)
// ---------------------------------------------------------------------------

json MCPStateReader::queryResources(PlayerId viewer) const
{
	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot)
		return json::object();

	auto it = snapshot->playerResources.find(viewer);
	if (it == snapshot->playerResources.end())
		return json::object();

	const auto& r = it->second;
	return {
		{"food", r.food},
		{"wood", r.wood},
		{"stone", r.stone},
		{"metal", r.metal},
		{"gatheringRates", {
			{"food", r.foodRate},
			{"wood", r.woodRate},
			{"stone", r.stoneRate},
			{"metal", r.metalRate}
		}},
		{"tickNumber", snapshot->tickNumber}
	};
}

// ---------------------------------------------------------------------------
// Resource locations query (task 6.3)
// ---------------------------------------------------------------------------

json MCPStateReader::queryResourceLocations(PlayerId viewer) const
{
	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot)
		return json::object();

	json deposits = json::array();
	for (const auto& deposit : snapshot->resourceDeposits)
	{
		if (!isEntityVisible(deposit.id, viewer))
			continue;

		deposits.push_back({
			{"id", deposit.id},
			{"position", {{"x", deposit.position.x}, {"y", deposit.position.y}, {"z", deposit.position.z}}},
			{"resourceType", deposit.resourceType},
			{"remainingQuantity", deposit.remainingQuantity}
		});
	}

	return {{"deposits", deposits}, {"tickNumber", snapshot->tickNumber}};
}

// ---------------------------------------------------------------------------
// Terrain query (task 6.3)
// ---------------------------------------------------------------------------

json MCPStateReader::queryTerrain(PlayerId viewer, int resolution) const
{
	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot || snapshot->terrainGrid.empty())
		return json::object();

	// The terrain grid in the snapshot is captured at a fixed resolution.
	// If the requested resolution differs, a full implementation would resample.
	// For now, return the stored grid and report its actual resolution.

	// Look up per-player terrain visibility
	const std::vector<std::vector<FogOfWarState>>* playerTerrainVis = nullptr;
	auto visIt = snapshot->terrainVisibility.find(viewer);
	if (visIt != snapshot->terrainVisibility.end())
		playerTerrainVis = &visIt->second;

	json grid = json::array();
	for (size_t z = 0; z < snapshot->terrainGrid.size(); ++z)
	{
		json jsonRow = json::array();
		for (size_t x = 0; x < snapshot->terrainGrid[z].size(); ++x)
		{
			const auto& cell = snapshot->terrainGrid[z][x];

			// Use per-player visibility if available, otherwise fall back to
			// the cell's default visibility (Visible when no FOW data captured).
			FogOfWarState cellVis = cell.visibility;
			if (playerTerrainVis &&
			    z < playerTerrainVis->size() &&
			    x < (*playerTerrainVis)[z].size())
			{
				cellVis = (*playerTerrainVis)[z][x];
			}

			jsonRow.push_back({
				{"terrainType", cell.terrainType},
				{"elevation", cell.elevation},
				{"passable", cell.passable},
				{"visibility", static_cast<int>(cellVis)}
			});
		}
		grid.push_back(jsonRow);
	}

	return {
		{"resolution", snapshot->terrainResolution},
		{"requestedResolution", resolution},
		{"grid", grid},
		{"tickNumber", snapshot->tickNumber}
	};
}

// ---------------------------------------------------------------------------
// Tech tree query (task 6.3)
// ---------------------------------------------------------------------------

json MCPStateReader::queryTechTree(PlayerId viewer) const
{
	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot)
		return json::object();

	auto it = snapshot->techTrees.find(viewer);
	if (it == snapshot->techTrees.end())
		return json::object();

	json technologies = json::array();
	for (const auto& tech : it->second)
	{
		json prereqs = json::array();
		for (const auto& prereq : tech.prerequisites)
			prereqs.push_back(prereq);

		json cost = {
			{"food", tech.cost.food},
			{"wood", tech.cost.wood},
			{"stone", tech.cost.stone},
			{"metal", tech.cost.metal}
		};

		std::string statusStr;
		switch (tech.status)
		{
		case TechStatus::Locked:      statusStr = "locked"; break;
		case TechStatus::Available:   statusStr = "available"; break;
		case TechStatus::Researching: statusStr = "researching"; break;
		case TechStatus::Completed:   statusStr = "completed"; break;
		}

		technologies.push_back({
			{"techId", tech.techId},
			{"name", tech.name},
			{"prerequisites", prereqs},
			{"cost", cost},
			{"status", statusStr},
			{"researchProgress", tech.researchProgress}
		});
	}

	return {{"technologies", technologies}, {"tickNumber", snapshot->tickNumber}};
}

// ---------------------------------------------------------------------------
// Player info query (task 6.3)
// ---------------------------------------------------------------------------

json MCPStateReader::queryPlayerInfo() const
{
	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot)
		return json::object();

	json players = json::array();
	for (const auto& player : snapshot->players)
	{
		players.push_back({
			{"id", player.id},
			{"civilization", player.civilization},
			{"team", player.team},
			{"diplomacyStance", player.diplomacyStance},
			{"currentPopulation", player.currentPopulation},
			{"maxPopulation", player.maxPopulation}
		});
	}

	return {{"players", players}, {"tickNumber", snapshot->tickNumber}};
}

// ---------------------------------------------------------------------------
// Fog-of-war visibility check (task 6.4 — uses pre-computed visibility map)
// ---------------------------------------------------------------------------

bool MCPStateReader::isEntityVisible(EntityId entity, PlayerId viewer) const
{
	auto snapshot = m_snapshot.getSnapshot();
	if (!snapshot)
		return false;

	// Look up the player's visibility map
	auto playerIt = snapshot->entityVisibility.find(viewer);
	if (playerIt == snapshot->entityVisibility.end())
		return true; // No visibility data captured = fog-of-war disabled, show all

	// Look up this entity in the player's map
	auto entityIt = playerIt->second.find(entity);
	if (entityIt == playerIt->second.end())
		return false; // Entity not in visibility map = not known to this player

	// Visible and Explored entities are shown (Explored = fogged but last-known)
	return entityIt->second == FogOfWarState::Visible ||
	       entityIt->second == FogOfWarState::Explored;
}

} // namespace MCP
