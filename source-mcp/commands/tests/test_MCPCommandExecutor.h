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

#include "mcp/commands/MCPCommandExecutor.h"
#include "mcp/commands/MCPCommandQueue.h"
#include "mcp/state/MCPGameStateSnapshot.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class TestMCPCommandExecutor : public CxxTest::TestSuite
{
private:
	/**
	 * Helper: create a minimal GameStateData with one player, one unit, and one building.
	 */
	std::shared_ptr<MCP::GameStateData> makeBaseState()
	{
		auto state = std::make_shared<MCP::GameStateData>();
		state->tickNumber = 100;

		// Player 1 owns a unit and a completed building
		MCP::UnitState unit;
		unit.id = 10;
		unit.owner = 1;
		unit.position = {50.0f, 0.0f, 50.0f};
		unit.health = 100;
		unit.maxHealth = 100;
		unit.templateName = "units/infantry_swordsman";
		unit.currentAction = "idle";
		state->units.push_back(unit);

		MCP::BuildingState building;
		building.id = 20;
		building.owner = 1;
		building.position = {100.0f, 0.0f, 100.0f};
		building.health = 500;
		building.maxHealth = 500;
		building.templateName = "structures/barracks";
		building.constructionProgress = 1.0f;
		building.supportsGarrison = false;
		state->buildings.push_back(building);

		// Player 2 owns an enemy unit
		MCP::UnitState enemyUnit;
		enemyUnit.id = 30;
		enemyUnit.owner = 2;
		enemyUnit.position = {200.0f, 0.0f, 200.0f};
		enemyUnit.health = 80;
		enemyUnit.maxHealth = 100;
		enemyUnit.templateName = "units/infantry_spearman";
		enemyUnit.currentAction = "idle";
		state->units.push_back(enemyUnit);

		// Player 2 owns an enemy building
		MCP::BuildingState enemyBuilding;
		enemyBuilding.id = 40;
		enemyBuilding.owner = 2;
		enemyBuilding.position = {250.0f, 0.0f, 250.0f};
		enemyBuilding.health = 300;
		enemyBuilding.maxHealth = 500;
		enemyBuilding.templateName = "structures/house";
		enemyBuilding.constructionProgress = 1.0f;
		enemyBuilding.supportsGarrison = false;
		state->buildings.push_back(enemyBuilding);

		// Resource deposit
		MCP::ResourceDeposit deposit;
		deposit.id = 50;
		deposit.position = {150.0f, 0.0f, 150.0f};
		deposit.resourceType = "wood";
		deposit.remainingQuantity = 500;
		state->resourceDeposits.push_back(deposit);

		// Tech tree for player 1
		MCP::TechnologyState availableTech;
		availableTech.techId = "phase_town";
		availableTech.name = "Town Phase";
		availableTech.status = MCP::TechStatus::Available;
		availableTech.researchProgress = 0.0f;

		MCP::TechnologyState lockedTech;
		lockedTech.techId = "phase_city";
		lockedTech.name = "City Phase";
		lockedTech.status = MCP::TechStatus::Locked;
		lockedTech.researchProgress = 0.0f;

		MCP::TechnologyState completedTech;
		completedTech.techId = "gather_food_bonus";
		completedTech.name = "Food Gathering Bonus";
		completedTech.status = MCP::TechStatus::Completed;
		completedTech.researchProgress = 1.0f;

		MCP::TechnologyState researchingTech;
		researchingTech.techId = "armor_upgrade";
		researchingTech.name = "Armor Upgrade";
		researchingTech.status = MCP::TechStatus::Researching;
		researchingTech.researchProgress = 0.5f;

		state->techTrees[1] = {availableTech, lockedTech, completedTech, researchingTech};

		return state;
	}

	MCP::MCPCommand makeCommand(MCP::MCPCommandType type, MCP::PlayerId player, const json& params)
	{
		MCP::MCPCommand cmd;
		cmd.type = type;
		cmd.issuingPlayer = player;
		cmd.requestId = 1;
		cmd.parameters = params.dump();
		return cmd;
	}

public:
	// --- TrainUnit validation ---

	void test_train_valid_building()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::TrainUnit, 1,
			json{{"buildingId", 20}, {"unitType", "units/infantry_swordsman"}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::Valid);
	}

	void test_train_nonexistent_building()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::TrainUnit, 1,
			json{{"buildingId", 999}, {"unitType", "units/infantry_swordsman"}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::InvalidTarget);
	}

	void test_train_enemy_building()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::TrainUnit, 1,
			json{{"buildingId", 40}, {"unitType", "units/infantry_swordsman"}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::InvalidTarget);
	}

	void test_train_incomplete_building()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		// Make building incomplete
		state->buildings[0].constructionProgress = 0.5f;

		auto cmd = makeCommand(MCP::MCPCommandType::TrainUnit, 1,
			json{{"buildingId", 20}, {"unitType", "units/infantry_swordsman"}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::InvalidProduction);
	}

	// --- MoveUnits validation ---

	void test_move_valid_units()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::MoveUnits, 1,
			json{{"unitIds", {10}}, {"targetPosition", {{"x", 100}, {"y", 0}, {"z", 100}}}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::Valid);
	}

	void test_move_nonexistent_unit()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::MoveUnits, 1,
			json{{"unitIds", {999}}, {"targetPosition", {{"x", 100}, {"y", 0}, {"z", 100}}}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::InvalidTarget);
	}

	void test_move_enemy_unit()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::MoveUnits, 1,
			json{{"unitIds", {30}}, {"targetPosition", {{"x", 100}, {"y", 0}, {"z", 100}}}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::InvalidTarget);
	}

	void test_move_empty_unit_list()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::MoveUnits, 1,
			json{{"unitIds", json::array()}, {"targetPosition", {{"x", 100}, {"y", 0}, {"z", 100}}}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::InvalidTarget);
	}

	// --- AttackTarget validation ---

	void test_attack_valid_enemy_unit()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::AttackTarget, 1,
			json{{"unitIds", {10}}, {"targetId", 30}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::Valid);
	}

	void test_attack_valid_enemy_building()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::AttackTarget, 1,
			json{{"unitIds", {10}}, {"targetId", 40}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::Valid);
	}

	void test_attack_own_unit()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::AttackTarget, 1,
			json{{"unitIds", {10}}, {"targetId", 10}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::InvalidTarget);
	}

	void test_attack_nonexistent_target()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::AttackTarget, 1,
			json{{"unitIds", {10}}, {"targetId", 999}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::InvalidTarget);
	}

	// --- AttackMove validation ---

	void test_attack_move_valid()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::AttackMove, 1,
			json{{"unitIds", {10}}, {"targetPosition", {{"x", 200}, {"y", 0}, {"z", 200}}}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::Valid);
	}

	// --- BuildStructure validation ---

	void test_build_valid_builders()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::BuildStructure, 1,
			json{{"buildingType", "structures/house"}, {"position", {{"x", 120}, {"y", 0}, {"z", 120}}}, {"builderIds", {10}}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::Valid);
	}

	void test_build_enemy_builders()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::BuildStructure, 1,
			json{{"buildingType", "structures/house"}, {"position", {{"x", 120}, {"y", 0}, {"z", 120}}}, {"builderIds", {30}}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::InvalidTarget);
	}

	// --- ResearchTech validation ---

	void test_research_available_tech()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::ResearchTech, 1,
			json{{"techId", "phase_town"}, {"buildingId", 20}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::Valid);
	}

	void test_research_locked_tech()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::ResearchTech, 1,
			json{{"techId", "phase_city"}, {"buildingId", 20}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::UnmetPrerequisites);
	}

	void test_research_completed_tech()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::ResearchTech, 1,
			json{{"techId", "gather_food_bonus"}, {"buildingId", 20}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::UnmetPrerequisites);
	}

	void test_research_already_researching()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::ResearchTech, 1,
			json{{"techId", "armor_upgrade"}, {"buildingId", 20}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::BuildingOccupied);
	}

	void test_research_enemy_building()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::ResearchTech, 1,
			json{{"techId", "phase_town"}, {"buildingId", 40}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::InvalidTarget);
	}

	// --- SetGatherPoint validation ---

	void test_set_gather_point_valid()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::SetGatherPoint, 1,
			json{{"buildingId", 20}, {"targetPosition", {{"x", 150}, {"y", 0}, {"z", 150}}}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::Valid);
	}

	void test_set_gather_point_nonexistent_building()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::SetGatherPoint, 1,
			json{{"buildingId", 999}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::InvalidTarget);
	}

	// --- GatherResource validation ---

	void test_gather_valid()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::GatherResource, 1,
			json{{"unitIds", {10}}, {"resourceId", 50}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::Valid);
	}

	void test_gather_nonexistent_resource()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::GatherResource, 1,
			json{{"unitIds", {10}}, {"resourceId", 999}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::InvalidTarget);
	}

	void test_gather_depleted_resource()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		state->resourceDeposits[0].remainingQuantity = 0;

		auto cmd = makeCommand(MCP::MCPCommandType::GatherResource, 1,
			json{{"unitIds", {10}}, {"resourceId", 50}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::InvalidTarget);
	}

	void test_gather_enemy_units()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		auto cmd = makeCommand(MCP::MCPCommandType::GatherResource, 1,
			json{{"unitIds", {30}}, {"resourceId", 50}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::InvalidGatherer);
	}

	// --- Malformed parameters ---

	void test_malformed_json_parameters()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();

		MCP::MCPCommand cmd;
		cmd.type = MCP::MCPCommandType::TrainUnit;
		cmd.issuingPlayer = 1;
		cmd.requestId = 1;
		cmd.parameters = "not valid json{{{";

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::UnknownCommand);
	}

	void test_missing_required_parameters()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		// TrainUnit missing unitType
		auto cmd = makeCommand(MCP::MCPCommandType::TrainUnit, 1,
			json{{"buildingId", 20}});

		auto result = executor.validate(cmd, *state);
		TS_ASSERT_EQUALS(result, MCP::CommandValidationResult::UnknownCommand);
	}

	// --- Execute and handle methods ---

	void test_execute_enqueues_command()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		MCP::MCPCommand cmd;
		cmd.type = MCP::MCPCommandType::MoveUnits;
		cmd.issuingPlayer = 1;
		cmd.requestId = 42;
		cmd.parameters = json{{"unitIds", {10}}, {"targetPosition", {{"x", 100}, {"y", 0}, {"z", 100}}}}.dump();

		executor.execute(std::move(cmd));

		auto drained = queue.drainAll();
		TS_ASSERT_EQUALS(drained.size(), 1u);
		TS_ASSERT_EQUALS(drained[0].type, MCP::MCPCommandType::MoveUnits);
		TS_ASSERT_EQUALS(drained[0].issuingPlayer, 1);
		TS_ASSERT_EQUALS(drained[0].requestId, 42u);
	}

	void test_handle_train_success()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		snapshot.updateSnapshot(state);

		json result = executor.handleTrain(1, 20, "units/infantry_swordsman");
		TS_ASSERT_EQUALS(result["status"], "accepted");
		TS_ASSERT_EQUALS(result["tickQueued"], 100u);

		// Verify command was enqueued
		auto drained = queue.drainAll();
		TS_ASSERT_EQUALS(drained.size(), 1u);
		TS_ASSERT_EQUALS(drained[0].type, MCP::MCPCommandType::TrainUnit);
	}

	void test_handle_train_failure()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		snapshot.updateSnapshot(state);

		// Try to train from nonexistent building
		json result = executor.handleTrain(1, 999, "units/infantry_swordsman");
		TS_ASSERT_EQUALS(result["status"], "error");
		TS_ASSERT(result.contains("error"));

		// Verify nothing was enqueued
		auto drained = queue.drainAll();
		TS_ASSERT_EQUALS(drained.size(), 0u);
	}

	void test_handle_move_success()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		snapshot.updateSnapshot(state);

		MCP::Vec3 target{150.0f, 0.0f, 150.0f};
		json result = executor.handleMove(1, {10}, target, std::nullopt);
		TS_ASSERT_EQUALS(result["status"], "accepted");
	}

	void test_handle_attack_success()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		snapshot.updateSnapshot(state);

		json result = executor.handleAttack(1, {10}, 30);
		TS_ASSERT_EQUALS(result["status"], "accepted");
	}

	void test_handle_gather_success()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		auto state = makeBaseState();
		snapshot.updateSnapshot(state);

		json result = executor.handleGather(1, {10}, 50);
		TS_ASSERT_EQUALS(result["status"], "accepted");
	}

	void test_handle_no_snapshot()
	{
		MCP::MCPGameStateSnapshot snapshot;
		MCP::MCPCommandQueue queue;
		MCP::MCPCommandExecutor executor(snapshot, queue);

		// No snapshot loaded - should return error
		json result = executor.handleTrain(1, 20, "units/infantry_swordsman");
		TS_ASSERT_EQUALS(result["status"], "error");
	}
};
