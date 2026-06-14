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

#include "mcp/state/MCPSimulationHook.h"

#include "ps/CLogger.h"
#include "scriptinterface/JSON.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"
#include "simulation2/system/CmpPtr.h"
#include "simulation2/system/ComponentManager.h"
#include "simulation2/components/ICmpCommandQueue.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpPathfinder.h"
#include "simulation2/components/ICmpRangeManager.h"

namespace MCP
{

MCPSimulationHook::MCPSimulationHook(MCPGameStateSnapshot& snapshot, MCPCommandQueue& commandQueue)
	: m_snapshot(snapshot)
	, m_commandQueue(commandQueue)
{
}

void MCPSimulationHook::onTickStart(CSimulation2& sim)
{
	// Drain all pending commands from the network thread
	std::vector<MCPCommand> commands = m_commandQueue.drainAll();

	if (!commands.empty())
	{
		LOGMESSAGE("MCP: Injecting %zu commands at tick start.", commands.size());
		injectCommands(sim, commands);
	}
}

void MCPSimulationHook::onTickEnd(CSimulation2& sim)
{
	captureSnapshot(sim);
}

void MCPSimulationHook::captureSnapshot(CSimulation2& sim)
{
	auto state = std::make_shared<GameStateData>();

	// Track the tick number via our own counter.
	state->tickNumber = m_tickCounter;
	++m_tickCounter;

	// Capture all game state categories
	capturePlayerResources(sim, *state);
	captureResourceDeposits(sim, *state);
	captureTerrain(sim, *state);
	captureTechTrees(sim, *state);
	capturePlayerInfo(sim, *state);
	captureUnitsAndBuildings(sim, *state);

	// Capture fog-of-war visibility after entities are captured,
	// so we know which entities to query visibility for.
	captureVisibility(sim, *state);

	m_snapshot.updateSnapshot(std::move(state));
}

void MCPSimulationHook::capturePlayerResources(CSimulation2& sim, GameStateData& state)
{
	// Access the player manager to iterate all players and read resource data.
	// In 0 A.D., player resources are stored as script properties on the Player
	// component (JavaScript-side). The C++ interface is minimal, so we query
	// the script interface to retrieve resource counts.

	CComponentManager& componentMgr = sim.GetComponentManager();
	ICmpPlayerManager* cmpPlayerMgr = static_cast<ICmpPlayerManager*>(
		componentMgr.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager));

	if (!cmpPlayerMgr)
		return;

	const int numPlayers = cmpPlayerMgr->GetNumPlayers();

	for (int playerId = 1; playerId <= numPlayers; ++playerId)
	{
		entity_id_t playerEntity = cmpPlayerMgr->GetPlayerByID(playerId);
		if (playerEntity == INVALID_ENTITY)
			continue;

		PlayerResources resources;

		ScriptInterface& scriptInterface = sim.GetScriptInterface();
		ScriptRequest rq(scriptInterface);

		// Query resource counts via the Player component's script methods
		JS::RootedValue playerData(rq.cx);
		if (scriptInterface.Eval(
			("Engine.QueryInterface(" + std::to_string(playerEntity) +
			 ", IID_Player).GetResourceCounts()").c_str(),
			&playerData))
		{
			JS::RootedValue val(rq.cx);
			if (scriptInterface.GetProperty(playerData, "food", &val))
				scriptInterface.FromJSVal(rq.cx, val, resources.food);
			if (scriptInterface.GetProperty(playerData, "wood", &val))
				scriptInterface.FromJSVal(rq.cx, val, resources.wood);
			if (scriptInterface.GetProperty(playerData, "stone", &val))
				scriptInterface.FromJSVal(rq.cx, val, resources.stone);
			if (scriptInterface.GetProperty(playerData, "metal", &val))
				scriptInterface.FromJSVal(rq.cx, val, resources.metal);
		}

		// Query gathering rates
		JS::RootedValue ratesData(rq.cx);
		if (scriptInterface.Eval(
			("Engine.QueryInterface(" + std::to_string(playerEntity) +
			 ", IID_Player).GetGatherRates()").c_str(),
			&ratesData))
		{
			JS::RootedValue val(rq.cx);
			if (scriptInterface.GetProperty(ratesData, "food", &val))
				scriptInterface.FromJSVal(rq.cx, val, resources.foodRate);
			if (scriptInterface.GetProperty(ratesData, "wood", &val))
				scriptInterface.FromJSVal(rq.cx, val, resources.woodRate);
			if (scriptInterface.GetProperty(ratesData, "stone", &val))
				scriptInterface.FromJSVal(rq.cx, val, resources.stoneRate);
			if (scriptInterface.GetProperty(ratesData, "metal", &val))
				scriptInterface.FromJSVal(rq.cx, val, resources.metalRate);
		}

		state.playerResources[static_cast<PlayerId>(playerId)] = resources;
	}
}

