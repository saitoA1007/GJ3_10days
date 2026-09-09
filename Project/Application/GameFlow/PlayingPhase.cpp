#pragma once
#include "PlayingPhase.h"
#include <algorithm>
#include "AudioManager.h"
#include "FPSCounter.h"
#include "Application/HalfTime/HalfTimeView.h"
#include "Application/Energy/EnergySpawner.h"
#include "Application/Enemy/EnemyManager.h"
#include "Application/Rocket/Rocket.h"
#include "Application/UI/TimeUI.h"
#include "Application/UI/UnitCountUI.h"

using namespace GameEngine;

namespace
{
	constexpr const char* kFirstHalfBgmName = "gameFirstHalfBGM.mp3";
	constexpr const char* kSecondHalfBgmName = "gameSecondHalfBGM.mp3";
	constexpr float kBgmVolume = 1.0f;
	constexpr float kSecondHalfStartProgress = 0.5f;
}

PlayingPhase::PlayingPhase() {};

void PlayingPhase::OnEnter(GameFlowContext& context)
{
	auto& audioManager = AudioManager::GetInstance();
	const uint32_t firstHalfBgmHandle = audioManager.GetHandleByName(kFirstHalfBgmName);
	// デバッグ操作などで再入場してもBGMが重ならないよう、両方を停止してから開始する。
	audioManager.Stop(firstHalfBgmHandle);
	audioManager.Stop(audioManager.GetHandleByName(kSecondHalfBgmName));
	audioManager.Play(firstHalfBgmHandle, kBgmVolume, true);
	hasStartedSecondHalfBgm_ = false;
	hasStartedHalfTimeView_ = false;
	if (context.halfTimeView)
	{
		context.halfTimeView->Reset();
	}

	if (context.settings)
	{
		remainingTime_ = context.settings->gameDuration;
	}
	if (context.timeUI)
	{
		context.timeUI->SetUnit(0.0f);
		context.timeUI->SetActive(true);
	}
	if (context.energySpawner)
	{
		context.energySpawner->BeginPlayingTimeline();
	}
	if (context.enemyManager)
	{
		context.enemyManager->BeginPlayingTimeline();
	}
	if (context.unitCountUI) {
		context.unitCountUI->SetActive(true);
	}
}

bool PlayingPhase::OnUpdate(GameFlowContext& context)
{
	remainingTime_ = (std::max)(remainingTime_ - FpsCounter::deltaTime, 0.0f);

	if (context.settings)
	{
		const float progress = 1.0f - remainingTime_ / context.settings->gameDuration;
		if (context.timeUI)
		{
			context.timeUI->SetUnit((std::clamp)(progress, 0.0f, 1.0f));
		}

		if (!hasStartedSecondHalfBgm_ && progress >= kSecondHalfStartProgress)
		{
			auto& audioManager = AudioManager::GetInstance();
			audioManager.Stop(audioManager.GetHandleByName(kFirstHalfBgmName));
			audioManager.Play(
				audioManager.GetHandleByName(kSecondHalfBgmName),
				kBgmVolume,
				true);
			hasStartedSecondHalfBgm_ = true;
		}

		if (!hasStartedHalfTimeView_ && progress >= kSecondHalfStartProgress)
		{
			if (context.halfTimeView)
			{
				context.halfTimeView->Start();
			}
			hasStartedHalfTimeView_ = true;
		}
	}

	return remainingTime_ <= 0.0f; // 時間切れで終了
}

void PlayingPhase::OnExit(GameFlowContext& context)
{
	if (context.energySpawner)
	{
		context.energySpawner->EndPlayingTimeline();
	}
	if (context.enemyManager)
	{
		context.enemyManager->EndPlayingTimeline();
	}

	auto& audioManager = AudioManager::GetInstance();
	audioManager.Stop(audioManager.GetHandleByName(kFirstHalfBgmName));
	audioManager.Stop(audioManager.GetHandleByName(kSecondHalfBgmName));
	if (context.halfTimeView)
	{
		context.halfTimeView->Stop();
	}

	if (context.timeUI)
	{
		context.timeUI->SetUnit(1.0f);
		context.timeUI->SetActive(false);
	}

	if (context.rocket)
	{
		context.finalEnergy = context.rocket->GetEnergy(); 
	}

	if (context.unitCountUI) {
		context.unitCountUI->SetActive(false);
	}
}
