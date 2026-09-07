#pragma once
#include "TutorialPhase.h"
#include "FPSCounter.h"
#include "Application/Energy/EnergySpawner.h"
#include "Application/LockOn/lockOnController.h"
#include "Application/Unit/unitManager.h"

using namespace GameEngine;

void TutorialPhase::OnEnter(GameFlowContext& context)
{
	step_ = Step::SelectEnergy;

	if (context.energySpawner && context.settings)
	{
		const Vector2& positionXZ = context.settings->tutorialEnergyPositionXZ;
		context.energySpawner->SpawnOnGround(
			EnergySize::Small,
			{ positionXZ.x, 0.0f, positionXZ.y });
	}
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
		// TODO: チュートリアル完成後にPlayingへの遷移を再度有効化する。
		return false;
	}
	return false;
}