void MCPSimulationHook::captureResourceDeposits(CSimulation2& sim, GameStateData& state)
{
	// Resource deposits are entities with a ResourceSupply component.
	// We iterate all entities with that interface and read position + supply data.

	CComponentManager& componentMgr = sim.GetComponentManager();
	ScriptInterface& scriptInterface = sim.GetScriptInterface();
	ScriptRequest rq(scriptInterface);

	const CSimulation2::InterfaceListUnordered& supplyEntities =
		sim.GetEntitiesWithInterface(IID_ResourceSupply);

	for (const auto& [entityId, component] : supplyEntities)
	{
		ResourceDeposit deposit;
		deposit.id = static_cast<EntityId>(entityId);

		// Get position
		ICmpPosition* cmpPosition = static_cast<ICmpPosition*>(
			componentMgr.QueryInterface(entityId, IID_Position));
		if (!cmpPosition || !cmpPosition->IsInWorld())
			continue;

		CFixedVector3D pos = cmpPosition->GetPosition();
		deposit.position.x = pos.X.ToFloat();
		deposit.position.y = pos.Y.ToFloat();
		deposit.position.z = pos.Z.ToFloat();

		// Query ResourceSupply component for remaining amount
		JS::RootedValue supplyData(rq.cx);
		if (scriptInterface.Eval(
			("Engine.QueryInterface(" + std::to_string(entityId) +
			 ", IID_ResourceSupply).GetCurrentAmount()").c_str(),
			&supplyData))
		{
			scriptInterface.FromJSVal(rq.cx, supplyData, deposit.remainingQuantity);
		}

		// Query resource type (generic category: food, wood, stone, metal)
		JS::RootedValue typeData(rq.cx);
		if (scriptInterface.Eval(
			("Engine.QueryInterface(" + std::to_string(entityId) +
			 ", IID_ResourceSupply).GetType().generic").c_str(),
			&typeData))
		{
			scriptInterface.FromJSVal(rq.cx, typeData, deposit.resourceType);
		}

		state.resourceDeposits.push_back(std::move(deposit));
	}
}

