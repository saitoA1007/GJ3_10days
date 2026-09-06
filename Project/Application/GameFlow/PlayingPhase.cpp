#pragma once
#include "PlayingPhase.h"
#include "FPSCounter.h"
#include "Application/Rocket/Rocket.h"

using namespace GameEngine;

PlayingPhase::PlayingPhase() {};

void PlayingPhase::OnEnter(GameFlowContext& context)
{
	if (context.settings)
	{
		remainingTime_ = context.settings->gameDuration;
	}
}

bool PlayingPhase::OnUpdate(GameFlowContext& context)
{
	remainingTime_ -= FpsCounter::deltaTime;
	return remainingTime_ <= 0.0f; // 時間切れで終了
}

void PlayingPhase::OnExit(GameFlowContext& context)
{
	if (context.rocket)
	{
		context.finalEnergy = context.rocket->GetEnergy(); 
	}
}