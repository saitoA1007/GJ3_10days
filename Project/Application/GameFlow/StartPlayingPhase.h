#pragma once

#include "IGamePhase.h"

/// @brief チュートリアル終了後、Playing開始演出を行うための待機フェーズ。
class StartPlayingPhase : public IGamePhase
{
public:
	void OnEnter(GameFlowContext& context) override;
	bool OnUpdate(GameFlowContext& context) override;
	void OnExit(GameFlowContext& context) override;

	const char* GetName() const override { return "StartPlaying"; }

private:
	float remainingTime_ = 0.0f;
};