void MCPSimulationHook::captureTerrain(CSimulation2& sim, GameStateData& state)
{
	// Read terrain data from the Terrain component.
	// We capture a grid at a fixed default resolution (every 4 tiles).

	CComponentManager& componentMgr = sim.GetComponentManager();
	ICmpTerrain* cmpTerrain = static_cast<ICmpTerrain*>(
		componentMgr.QueryInterface(SYSTEM_ENTITY, IID_Terrain));

	if (!cmpTerrain)
		return;

	const u16 tilesPerSide = cmpTerrain->GetTilesPerSide();
	if (tilesPerSide == 0)
		return;

	// Default resolution: sample every 4 tiles
	const int resolution = 4;
	const int gridSize = tilesPerSide / resolution;
	state.terrainResolution = resolution;

	// Get pathfinder for passability checks
	ICmpPathfinder* cmpPathfinder = static_cast<ICmpPathfinder*>(
		componentMgr.QueryInterface(SYSTEM_ENTITY, IID_Pathfinder));

	state.terrainGrid.resize(gridSize);
	for (int z = 0; z < gridSize; ++z)
	{
		state.terrainGrid[z].resize(gridSize);
		for (int x = 0; x < gridSize; ++x)
		{
			TerrainCell& cell = state.terrainGrid[z][x];

			// Sample at center of each grid cell
			int tileX = x * resolution + resolution / 2;
			int tileZ = z * resolution + resolution / 2;

			// Elevation from terrain heightmap
			cell.elevation = cmpTerrain->GetExactGroundLevel(
				static_cast<float>(tileX), static_cast<float>(tileZ));

			// Terrain type — placeholder until texture query is wired
			cell.terrainType = "terrain";

			// Passability — default true; full pathfinder check in task 6.4
			cell.passable = true;
			(void)cmpPathfinder; // Will be used in full passability implementation

			// Fog-of-war — default Visible; per-player filtering in task 6.4
			cell.visibility = FogOfWarState::Visible;
		}
	}
}

void MCPSimulationHook::captureTechTrees(CSimulation2& sim, GameStateData& state)
{
	// Technology state is managed by TechnologyManager (JS component).
	// We query via script to get research status per player.

	CComponentManager& componentMgr = sim.GetComponentManager();
	ICmpPlayerManager* cmpPlayerMgr = static_cast<ICmpPlayerManager*>(
		componentMgr.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager));

	if (!cmpPlayerMgr)
		return;

	ScriptInterface& scriptInterface = sim.GetScriptInterface();
	ScriptRequest rq(scriptInterface);

	const int numPlayers = cmpPlayerMgr->GetNumPlayers();

	for (int playerId = 1; playerId <= numPlayers; ++playerId)
	{
		entity_id_t playerEntity = cmpPlayerMgr->GetPlayerByID(playerId);
		if (playerEntity == INVALID_ENTITY)
			continue;

		std::vector<TechnologyState> techs;

		// Query completed technologies
		JS::RootedValue researchedVal(rq.cx);
		if (scriptInterface.Eval(
			("Engine.QueryInterface(" + std::to_string(playerEntity) +
			 ", IID_TechnologyManager)?.GetResearchedTechs() || {}").c_str(),
			&researchedVal))
		{
			std::vector<std::string> researchedTechIds;
			scriptInterface.EnumeratePropertyNames(researchedVal, false, researchedTechIds);

			for (const auto& techId : researchedTechIds)
			{
				TechnologyState ts;
				ts.techId = techId;
				ts.name = techId;
				ts.status = TechStatus::Completed;
				ts.researchProgress = 1.0f;
				techs.push_back(std::move(ts));
			}
		}

		// Query in-progress technologies
		JS::RootedValue researchingVal(rq.cx);
		if (scriptInterface.Eval(
			("Engine.QueryInterface(" + std::to_string(playerEntity) +
			 ", IID_TechnologyManager)?.GetStartedTechs() || {}").c_str(),
			&researchingVal))
		{
			std::vector<std::string> researchingTechIds;
			scriptInterface.EnumeratePropertyNames(researchingVal, false, researchingTechIds);

			for (const auto& techId : researchingTechIds)
			{
				TechnologyState ts;
				ts.techId = techId;
				ts.name = techId;
				ts.status = TechStatus::Researching;
				ts.researchProgress = 0.5f; // Approximate; full progress query later
				techs.push_back(std::move(ts));
			}
		}

		state.techTrees[static_cast<PlayerId>(playerId)] = std::move(techs);
	}
}

