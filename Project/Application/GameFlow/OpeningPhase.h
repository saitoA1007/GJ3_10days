#pragma once
#include "IGamePhase.h"

class OpeningPhase : public IGamePhase
{
public:
	OpeningPhase();

	void OnEnter(GameFlowContext& context) override;

	bool OnUpdate(GameFlowContext& context) override;

	const char* GetName() const override { return "Opening Cutscene"; }
	// 操作不可
	bool IsGameplayEnabled() const override { return false; } 

private:
	float timer_ = 0.0f;
};