#pragma once
#include "OpeningPhase.h"
#include "FPSCounter.h"

using namespace GameEngine;

OpeningPhase::OpeningPhase() {};

void OpeningPhase::OnEnter(GameFlowContext& context)
{
	if (context.settings)
	{
		timer_ = context.settings->openingDuration;
	}
}

bool OpeningPhase::OnUpdate(GameFlowContext& context)
{
	timer_ -= FpsCounter::deltaTime;
	return timer_ <= 0.0f; // 時間経過で次へ
}