void MCPSimulationHook::capturePlayerInfo(CSimulation2& sim, GameStateData& state)
{
	// Read player info: civilization, team, diplomacy, population.

	CComponentManager& componentMgr = sim.GetComponentManager();
	ICmpPlayerManager* cmpPlayerMgr = static_cast<ICmpPlayerManager*>(
		componentMgr.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager));

	if (!cmpPlayerMgr)
		return;

	ScriptInterface& scriptInterface = sim.GetScriptInterface();
	ScriptRequest rq(scriptInterface);

	const int numPlayers = cmpPlayerMgr->GetNumPlayers();

	for (int playerId = 1; playerId <= numPlayers; ++playerId)
	{
		entity_id_t playerEntity = cmpPlayerMgr->GetPlayerByID(playerId);
		if (playerEntity == INVALID_ENTITY)
			continue;

		PlayerInfo info;
		info.id = static_cast<PlayerId>(playerId);

		// Civilization
		JS::RootedValue civVal(rq.cx);
		if (scriptInterface.Eval(
			("Engine.QueryInterface(" + std::to_string(playerEntity) +
			 ", IID_Identity)?.GetCiv() || \"unknown\"").c_str(),
			&civVal))
		{
			scriptInterface.FromJSVal(rq.cx, civVal, info.civilization);
		}
		else
		{
			info.civilization = "unknown";
		}

		// Team
		JS::RootedValue teamVal(rq.cx);
		if (scriptInterface.Eval(
			("Engine.QueryInterface(" + std::to_string(playerEntity) +
			 ", IID_Player)?.GetTeam() ?? -1").c_str(),
			&teamVal))
		{
			scriptInterface.FromJSVal(rq.cx, teamVal, info.team);
		}
		else
		{
			info.team = -1;
		}

		// Diplomacy stance (simplified — full impl would be per-player pair)
		info.diplomacyStance = "neutral";

		// Population count
		JS::RootedValue popVal(rq.cx);
		if (scriptInterface.Eval(
			("Engine.QueryInterface(" + std::to_string(playerEntity) +
			 ", IID_Player)?.GetPopulationCount() ?? 0").c_str(),
			&popVal))
		{
			scriptInterface.FromJSVal(rq.cx, popVal, info.currentPopulation);
		}
		else
		{
			info.currentPopulation = 0;
		}

		// Population limit
		JS::RootedValue maxPopVal(rq.cx);
		if (scriptInterface.Eval(
			("Engine.QueryInterface(" + std::to_string(playerEntity) +
			 ", IID_Player)?.GetPopulationLimit() ?? 0").c_str(),
			&maxPopVal))
		{
			scriptInterface.FromJSVal(rq.cx, maxPopVal, info.maxPopulation);
		}
		else
		{
			info.maxPopulation = 0;
		}

		state.players.push_back(std::move(info));
	}
}

