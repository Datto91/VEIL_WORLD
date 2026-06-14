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

#include "mcp/state/MCPStateReader.h"
#include "mcp/state/MCPGameStateSnapshot.h"
#include "mcp/state/MCPGameStateData.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class TestMCPStateReader : public CxxTest::TestSuite
{
public:
	// -----------------------------------------------------------------------
	// Units tests (task 6.2, updated for task 6.3 API)
	// -----------------------------------------------------------------------

	void test_queryUnits_returns_empty_object_when_no_snapshot()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		json result = reader.queryUnits(1);
		TS_ASSERT(result.is_object());
		TS_ASSERT(result.empty());
	}

	void test_queryUnits_returns_all_units_with_required_fields()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		auto state = std::make_shared<MCP::GameStateData>();
		state->tickNumber = 5;

		MCP::UnitState u1;
		u1.id = 100;
		u1.position = {10.0f, 0.0f, 20.0f};
		u1.health = 80;
		u1.maxHealth = 100;
		u1.owner = 1;
		u1.templateName = "units/athen/infantry_spearman_b";
		u1.currentAction = "moving";
		state->units.push_back(u1);

		MCP::UnitState u2;
		u2.id = 101;
		u2.position = {50.0f, 5.0f, 60.0f};
		u2.health = 100;
		u2.maxHealth = 100;
		u2.owner = 2;
		u2.templateName = "units/rome/cavalry_swordsman_e";
		u2.currentAction = "idle";
		state->units.push_back(u2);

		snapshot.updateSnapshot(std::move(state));

		json result = reader.queryUnits(1);
		TS_ASSERT(result.is_object());
		TS_ASSERT(result.contains("units"));
		TS_ASSERT(result.contains("tickNumber"));
		TS_ASSERT_EQUALS(result["tickNumber"].get<uint32_t>(), 5u);

		json units = result["units"];
		TS_ASSERT_EQUALS(units.size(), 2u);

		TS_ASSERT_EQUALS(units[0]["id"].get<MCP::EntityId>(), 100u);
		TS_ASSERT_DELTA(units[0]["position"]["x"].get<float>(), 10.0f, 0.001f);
		TS_ASSERT_EQUALS(units[0]["health"].get<int>(), 80);
		TS_ASSERT_EQUALS(units[0]["type"].get<std::string>(), "units/athen/infantry_spearman_b");
		TS_ASSERT_EQUALS(units[0]["currentAction"].get<std::string>(), "moving");
	}

	// -----------------------------------------------------------------------
	// Buildings tests (task 6.2, updated for task 6.3 API)
	// -----------------------------------------------------------------------

	void test_queryBuildings_returns_empty_object_when_no_snapshot()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		json result = reader.queryBuildings(1);
		TS_ASSERT(result.is_object());
		TS_ASSERT(result.empty());
	}

	void test_queryBuildings_returns_buildings_with_full_fields()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		auto state = std::make_shared<MCP::GameStateData>();
		state->tickNumber = 10;

		MCP::BuildingState b1;
		b1.id = 200;
		b1.position = {100.0f, 0.0f, 150.0f};
		b1.health = 500;
		b1.maxHealth = 800;
		b1.owner = 1;
		b1.templateName = "structures/athen/civil_centre";
		b1.constructionProgress = 0.75f;
		b1.supportsGarrison = true;
		b1.garrisonedUnits = {300, 301};

		MCP::ProductionQueueEntry pq;
		pq.unitType = "units/athen/infantry_spearman_b";
		pq.progress = 0.5f;
		pq.timeRemaining = 120;
		b1.productionQueue.push_back(pq);

		state->buildings.push_back(b1);
		snapshot.updateSnapshot(std::move(state));

		json result = reader.queryBuildings(1);
		TS_ASSERT(result.is_object());
		TS_ASSERT(result.contains("buildings"));

		json buildings = result["buildings"];
		TS_ASSERT_EQUALS(buildings.size(), 1u);

		json building = buildings[0];
		TS_ASSERT_EQUALS(building["id"].get<MCP::EntityId>(), 200u);
		TS_ASSERT_DELTA(building["constructionProgress"].get<float>(), 0.75f, 0.001f);
		TS_ASSERT_EQUALS(building["supportsGarrison"].get<bool>(), true);
		TS_ASSERT_EQUALS(building["garrisonedUnits"].size(), 2u);
		TS_ASSERT_EQUALS(building["productionQueue"].size(), 1u);
		TS_ASSERT_EQUALS(building["productionQueue"][0]["unitType"].get<std::string>(),
			"units/athen/infantry_spearman_b");
	}

	// -----------------------------------------------------------------------
	// Resources tests (task 6.3)
	// -----------------------------------------------------------------------

	void test_queryResources_returns_empty_object_when_no_snapshot()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		json result = reader.queryResources(1);
		TS_ASSERT(result.is_object());
		TS_ASSERT(result.empty());
	}

	void test_queryResources_returns_empty_when_player_not_found()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		auto state = std::make_shared<MCP::GameStateData>();
		state->tickNumber = 1;
		// No resources added for any player
		snapshot.updateSnapshot(std::move(state));

		json result = reader.queryResources(1);
		TS_ASSERT(result.is_object());
		TS_ASSERT(result.empty());
	}

	void test_queryResources_returns_correct_resource_values()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		auto state = std::make_shared<MCP::GameStateData>();
		state->tickNumber = 7;

		MCP::PlayerResources res;
		res.food = 300;
		res.wood = 200;
		res.stone = 150;
		res.metal = 100;
		res.foodRate = 5.5f;
		res.woodRate = 3.2f;
		res.stoneRate = 1.0f;
		res.metalRate = 0.8f;
		state->playerResources[1] = res;

		snapshot.updateSnapshot(std::move(state));

		json result = reader.queryResources(1);
		TS_ASSERT(result.is_object());
		TS_ASSERT_EQUALS(result["food"].get<int>(), 300);
		TS_ASSERT_EQUALS(result["wood"].get<int>(), 200);
		TS_ASSERT_EQUALS(result["stone"].get<int>(), 150);
		TS_ASSERT_EQUALS(result["metal"].get<int>(), 100);
		TS_ASSERT_DELTA(result["gatheringRates"]["food"].get<float>(), 5.5f, 0.01f);
		TS_ASSERT_DELTA(result["gatheringRates"]["wood"].get<float>(), 3.2f, 0.01f);
		TS_ASSERT_DELTA(result["gatheringRates"]["stone"].get<float>(), 1.0f, 0.01f);
		TS_ASSERT_DELTA(result["gatheringRates"]["metal"].get<float>(), 0.8f, 0.01f);
		TS_ASSERT_EQUALS(result["tickNumber"].get<uint32_t>(), 7u);
	}

	// -----------------------------------------------------------------------
	// Resource locations tests (task 6.3)
	// -----------------------------------------------------------------------

	void test_queryResourceLocations_returns_empty_when_no_snapshot()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		json result = reader.queryResourceLocations(1);
		TS_ASSERT(result.is_object());
		TS_ASSERT(result.empty());
	}

	void test_queryResourceLocations_returns_deposits()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		auto state = std::make_shared<MCP::GameStateData>();
		state->tickNumber = 3;

		MCP::ResourceDeposit d1;
		d1.id = 500;
		d1.position = {80.0f, 0.0f, 120.0f};
		d1.resourceType = "wood";
		d1.remainingQuantity = 1000;
		state->resourceDeposits.push_back(d1);

		MCP::ResourceDeposit d2;
		d2.id = 501;
		d2.position = {200.0f, 5.0f, 50.0f};
		d2.resourceType = "stone";
		d2.remainingQuantity = 5000;
		state->resourceDeposits.push_back(d2);

		snapshot.updateSnapshot(std::move(state));

		json result = reader.queryResourceLocations(1);
		TS_ASSERT(result.contains("deposits"));

		json deposits = result["deposits"];
		TS_ASSERT_EQUALS(deposits.size(), 2u);

		TS_ASSERT_EQUALS(deposits[0]["id"].get<MCP::EntityId>(), 500u);
		TS_ASSERT_EQUALS(deposits[0]["resourceType"].get<std::string>(), "wood");
		TS_ASSERT_EQUALS(deposits[0]["remainingQuantity"].get<int>(), 1000);
		TS_ASSERT_DELTA(deposits[0]["position"]["x"].get<float>(), 80.0f, 0.01f);

		TS_ASSERT_EQUALS(deposits[1]["id"].get<MCP::EntityId>(), 501u);
		TS_ASSERT_EQUALS(deposits[1]["resourceType"].get<std::string>(), "stone");
	}

	// -----------------------------------------------------------------------
	// Terrain tests (task 6.3)
	// -----------------------------------------------------------------------

	void test_queryTerrain_returns_empty_when_no_snapshot()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		json result = reader.queryTerrain(1, 4);
		TS_ASSERT(result.is_object());
		TS_ASSERT(result.empty());
	}

	void test_queryTerrain_returns_empty_when_grid_is_empty()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		auto state = std::make_shared<MCP::GameStateData>();
		state->tickNumber = 1;
		// terrainGrid is empty by default
		snapshot.updateSnapshot(std::move(state));

		json result = reader.queryTerrain(1, 4);
		TS_ASSERT(result.is_object());
		TS_ASSERT(result.empty());
	}

	void test_queryTerrain_returns_grid_with_correct_structure()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		auto state = std::make_shared<MCP::GameStateData>();
		state->tickNumber = 2;
		state->terrainResolution = 4;

		// Create a 2x2 terrain grid
		state->terrainGrid.resize(2);
		for (int z = 0; z < 2; ++z)
		{
			state->terrainGrid[z].resize(2);
			for (int x = 0; x < 2; ++x)
			{
				MCP::TerrainCell& cell = state->terrainGrid[z][x];
				cell.terrainType = "grass";
				cell.elevation = 10.0f + z * 2.0f + x;
				cell.passable = true;
				cell.visibility = MCP::FogOfWarState::Visible;
			}
		}
		// Make one cell impassable
		state->terrainGrid[1][1].passable = false;
		state->terrainGrid[1][1].terrainType = "water";

		snapshot.updateSnapshot(std::move(state));

		json result = reader.queryTerrain(1, 4);
		TS_ASSERT(result.contains("resolution"));
		TS_ASSERT(result.contains("grid"));
		TS_ASSERT_EQUALS(result["resolution"].get<int>(), 4);

		json grid = result["grid"];
		TS_ASSERT_EQUALS(grid.size(), 2u);
		TS_ASSERT_EQUALS(grid[0].size(), 2u);

		// Check cell [0][0]
		TS_ASSERT_EQUALS(grid[0][0]["terrainType"].get<std::string>(), "grass");
		TS_ASSERT_DELTA(grid[0][0]["elevation"].get<float>(), 10.0f, 0.01f);
		TS_ASSERT_EQUALS(grid[0][0]["passable"].get<bool>(), true);
		TS_ASSERT_EQUALS(grid[0][0]["visibility"].get<int>(), static_cast<int>(MCP::FogOfWarState::Visible));

		// Check impassable cell [1][1]
		TS_ASSERT_EQUALS(grid[1][1]["terrainType"].get<std::string>(), "water");
		TS_ASSERT_EQUALS(grid[1][1]["passable"].get<bool>(), false);
	}

	// -----------------------------------------------------------------------
	// Tech tree tests (task 6.3)
	// -----------------------------------------------------------------------

	void test_queryTechTree_returns_empty_when_no_snapshot()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		json result = reader.queryTechTree(1);
		TS_ASSERT(result.is_object());
		TS_ASSERT(result.empty());
	}

	void test_queryTechTree_returns_empty_when_player_not_in_tree()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		auto state = std::make_shared<MCP::GameStateData>();
		state->tickNumber = 1;
		// No tech tree entries for player 1
		snapshot.updateSnapshot(std::move(state));

		json result = reader.queryTechTree(1);
		TS_ASSERT(result.is_object());
		TS_ASSERT(result.empty());
	}

	void test_queryTechTree_returns_technologies_with_all_fields()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		auto state = std::make_shared<MCP::GameStateData>();
		state->tickNumber = 15;

		MCP::TechnologyState tech1;
		tech1.techId = "phase_town";
		tech1.name = "Town Phase";
		tech1.prerequisites = {"phase_village"};
		tech1.cost.food = 500;
		tech1.cost.wood = 500;
		tech1.cost.stone = 0;
		tech1.cost.metal = 0;
		tech1.status = MCP::TechStatus::Completed;
		tech1.researchProgress = 1.0f;

		MCP::TechnologyState tech2;
		tech2.techId = "phase_city";
		tech2.name = "City Phase";
		tech2.prerequisites = {"phase_town"};
		tech2.cost.food = 1000;
		tech2.cost.wood = 1000;
		tech2.cost.stone = 0;
		tech2.cost.metal = 0;
		tech2.status = MCP::TechStatus::Researching;
		tech2.researchProgress = 0.6f;

		MCP::TechnologyState tech3;
		tech3.techId = "unlock_champion_infantry";
		tech3.name = "Champion Infantry";
		tech3.prerequisites = {"phase_city", "barracks_upgrade"};
		tech3.cost.food = 200;
		tech3.cost.wood = 0;
		tech3.cost.stone = 0;
		tech3.cost.metal = 300;
		tech3.status = MCP::TechStatus::Locked;
		tech3.researchProgress = 0.0f;

		state->techTrees[1] = {tech1, tech2, tech3};
		snapshot.updateSnapshot(std::move(state));

		json result = reader.queryTechTree(1);
		TS_ASSERT(result.contains("technologies"));

		json techs = result["technologies"];
		TS_ASSERT_EQUALS(techs.size(), 3u);

		// Completed tech
		TS_ASSERT_EQUALS(techs[0]["techId"].get<std::string>(), "phase_town");
		TS_ASSERT_EQUALS(techs[0]["status"].get<std::string>(), "completed");
		TS_ASSERT_DELTA(techs[0]["researchProgress"].get<float>(), 1.0f, 0.01f);
		TS_ASSERT_EQUALS(techs[0]["cost"]["food"].get<int>(), 500);

		// Researching tech
		TS_ASSERT_EQUALS(techs[1]["techId"].get<std::string>(), "phase_city");
		TS_ASSERT_EQUALS(techs[1]["status"].get<std::string>(), "researching");
		TS_ASSERT_DELTA(techs[1]["researchProgress"].get<float>(), 0.6f, 0.01f);

		// Locked tech with multiple prerequisites
		TS_ASSERT_EQUALS(techs[2]["status"].get<std::string>(), "locked");
		TS_ASSERT_EQUALS(techs[2]["prerequisites"].size(), 2u);
		TS_ASSERT_EQUALS(techs[2]["prerequisites"][0].get<std::string>(), "phase_city");
	}

	// -----------------------------------------------------------------------
	// Player info tests (task 6.3)
	// -----------------------------------------------------------------------

	void test_queryPlayerInfo_returns_empty_when_no_snapshot()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		json result = reader.queryPlayerInfo();
		TS_ASSERT(result.is_object());
		TS_ASSERT(result.empty());
	}

	void test_queryPlayerInfo_returns_all_players()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPStateReader reader(snapshot);

		auto state = std::make_shared<MCP::GameStateData>();
		state->tickNumber = 20;

		MCP::PlayerInfo p1;
		p1.id = 1;
		p1.civilization = "athen";
		p1.team = 1;
		p1.diplomacyStance = "ally";
		p1.currentPopulation = 45;
		p1.maxPopulation = 200;
		state->players.push_back(p1);

		MCP::PlayerInfo p2;
		p2.id = 2;
		p2.civilization = "rome";
		p2.team = 2;
		p2.diplomacyStance = "enemy";
		p2.currentPopulation = 60;
		p2.maxPopulation = 200;
		state->players.push_back(p2);

		snapshot.updateSnapshot(std::move(state));

		json result = reader.queryPlayerInfo();
		TS_ASSERT(result.contains("players"));
		TS_ASSERT(result.contains("tickNumber"));
		TS_ASSERT_EQUALS(result["tickNumber"].get<uint32_t>(), 20u);

		json players = result["players"];
		TS_ASSERT_EQUALS(players.size(), 2u);

		TS_ASSERT_EQUALS(players[0]["id"].get<MCP::PlayerId>(), 1);
		TS_ASSERT_EQUALS(players[0]["civilization"].get<std::string>(), "athen");
		TS_ASSERT_EQUALS(players[0]["team"].get<int>(), 1);
		TS_ASSERT_EQUALS(players[0]["diplomacyStance"].get<std::string>(), "ally");
		TS_ASSERT_EQUALS(players[0]["currentPopulation"].get<int>(), 45);
		TS_ASSERT_EQUALS(players[0]["maxPopulation"].get<int>(), 200);

		TS_ASSERT_EQUALS(players[1]["id"].get<MCP::PlayerId>(), 2);
		TS_ASSERT_EQUALS(players[1]["civilization"].get<std::string>(), "rome");
		TS_ASSERT_EQUALS(players[1]["diplomacyStance"].get<std::string>(), "enemy");
	}
};
