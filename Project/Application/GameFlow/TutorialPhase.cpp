#pragma once
#include "TutorialPhase.h"
#include <algorithm>
#include "AudioManager.h"
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
	constexpr const char* kLockOnPushCommand = "LockOnPush";
	constexpr const char* kLockOnReleaseCommand = "LockOnRelease";
	constexpr const char* kSkipTutorialCommand = "SkipTutorial";
	constexpr const char* kTutorialBgmName = "tutorialBGM.mp3";
	constexpr float kTutorialBgmVolume = 1.0f;
}

void TutorialPhase::OnEnter(GameFlowContext& context)
{
	auto& audioManager = AudioManager::GetInstance();
	const uint32_t tutorialBgmHandle = audioManager.GetHandleByName(kTutorialBgmName);
	// デバッグ操作などで再入場しても多重再生にならないよう、既存の再生を止めてから開始する。
	audioManager.Stop(tutorialBgmHandle);
	audioManager.Play(tutorialBgmHandle, kTutorialBgmVolume, true);

	step_ = Step::SelectEnergy;
	tutorialEnergy_ = nullptr;
	chargeEnergy_ = nullptr;
	enemyLockOnTarget_ = nullptr;
	enemyHoldTarget_ = nullptr;
	tutorialUnitHoldTarget_ = nullptr;
	tutorialUnitEnergies_.fill(nullptr);
	tutorialStaticEnemies_.fill(nullptr);
	tutorialUnitHoldElapsed_ = 0.0f;
	tutorialUnitHoldStarted_ = false;
	tutorialUnitBlackholeStarted_ = false;
	enemyHitCountAtSpawn_ = context.rocket ? context.rocket->GetEnemyHitCount() : 0;
	if (context.lockOnController)
	{
		context.lockOnController->SetMinimumDispatchHoldSeconds(0.0f);
		context.lockOnController->SetEnemySelectionEnabled(true);
		context.lockOnController->SetTutorialAllowedTargets(nullptr, nullptr);
	}

	if (context.energySpawner && context.settings)
	{
		const Vector2& positionXZ = context.settings->tutorialEnergyPositionXZ;
		tutorialEnergy_ = context.energySpawner->SpawnOnGround(
			EnergySize::Small,
			{ positionXZ.x, 0.0f, positionXZ.y });
		if (context.lockOnController)
		{
			context.lockOnController->SetTutorialAllowedTargets(tutorialEnergy_, nullptr);
		}
	}
}