void MCPSimulationHook::captureUnitsAndBuildings(CSimulation2& sim, GameStateData& state)
{
	// Iterate entities with Position to categorize as units or buildings.

	CComponentManager& componentMgr = sim.GetComponentManager();
	ScriptInterface& scriptInterface = sim.GetScriptInterface();
	ScriptRequest rq(scriptInterface);

	const CSimulation2::InterfaceListUnordered& positionEntities =
		sim.GetEntitiesWithInterface(IID_Position);

	for (const auto& [entityId, component] : positionEntities)
	{
		ICmpPosition* cmpPosition = static_cast<ICmpPosition*>(component);
		if (!cmpPosition || !cmpPosition->IsInWorld())
			continue;

		// Get owner
		ICmpOwnership* cmpOwnership = static_cast<ICmpOwnership*>(
			componentMgr.QueryInterface(entityId, IID_Ownership));
		if (!cmpOwnership)
			continue;

		player_id_t owner = cmpOwnership->GetOwner();
		if (owner <= 0)
			continue; // Skip Gaia and unowned entities

		CFixedVector3D pos = cmpPosition->GetPosition();

		// Check if entity is a unit (has UnitAI component)
		JS::RootedValue hasUnitAI(rq.cx);
		bool isUnit = false;
		if (scriptInterface.Eval(
			("!!Engine.QueryInterface(" + std::to_string(entityId) +
			 ", IID_UnitAI)").c_str(),
			&hasUnitAI))
		{
			scriptInterface.FromJSVal(rq.cx, hasUnitAI, isUnit);
		}

		if (isUnit)
		{
			UnitState unit;
			unit.id = static_cast<EntityId>(entityId);
			unit.position.x = pos.X.ToFloat();
			unit.position.y = pos.Y.ToFloat();
			unit.position.z = pos.Z.ToFloat();
			unit.owner = static_cast<PlayerId>(owner);

			// Health
			JS::RootedValue healthVal(rq.cx);
			if (scriptInterface.Eval(
				("Engine.QueryInterface(" + std::to_string(entityId) +
				 ", IID_Health)?.GetHitpoints() ?? 0").c_str(),
				&healthVal))
			{
				scriptInterface.FromJSVal(rq.cx, healthVal, unit.health);
			}

			JS::RootedValue maxHealthVal(rq.cx);
			if (scriptInterface.Eval(
				("Engine.QueryInterface(" + std::to_string(entityId) +
				 ", IID_Health)?.GetMaxHitpoints() ?? 0").c_str(),
				&maxHealthVal))
			{
				scriptInterface.FromJSVal(rq.cx, maxHealthVal, unit.maxHealth);
			}

			// Template name
			JS::RootedValue templateVal(rq.cx);
			if (scriptInterface.Eval(
				("Engine.QueryInterface(" + std::to_string(entityId) +
				 ", IID_Identity)?.GetSelectionGroupName() || \"unknown\"").c_str(),
				&templateVal))
			{
				scriptInterface.FromJSVal(rq.cx, templateVal, unit.templateName);
			}

			// Current action from UnitAI
			JS::RootedValue actionVal(rq.cx);
			if (scriptInterface.Eval(
				("Engine.QueryInterface(" + std::to_string(entityId) +
				 ", IID_UnitAI)?.GetCurrentState() || \"idle\"").c_str(),
				&actionVal))
			{
				scriptInterface.FromJSVal(rq.cx, actionVal, unit.currentAction);
			}

			state.units.push_back(std::move(unit));
		}
		else
		{
			// Check if it's a building (has ProductionQueue or GarrisonHolder)
			JS::RootedValue isBuildingVal(rq.cx);
			bool bldg = false;
			if (scriptInterface.Eval(
				("!!(Engine.QueryInterface(" + std::to_string(entityId) +
				 ", IID_ProductionQueue) || Engine.QueryInterface(" +
				 std::to_string(entityId) + ", IID_GarrisonHolder))").c_str(),
				&isBuildingVal))
			{
				scriptInterface.FromJSVal(rq.cx, isBuildingVal, bldg);
			}

			if (bldg)
			{
				BuildingState building;
				building.id = static_cast<EntityId>(entityId);
				building.position.x = pos.X.ToFloat();
				building.position.y = pos.Y.ToFloat();
				building.position.z = pos.Z.ToFloat();
				building.owner = static_cast<PlayerId>(owner);

				// Health
				JS::RootedValue healthVal(rq.cx);
				if (scriptInterface.Eval(
					("Engine.QueryInterface(" + std::to_string(entityId) +
					 ", IID_Health)?.GetHitpoints() ?? 0").c_str(),
					&healthVal))
				{
					scriptInterface.FromJSVal(rq.cx, healthVal, building.health);
				}

				JS::RootedValue maxHealthVal(rq.cx);
				if (scriptInterface.Eval(
					("Engine.QueryInterface(" + std::to_string(entityId) +
					 ", IID_Health)?.GetMaxHitpoints() ?? 0").c_str(),
					&maxHealthVal))
				{
					scriptInterface.FromJSVal(rq.cx, maxHealthVal, building.maxHealth);
				}

				// Template name
				JS::RootedValue templateVal(rq.cx);
				if (scriptInterface.Eval(
					("Engine.QueryInterface(" + std::to_string(entityId) +
					 ", IID_Identity)?.GetSelectionGroupName() || \"unknown\"").c_str(),
					&templateVal))
				{
					scriptInterface.FromJSVal(rq.cx, templateVal, building.templateName);
				}

				// Construction progress
				building.constructionProgress = 1.0f;
				JS::RootedValue foundationVal(rq.cx);
				if (scriptInterface.Eval(
					("Engine.QueryInterface(" + std::to_string(entityId) +
					 ", IID_Foundation)?.GetBuildProgress() ?? 1.0").c_str(),
					&foundationVal))
				{
					scriptInterface.FromJSVal(rq.cx, foundationVal, building.constructionProgress);
				}

				// Garrison support
				JS::RootedValue garrisonCheck(rq.cx);
				building.supportsGarrison = false;
				if (scriptInterface.Eval(
					("!!Engine.QueryInterface(" + std::to_string(entityId) +
					 ", IID_GarrisonHolder)").c_str(),
					&garrisonCheck))
				{
					scriptInterface.FromJSVal(rq.cx, garrisonCheck, building.supportsGarrison);
				}

				state.buildings.push_back(std::move(building));
			}
		}
	}
}

