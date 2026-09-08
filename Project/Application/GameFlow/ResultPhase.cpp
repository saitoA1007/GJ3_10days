#pragma once
#include "ResultPhase.h"
#include "FPSCounter.h"

using namespace GameEngine;

void ResultPhase::OnEnter(GameFlowContext& context) {
	//スコアをもとにリザルトメッセージを決定する
	int score = 100;
	ResultMessage::Type resultType = ResultMessage::Type::Mousukosi;
	if (score < 10) {
		resultType = ResultMessage::Type::Mousukosi;
	} else if (score < 100) {
		resultType = ResultMessage::Type::Tyakuriku;
	} else {
		resultType = ResultMessage::Type::Tobisugi;
	}

	context.resultMessage->Boot(resultType);
}

bool ResultPhase::OnUpdate(GameFlowContext& context)
{

	return false; 
}