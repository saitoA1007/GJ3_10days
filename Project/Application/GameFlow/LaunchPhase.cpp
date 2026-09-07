#pragma once
#include "LaunchPhase.h"
#include "FPSCounter.h"
#include "Application/Result/ResultMovieManager.h"
using namespace GameEngine;

void LaunchPhase::OnEnter(GameFlowContext& context)
{
	// ムービーを開始する
	context.resultMovieManager_->Start();
}

bool LaunchPhase::OnUpdate(GameFlowContext& context)
{

	isFinished_ = context.resultMovieManager_->IsFin();
	return isFinished_;
}