void MCPSimulationHook::captureVisibility(CSimulation2& sim, GameStateData& state)
{
	// Query ICmpRangeManager for entity and terrain visibility per player.
	// This runs on the simulation thread at tick boundary, so it's safe to
	// access simulation components directly.

	CComponentManager& componentMgr = sim.GetComponentManager();
	ICmpRangeManager* cmpRangeManager = static_cast<ICmpRangeManager*>(
		componentMgr.QueryInterface(SYSTEM_ENTITY, IID_RangeManager));

	if (!cmpRangeManager)
		return;

	// Collect all entity IDs that need visibility checks
	std::vector<EntityId> entityIds;
	entityIds.reserve(state.units.size() + state.buildings.size() + state.resourceDeposits.size());

	for (const auto& unit : state.units)
		entityIds.push_back(unit.id);
	for (const auto& building : state.buildings)
		entityIds.push_back(building.id);
	for (const auto& deposit : state.resourceDeposits)
		entityIds.push_back(deposit.id);

	// Helper to map 0 A.D.'s LosVisibility enum to MCP::FogOfWarState
	auto mapVisibility = [](LosVisibility vis) -> FogOfWarState
	{
		switch (vis)
		{
		case LosVisibility::VISIBLE:
			return FogOfWarState::Visible;
		case LosVisibility::FOGGED:
			return FogOfWarState::Explored;
		case LosVisibility::HIDDEN:
		default:
			return FogOfWarState::Unexplored;
		}
	};

	// Query entity visibility for each active player
	for (const auto& playerInfo : state.players)
	{
		PlayerId playerId = playerInfo.id;
		auto& playerVisMap = state.entityVisibility[playerId];
		playerVisMap.reserve(entityIds.size());

		for (EntityId entityId : entityIds)
		{
			LosVisibility vis = cmpRangeManager->GetLosVisibility(
				static_cast<entity_id_t>(entityId),
				static_cast<player_id_t>(playerId));
			playerVisMap[entityId] = mapVisibility(vis);
		}
	}

	// Capture per-player terrain visibility grid
	// Uses GetLosVisibilityPosition to query visibility of each terrain cell center.
	if (state.terrainGrid.empty() || state.players.empty())
		return;

	const int gridHeight = static_cast<int>(state.terrainGrid.size());
	const int gridWidth = static_cast<int>(state.terrainGrid[0].size());
	const int resolution = state.terrainResolution;

	for (const auto& playerInfo : state.players)
	{
		PlayerId playerId = playerInfo.id;
		auto& terrainVis = state.terrainVisibility[playerId];
		terrainVis.resize(gridHeight);

		for (int z = 0; z < gridHeight; ++z)
		{
			terrainVis[z].resize(gridWidth);
			for (int x = 0; x < gridWidth; ++x)
			{
				// Compute the world position of the terrain cell center
				int tileX = x * resolution + resolution / 2;
				int tileZ = z * resolution + resolution / 2;

				entity_pos_t posX = entity_pos_t::FromInt(tileX);
				entity_pos_t posZ = entity_pos_t::FromInt(tileZ);

				LosVisibility vis = cmpRangeManager->GetLosVisibilityPosition(
					posX, posZ, static_cast<player_id_t>(playerId));
				terrainVis[z][x] = mapVisibility(vis);
			}
		}
	}
}