bool TutorialPhase::OnUpdate(GameFlowContext& context)
{
	if (context.inputCommand &&
		context.inputCommand->IsCommandActive(kSkipTutorialCommand))
	{
		return true;
	}

	switch (step_)
	{
	case Step::SelectEnergy:
		// カーソルが重なっただけではなく、Energyへのロックオン開始で次工程へ進む。
		if (context.lockOnController->GetSelectedEnergy() == tutorialEnergy_ &&
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
			context.lockOnController->SetTutorialAllowedTargets(nullptr, nullptr);
		}
		break;
	case Step::WaitForChargeInstruction:
		context.lockOnController->SetTutorialAllowedTargets(nullptr, nullptr);
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
		context.lockOnController->SetTutorialAllowedTargets(nullptr, nullptr);
		break;
	case Step::EnemyCollision:
		if (context.rocket && context.rocket->GetEnemyHitCount() > enemyHitCountAtSpawn_)
		{
			step_ = Step::WaitForEnemyLockOnInstruction;
			if (context.lockOnController)
			{
				context.lockOnController->SetEnemySelectionEnabled(true);
			}
		}
		break;
	case Step::WaitForEnemyLockOnInstruction:
		context.lockOnController->SetTutorialAllowedTargets(nullptr, nullptr);
		break;
	case Step::EnemyLockOnOrCollision:
	{
		const bool enemyLockOnStarted = context.lockOnController &&
			context.lockOnController->GetSelectedEnemy() == enemyLockOnTarget_ &&
			context.lockOnController->IsCharging();
		const bool enemyReachedRocket = context.rocket &&
			context.rocket->GetEnemyHitCount() > enemyHitCountAtSpawn_;
		if (enemyLockOnStarted || enemyReachedRocket)
		{
			step_ = Step::WaitForEnemyHoldInstruction;
		}
		break;
	}
	case Step::WaitForEnemyHoldInstruction:
		context.lockOnController->SetTutorialAllowedTargets(nullptr, nullptr);
		break;
	case Step::EnemyHoldOrCollision:
	{
		const float requiredHoldDuration = context.settings
			? (std::max)(context.settings->tutorialEnemyRequiredHoldDuration, 0.0f)
			: 0.0f;
		context.lockOnController->SetMinimumDispatchHoldSeconds(requiredHoldDuration);
		const bool released = context.inputCommand &&
			context.inputCommand->IsCommandActive(kLockOnReleaseCommand);
		const bool heldTargetEnemy =
			context.lockOnController->GetSelectedEnemy() == enemyHoldTarget_ &&
			context.lockOnController->IsCharging();
		const bool enemyHoldCompleted = released && heldTargetEnemy &&
			context.lockOnController->GetLockOnSeconds() >= requiredHoldDuration;
		const bool enemyDefeated = enemyHoldTarget_ &&
			enemyHoldTarget_->WasDefeated();
		const bool enemyReachedRocket = context.rocket &&
			context.rocket->GetEnemyHitCount() > enemyHitCountAtSpawn_;
		if (enemyHoldCompleted || enemyDefeated || enemyReachedRocket)
		{
			step_ = Step::WaitForUnitHoldInstruction;
			context.lockOnController->SetMinimumDispatchHoldSeconds(0.0f);
		}
		break;
	}
	case Step::WaitForUnitHoldInstruction:
		context.lockOnController->SetTutorialAllowedTargets(nullptr, nullptr);
		break;
	case Step::UnitHold:
	{
		const float requiredHoldDuration = context.settings
			? (std::max)(context.settings->tutorialUnitRequiredHoldDuration, 0.0f)
			: 0.0f;
		const bool isHolding = context.inputCommand &&
			context.inputCommand->IsCommandActive(kLockOnPushCommand);
		const bool released = context.inputCommand &&
			context.inputCommand->IsCommandActive(kLockOnReleaseCommand);
		const bool targetHovered = context.lockOnController &&
			context.lockOnController->IsTutorialHoldTargetHovered();

		if (!tutorialUnitBlackholeStarted_ && isHolding && targetHovered)
		{
			if (!tutorialUnitHoldStarted_)
			{
				tutorialUnitHoldElapsed_ = 0.0f;
				tutorialUnitHoldStarted_ = true;
			}
			tutorialUnitHoldElapsed_ +=
				(std::max)(GameEngine::FpsCounter::deltaTime, 0.0f);

			if (tutorialUnitHoldElapsed_ >= requiredHoldDuration &&
				tutorialUnitHoldTarget_)
			{
				tutorialUnitBlackholeStarted_ =
					tutorialUnitHoldTarget_->ActivateBlackhole();
			}
		}
		else if (!tutorialUnitBlackholeStarted_ && isHolding)
		{
			tutorialUnitHoldElapsed_ = 0.0f;
			tutorialUnitHoldStarted_ = false;
		}

		if (released)
		{
			if (tutorialUnitBlackholeStarted_)
			{
				step_ = Step::Complete;
			}
			tutorialUnitHoldElapsed_ = 0.0f;
			tutorialUnitHoldStarted_ = false;
			tutorialUnitBlackholeStarted_ = false;
		}
		break;
	}
	case Step::Complete:
		// TODO: チュートリアル完成後にPlayingへの遷移を再度有効化する。
		return false;
	}
	return false;
}

void TutorialPhase::OnExit(GameFlowContext& context)
{
	auto& audioManager = AudioManager::GetInstance();
	audioManager.Stop(audioManager.GetHandleByName(kTutorialBgmName));

	if (context.lockOnController)
	{
		context.lockOnController->SetMinimumDispatchHoldSeconds(0.0f);
		context.lockOnController->SetEnemySelectionEnabled(true);
		context.lockOnController->ClearTutorialInteractionRestriction();
	}
	if (context.unitManager)
	{
		context.unitManager->RecallTutorialStaticUnit(tutorialUnitHoldTarget_);
	}
	for (EnergyPickup* energy : tutorialUnitEnergies_)
	{
		if (energy && energy->IsActive())
		{
			energy->Deactivate();
		}
	}
	if (context.enemyManager)
	{
		for (Enemy* enemy : tutorialStaticEnemies_)
		{
			context.enemyManager->Despawn(enemy);
		}
	}
	tutorialEnergy_ = nullptr;
	chargeEnergy_ = nullptr;
	enemyLockOnTarget_ = nullptr;
	enemyHoldTarget_ = nullptr;
	tutorialUnitHoldTarget_ = nullptr;
	tutorialUnitEnergies_.fill(nullptr);
	tutorialStaticEnemies_.fill(nullptr);
	tutorialUnitHoldElapsed_ = 0.0f;
	tutorialUnitHoldStarted_ = false;
	tutorialUnitBlackholeStarted_ = false;
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
		context.lockOnController->SetTutorialAllowedTargets(chargeEnergy_, nullptr);
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
	context.lockOnController->SetTutorialAllowedTargets(nullptr, nullptr);
	const Vector2& positionXZ = context.settings->tutorialEnemyPositionXZ;
	context.enemyManager->Pop(1, positionXZ, EnemyType::Straight_S);
	step_ = Step::EnemyCollision;
}

