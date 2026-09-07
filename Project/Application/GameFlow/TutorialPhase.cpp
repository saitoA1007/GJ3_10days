#pragma once
#include "TutorialPhase.h"
#include <algorithm>
#include "FPSCounter.h"
#include "InputCommand.h"
#include "Application/Energy/EnergySpawner.h"
#include "Application/Enemy/EnemyManager.h"
#include "Application/LockOn/lockOnController.h"
#include "Application/Rocket/Rocket.h"
#include "Application/Unit/unitManager.h"

using namespace GameEngine;

namespace
{
	constexpr const char* kLockOnReleaseCommand = "LockOnRelease";
}

void TutorialPhase::OnEnter(GameFlowContext& context)
{
	step_ = Step::SelectEnergy;
	chargeEnergy_ = nullptr;
	enemyHitCountAtSpawn_ = context.rocket ? context.rocket->GetEnemyHitCount() : 0;
	if (context.lockOnController)
	{
		context.lockOnController->SetMinimumDispatchHoldSeconds(0.0f);
		context.lockOnController->SetEnemySelectionEnabled(true);
	}

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
		// カーソルが重なっただけではなく、Energyへのロックオン開始で次工程へ進む。
		if (context.lockOnController->GetSelectedEnergy() != nullptr &&
			context.lockOnController->IsCharging())
		{
			step_ = Step::DispatchUnit;
		}
		break;
	case Step::DispatchUnit:
		// Unit出撃後は、Text4が表示されて長押し工程を開始するまで待機する。
		if (context.unitManager->GetDeployedCount() > 0)
		{
			step_ = Step::WaitForChargeInstruction;
		}
		break;
	case Step::WaitForChargeInstruction:
		break;
	case Step::ChargeEnergy:
	{
		const float requiredHoldDuration = context.settings
			? (std::max)(context.settings->tutorialRequiredHoldDuration, 0.0f)
			: 0.0f;
		context.lockOnController->SetMinimumDispatchHoldSeconds(requiredHoldDuration);
		const bool released = context.inputCommand &&
			context.inputCommand->IsCommandActive(kLockOnReleaseCommand);
		const bool heldTargetEnergy =
			context.lockOnController->GetSelectedEnergy() == chargeEnergy_ &&
			context.lockOnController->IsCharging();
		if (released && heldTargetEnergy &&
			context.lockOnController->GetLockOnSeconds() >= requiredHoldDuration)
		{
			step_ = Step::WaitForEnemyCollisionInstruction;
			context.lockOnController->SetMinimumDispatchHoldSeconds(0.0f);
		}
		break;
	}
	case Step::WaitForEnemyCollisionInstruction:
		break;
	case Step::EnemyCollision:
		if (context.rocket && context.rocket->GetEnemyHitCount() > enemyHitCountAtSpawn_)
		{
			step_ = Step::Complete;
			if (context.lockOnController)
			{
				context.lockOnController->SetEnemySelectionEnabled(true);
			}
		}
		break;
	case Step::Complete:
		// TODO: チュートリアル完成後にPlayingへの遷移を再度有効化する。
		return false;
	}
	return false;
}

void TutorialPhase::OnExit(GameFlowContext& context)
{
	if (context.lockOnController)
	{
		context.lockOnController->SetMinimumDispatchHoldSeconds(0.0f);
		context.lockOnController->SetEnemySelectionEnabled(true);
	}
	chargeEnergy_ = nullptr;
}

void TutorialPhase::BeginChargeEnergyStep(GameFlowContext& context)
{
	if (step_ != Step::WaitForChargeInstruction ||
		!context.energySpawner || !context.settings || !context.lockOnController)
	{
		return;
	}

	const Vector2& positionXZ = context.settings->tutorialMediumEnergyPositionXZ;
	chargeEnergy_ = context.energySpawner->SpawnOnGround(
		EnergySize::Medium,
		{ positionXZ.x, 0.0f, positionXZ.y });
	if (chargeEnergy_)
	{
		step_ = Step::ChargeEnergy;
		context.lockOnController->SetMinimumDispatchHoldSeconds(
			context.settings->tutorialRequiredHoldDuration);
	}
}

void TutorialPhase::BeginEnemyCollisionStep(GameFlowContext& context)
{
	if (step_ != Step::WaitForEnemyCollisionInstruction ||
		!context.enemyManager || !context.rocket || !context.settings ||
		!context.lockOnController)
	{
		return;
	}

	enemyHitCountAtSpawn_ = context.rocket->GetEnemyHitCount();
	context.lockOnController->SetEnemySelectionEnabled(false);
	const Vector2& positionXZ = context.settings->tutorialEnemyPositionXZ;
	context.enemyManager->Pop(1, positionXZ, EnemyType::Straight_S);
	step_ = Step::EnemyCollision;
}
