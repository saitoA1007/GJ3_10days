#pragma once
#include "LaunchPhase.h"
#include "FPSCounter.h"
#include "Application/Result/ResultMovieManager.h"
#include "Application/Rocket/Rocket.h"
#include "Application/Unit/UnitManager.h"

using namespace GameEngine;

void LaunchPhase::OnEnter(GameFlowContext& context)
{
	float clearRate = 0.0f;

	if (context.rocket && context.rocket->GetRequiredEnergy() > 0)
	{
		clearRate = static_cast<float>(context.finalEnergy) / static_cast<float>(context.rocket->GetRequiredEnergy());
	}

	clearRate = (std::max)(0.0f, clearRate);

	if (context.unitManager)
	{
		context.unitManager->RecallAll();       
		context.unitManager->SetGameplayEnabled(false);
	}

	// ムービーを開始する
	context.resultMovieManager_->Start(clearRate);
}

bool LaunchPhase::OnUpdate(GameFlowContext& context)
{
	isFinished_ = context.resultMovieManager_->IsFin();
	return isFinished_;
}