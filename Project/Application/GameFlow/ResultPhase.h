#pragma once
#include "IGamePhase.h"

class ResultPhase : public IGamePhase
{
public:
	ResultPhase() = default;

	void OnEnter(GameFlowContext& context) override;
	bool OnUpdate(GameFlowContext& context) override;

	const char* GetName() const override { return "Result"; }
	bool IsGameplayEnabled() const override { return false; }
};