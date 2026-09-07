#pragma once
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
		Complete,
	};

	void OnEnter(GameFlowContext& context) override;

	bool OnUpdate(GameFlowContext& context) override;
	void OnExit(GameFlowContext& context) override;

	void BeginChargeEnergyStep(GameFlowContext& context);

	const char* GetName() const override { return "Tutorial"; }
	// 操作を許可
	bool IsGameplayEnabled() const override { return true; }

	Step GetStep() const { return step_; }

private:
	Step step_ = Step::SelectEnergy;
	EnergyPickup* chargeEnergy_ = nullptr;
};