void MCPSimulationHook::injectCommands(CSimulation2& sim, const std::vector<MCPCommand>& commands)
{
	// Access the simulation command queue component (same path used by the built-in AI).
	CmpPtr<ICmpCommandQueue> cmpCommandQueue(sim, SYSTEM_ENTITY);
	if (!cmpCommandQueue)
	{
		LOGERROR("MCP: Cannot inject commands — ICmpCommandQueue not available.");
		return;
	}

	const ScriptInterface& scriptInterface = sim.GetScriptInterface();
	ScriptRequest rq(scriptInterface);

	for (const auto& cmd : commands)
	{
		JS::RootedValue cmdVal(rq.cx);
		if (convertToJSCommand(rq, cmd, &cmdVal))
		{
			cmpCommandQueue->PushLocalCommand(
				static_cast<player_id_t>(cmd.issuingPlayer), cmdVal);
			LOGMESSAGE("MCP: Injected command type=%d for player=%d (request=%llu).",
				static_cast<int>(cmd.type), cmd.issuingPlayer,
				static_cast<unsigned long long>(cmd.requestId));
		}
		else
		{
			LOGERROR("MCP: Failed to convert command type=%d for player=%d to JS value.",
				static_cast<int>(cmd.type), cmd.issuingPlayer);
		}
	}
}

bool MCPSimulationHook::convertToJSCommand(
	const ScriptRequest& rq, const MCPCommand& cmd, JS::MutableHandleValue out)
{
	// Parse the JSON parameters string into a JS value.
	// The parameters field contains the type-specific data as a JSON object,
	// e.g. {"entities": [101, 102], "x": 150.0, "z": 200.0, "queued": false}
	JS::RootedValue params(rq.cx);
	if (!cmd.parameters.empty())
	{
		if (!Script::ParseJSON(rq, cmd.parameters, &params))
		{
			LOGERROR("MCP: Failed to parse command parameters JSON: %s",
				cmd.parameters.c_str());
			return false;
		}
	}
	else
	{
		// No parameters provided — create empty object
		if (!Script::CreateObject(rq, &params))
			return false;
	}

	// Determine the 0 A.D. command type string based on MCPCommandType.
	// The simulation ProcessCommand function dispatches on the "type" property.
	const char* typeStr = nullptr;
	switch (cmd.type)
	{
	case MCPCommandType::MoveUnits:
		typeStr = "walk";
		break;
	case MCPCommandType::AttackTarget:
		typeStr = "attack";
		break;
	case MCPCommandType::AttackMove:
		typeStr = "attack-walk";
		break;
	case MCPCommandType::TrainUnit:
		typeStr = "train";
		break;
	case MCPCommandType::BuildStructure:
		typeStr = "construct";
		break;
	case MCPCommandType::ResearchTech:
		typeStr = "research";
		break;
	case MCPCommandType::SetGatherPoint:
		typeStr = "set-rallypoint";
		break;
	case MCPCommandType::GatherResource:
		typeStr = "gather";
		break;
	default:
		LOGERROR("MCP: Unknown command type %d", static_cast<int>(cmd.type));
		return false;
	}

	// Set the "type" property on the parameters object so the simulation
	// command processor knows which handler to invoke.
	std::string typeString(typeStr);
	if (!Script::SetProperty(rq, params, "type", typeString))
	{
		LOGERROR("MCP: Failed to set 'type' property on command object.");
		return false;
	}

	out.set(params);
	return true;
}

} // namespace MCP
