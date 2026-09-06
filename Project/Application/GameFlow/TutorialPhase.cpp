#pragma once
#include "TutorialPhase.h"
#include "FPSCounter.h"
#include "Application/LockOn/lockOnController.h"
#include "Application/Unit/unitManager.h"

using namespace GameEngine;

void TutorialPhase::OnEnter(GameFlowContext& context)
{
	step_ = Step::SelectEnergy;
}

bool TutorialPhase::OnUpdate(GameFlowContext& context)
{
	switch (step_)
	{
	case Step::SelectEnergy:
		// 今はとりあえずプレイヤーがEnergyを選択したかチェック
		if (context.lockOnController->GetSelectedEnergy() != nullptr)
		{
			step_ = Step::DispatchUnit;
		}
		break;
	case Step::DispatchUnit:
		// 今はとりあえずユニットを出撃させたらチュートリアル完了
		if (context.unitManager->GetDeployedCount() > 0)
		{
			step_ = Step::Complete;
		}
		break;
	case Step::Complete:
		return true; // 次のフェーズへ
	}
	return false;
}