void TutorialPhase::BeginEnemyLockOnStep(GameFlowContext& context)
{
	if (step_ != Step::WaitForEnemyLockOnInstruction ||
		!context.enemyManager || !context.rocket || !context.settings ||
		!context.lockOnController)
	{
		return;
	}

	enemyHitCountAtSpawn_ = context.rocket->GetEnemyHitCount();
	context.lockOnController->SetEnemySelectionEnabled(true);
	const Vector2& positionXZ = context.settings->tutorialLockOnEnemyPositionXZ;
	enemyLockOnTarget_ = context.enemyManager->Pop(1, positionXZ, EnemyType::Straight_S);
	if (enemyLockOnTarget_)
	{
		context.lockOnController->SetTutorialAllowedTargets(nullptr, enemyLockOnTarget_);
		step_ = Step::EnemyLockOnOrCollision;
	}
	else
	{
		context.lockOnController->SetTutorialAllowedTargets(nullptr, nullptr);
	}
}

void TutorialPhase::BeginEnemyHoldStep(GameFlowContext& context)
{
	if (step_ != Step::WaitForEnemyHoldInstruction ||
		!context.enemyManager || !context.rocket || !context.settings ||
		!context.lockOnController)
	{
		return;
	}

	// 前工程のロックオンを持ち越さず、text12で改めて長押ししてもらう。
	context.lockOnController->SetEnemySelectionEnabled(false);
	context.lockOnController->SetEnemySelectionEnabled(true);
	context.lockOnController->SetMinimumDispatchHoldSeconds(
		context.settings->tutorialEnemyRequiredHoldDuration);
	enemyHitCountAtSpawn_ = context.rocket->GetEnemyHitCount();
	const Vector2& positionXZ = context.settings->tutorialEnemyHoldPositionXZ;
	enemyHoldTarget_ = context.enemyManager->Pop(1, positionXZ, EnemyType::Straight_S);
	if (enemyHoldTarget_)
	{
		context.lockOnController->SetTutorialAllowedTargets(nullptr, enemyHoldTarget_);
		step_ = Step::EnemyHoldOrCollision;
	}
	else
	{
		context.lockOnController->SetTutorialAllowedTargets(nullptr, nullptr);
		context.lockOnController->SetMinimumDispatchHoldSeconds(0.0f);
	}
}

void TutorialPhase::BeginUnitHoldStep(GameFlowContext& context)
{
	if (step_ != Step::WaitForUnitHoldInstruction ||
		!context.unitManager || !context.energySpawner ||
		!context.enemyManager || !context.lockOnController || !context.settings)
	{
		return;
	}

	const Vector2& unitPosition = context.settings->tutorialStaticUnitPositionXZ;
	tutorialUnitHoldTarget_ = context.unitManager->SpawnTutorialStaticUnit(
		{ unitPosition.x, 0.0f, unitPosition.y });
	if (!tutorialUnitHoldTarget_)
	{
		return;
	}

	const std::array<Vector2, 3> energyPositions = {
		context.settings->tutorialUnitEnergy0PositionXZ,
		context.settings->tutorialUnitEnergy1PositionXZ,
		context.settings->tutorialUnitEnergy2PositionXZ,
	};
	for (size_t i = 0; i < energyPositions.size(); ++i)
	{
		tutorialUnitEnergies_[i] = context.energySpawner->SpawnOnGround(
			EnergySize::Small,
			{ energyPositions[i].x, 0.0f, energyPositions[i].y });
	}

	const std::array<Vector2, 2> enemyPositions = {
		context.settings->tutorialStaticEnemy0PositionXZ,
		context.settings->tutorialStaticEnemy1PositionXZ,
	};
	for (size_t i = 0; i < enemyPositions.size(); ++i)
	{
		tutorialStaticEnemies_[i] = context.enemyManager->Pop(
			1, enemyPositions[i], EnemyType::Straight_S);
		if (tutorialStaticEnemies_[i])
		{
			tutorialStaticEnemies_[i]->SetMovementEnabled(false);
		}
	}

	tutorialUnitHoldElapsed_ = 0.0f;
	tutorialUnitHoldStarted_ = false;
	tutorialUnitBlackholeStarted_ = false;
	context.lockOnController->SetTutorialHoldTarget(tutorialUnitHoldTarget_);
	step_ = Step::UnitHold;
}
