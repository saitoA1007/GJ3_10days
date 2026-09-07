#pragma once
#include <cstdint>

#include "IGamePhase.h"

class EnergyPickup;

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
		Complete,
	};

	void OnEnter(GameFlowContext& context) override;

	bool OnUpdate(GameFlowContext& context) override;
	void OnExit(GameFlowContext& context) override;

	void BeginChargeEnergyStep(GameFlowContext& context);
	void BeginEnemyCollisionStep(GameFlowContext& context);
	void BeginEnemyLockOnStep(GameFlowContext& context);

	const char* GetName() const override { return "Tutorial"; }
	// 操作を許可
	bool IsGameplayEnabled() const override { return true; }

	Step GetStep() const { return step_; }

private:
	Step step_ = Step::SelectEnergy;
	EnergyPickup* chargeEnergy_ = nullptr;
	uint64_t enemyHitCountAtSpawn_ = 0;
};
