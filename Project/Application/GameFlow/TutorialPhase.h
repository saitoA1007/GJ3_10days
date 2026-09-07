#pragma once
#include "IGamePhase.h"

class TutorialPhase : public IGamePhase
{
public:
	enum class Step { SelectEnergy, DispatchUnit, Complete };

	void OnEnter(GameFlowContext& context) override;

	bool OnUpdate(GameFlowContext& context) override;

	const char* GetName() const override { return "Tutorial"; }
	// 操作を許可
	bool IsGameplayEnabled() const override { return true; }

	Step GetStep() const { return step_; }

private:
	Step step_ = Step::SelectEnergy;
};
