#pragma once
#include <array>
#include <cstdint>

#include "IGamePhase.h"

class EnergyPickup;
class Enemy;
class Unit;

class TutorialPhase : public IGamePhase
{
public:
	enum class Step
	{
		SelectEnergy,
		DispatchUnit,
		WaitForChargeInstruction,
		ChargeEnergy,
		WaitForEnemyCollisionInstruction,
		EnemyCollision,
		WaitForEnemyLockOnInstruction,
		EnemyLockOnOrCollision,
		WaitForEnemyHoldInstruction,
		EnemyHoldOrCollision,
		WaitForUnitHoldInstruction,
		UnitHold,
		Complete,
	};

	void OnEnter(GameFlowContext& context) override;

	bool OnUpdate(GameFlowContext& context) override;
	void OnExit(GameFlowContext& context) override;

	void BeginChargeEnergyStep(GameFlowContext& context);
	void BeginEnemyCollisionStep(GameFlowContext& context);
	void BeginEnemyLockOnStep(GameFlowContext& context);
	void BeginEnemyHoldStep(GameFlowContext& context);
	void BeginUnitHoldStep(GameFlowContext& context);

	const char* GetName() const override { return "Tutorial"; }
	// 操作を許可
	bool IsGameplayEnabled() const override { return true; }

	Step GetStep() const { return step_; }

private:
	Step step_ = Step::SelectEnergy;
	EnergyPickup* tutorialEnergy_ = nullptr;
	EnergyPickup* chargeEnergy_ = nullptr;
	Enemy* enemyLockOnTarget_ = nullptr;
	Enemy* enemyHoldTarget_ = nullptr;
	Unit* tutorialUnitHoldTarget_ = nullptr;
	std::array<EnergyPickup*, 3> tutorialUnitEnergies_{};
	std::array<Enemy*, 2> tutorialStaticEnemies_{};
	float tutorialUnitHoldElapsed_ = 0.0f;
	bool tutorialUnitHoldStarted_ = false;
	bool tutorialUnitBlackholeStarted_ = false;
	uint64_t enemyHitCountAtSpawn_ = 0;
};
