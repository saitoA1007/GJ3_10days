#include "StartPlayingPhase.h"

#include <algorithm>

#include "FPSCounter.h"
#include "Application/Enemy/EnemyManager.h"
#include "Application/Energy/EnergySpawner.h"
#include "Application/LockOn/LockOnController.h"
#include "Application/Rocket/Rocket.h"
#include "Application/StartPlaying/StartPlayingView.h"
#include "Application/Tutorial/TutorialCameraModelView.h"
#include "Application/Unit/UnitManager.h"

void StartPlayingPhase::OnEnter(GameFlowContext& context)
{
	remainingTime_ = context.settings
		? (std::max)(context.settings->startPlayingDuration, 0.0f)
		: 0.0f;

	// チャージ中の返却Energyがリセット後に加算されないよう、最初に操作を解除する。
	if (context.lockOnController)
	{
		context.lockOnController->SetGameplayEnabled(false);
	}

	// 運搬物と攻撃予約を解放してから、チュートリアル中の全オブジェクトを初期化する。
	if (context.unitManager)
	{
		context.unitManager->RecallAll();
	}
	if (context.energySpawner)
	{
		context.energySpawner->ResetAll();
	}
	if (context.enemyManager)
	{
		context.enemyManager->ResetAll();
	}

	// チュートリアル中の増減を持ち越さず、本編開始演出では初期Energyから始める。
	if (context.rocket)
	{
		context.rocket->ResetEnergy();
	}

	if (context.tutorialLogoView)
	{
		context.tutorialLogoView->StartReturnAnimation(
			context.tutorialLogoView->GetMoveDuration());
	}
	if (context.tutorialLogo2View)
	{
		context.tutorialLogo2View->StartReturnAnimation(
			context.tutorialLogo2View->GetMoveDuration());
	}
	if (context.startPlayingView)
	{
		context.startPlayingView->Start();
	}
}

bool StartPlayingPhase::OnUpdate(GameFlowContext& context)
{
	const float deltaTime = (std::max)(GameEngine::FpsCounter::deltaTime, 0.0f);
	remainingTime_ = (std::max)(remainingTime_ - deltaTime, 0.0f);
	if (context.startPlayingView)
	{
		context.startPlayingView->Update(deltaTime);
	}
	return remainingTime_ <= 0.0f;
}

void StartPlayingPhase::OnExit(GameFlowContext& context)
{
	if (context.startPlayingView)
	{
		context.startPlayingView->Stop();
	}
}
