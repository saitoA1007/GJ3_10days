#pragma once
#include "IGamePhase.h"

class PlayingPhase : public IGamePhase 
{
public:
	PlayingPhase();

	void OnEnter(GameFlowContext& context) override;

	bool OnUpdate(GameFlowContext& context) override;

	void OnExit(GameFlowContext& context) override;

	const char* GetName() const override { return "Playing"; }
	bool IsGameplayEnabled() const override { return true; }
	bool IsAutoSpawnEnabled() const override { return true; }
	float GetRemainingTime() const { return remainingTime_; }

private:
	float remainingTime_ = 0.0f;
	bool hasStartedSecondHalfBgm_ = false;
	bool hasStartedHalfTimeView_ = false;
};
