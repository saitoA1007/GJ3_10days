#pragma once
#include "ResultPhase.h"
#include "FPSCounter.h"

using namespace GameEngine;

void ResultPhase::OnEnter(GameFlowContext& context) {
	context.resultMessage->Boot(context.finalEnergy);
}

bool ResultPhase::OnUpdate(GameFlowContext& context)
{

	return false; 
}