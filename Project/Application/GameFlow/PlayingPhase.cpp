#pragma once
#include "PlayingPhase.h"
#include <algorithm>
#include "FPSCounter.h"
#include "Application/Rocket/Rocket.h"
#include "Application/UI/TimeUI.h"

using namespace GameEngine;

PlayingPhase::PlayingPhase() {};

void PlayingPhase::OnEnter(GameFlowContext& context)
{
	if (context.settings)
	{
		remainingTime_ = context.settings->gameDuration;
	}
	if (context.timeUI)
	{
		context.timeUI->SetUnit(0.0f);
		context.timeUI->SetActive(true);
	}
}

bool PlayingPhase::OnUpdate(GameFlowContext& context)
{
	remainingTime_ = (std::max)(remainingTime_ - FpsCounter::deltaTime, 0.0f);

	if (context.timeUI && context.settings)
	{
		const float progress = 1.0f - remainingTime_ / context.settings->gameDuration;
		context.timeUI->SetUnit((std::clamp)(progress, 0.0f, 1.0f));
	}

	return remainingTime_ <= 0.0f; // 時間切れで終了
}

void PlayingPhase::OnExit(GameFlowContext& context)
{
	if (context.timeUI)
	{
		context.timeUI->SetUnit(1.0f);
		context.timeUI->SetActive(false);
	}

	if (context.rocket)
	{
		context.finalEnergy = context.rocket->GetEnergy(); 
	}
}
