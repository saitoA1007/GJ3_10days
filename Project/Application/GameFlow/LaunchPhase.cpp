#pragma once
#include "LaunchPhase.h"
#include "FPSCounter.h"

using namespace GameEngine;

void LaunchPhase::OnEnter(GameFlowContext& context)
{
	timer_ = context.settings ? context.settings->launchDuration : 3.0f;

}

bool LaunchPhase::OnUpdate(GameFlowContext& context)
{
	const float deltaTime = (std::max)(FpsCounter::deltaTime, 0.0f);
	timer_ -= deltaTime;
	return timer_ <= 0.0f; // 時間経過でリザルトへ
}