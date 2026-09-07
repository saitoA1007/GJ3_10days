#pragma once
#include "IGamePhase.h"

class LaunchPhase : public IGamePhase
{
public:
	LaunchPhase() = default;

	void OnEnter(GameFlowContext& context) override;
	bool OnUpdate(GameFlowContext& context) override;

	const char* GetName() const override { return "Clear"; }
	bool IsGameplayEnabled() const override { return false; } // 敵の動きなどを止める

private:
	bool isFinished_ = false;
};