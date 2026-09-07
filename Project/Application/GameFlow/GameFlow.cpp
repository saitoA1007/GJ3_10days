#include "GameFlow.h"

#include <algorithm>
#include <cassert>

#include "FPSCounter.h"
#include "ImGuiManager.h"

#include "Application/Enemy/EnemyManager.h"
#include "Application/Energy/EnergySpawner.h"
#include "Application/LockOn/LockOnController.h"
#include "Application/Rocket/Rocket.h"
#include "Application/Unit/UnitManager.h"

#include "OpeningPhase.h"
#include "PlayingPhase.h"
#include "StartPlayingPhase.h"
#include "TutorialPhase.h"
#include "LaunchPhase.h"
#include "ResultPhase.h"

using namespace GameEngine;

GameFlow::GameFlow(const GameFlowContext& context, const GameFlowSettings& settings)
	: context_(context), settings_(settings)
{
	context_.settings = &settings_;

	// DebugParameterの登録
	debugParameter_ = std::make_unique<GameEngine::DebugParameter>("GameFlow");
	debugParameter_->Register("OpeningDuration", settings_.openingDuration, 0, "Time");
	debugParameter_->Register("StartPlayingDuration", settings_.startPlayingDuration, 1, "Time");
	debugParameter_->Register("GameDuration", settings_.gameDuration, 2, "Time");
	debugParameter_->Register("LaunchDuration", settings_.launchDuration, 3, "Time");
	debugParameter_->Register("EnergyPositionXZ", settings_.tutorialEnergyPositionXZ, 0, "Tutorial");
	debugParameter_->Register("MediumEnergyPositionXZ", settings_.tutorialMediumEnergyPositionXZ, 1, "Tutorial");
	debugParameter_->Register("RequiredHoldDuration", settings_.tutorialRequiredHoldDuration, 2, "Tutorial");
	debugParameter_->Register("EnemyPositionXZ", settings_.tutorialEnemyPositionXZ, 3, "Tutorial");
	debugParameter_->Register("LockOnEnemyPositionXZ", settings_.tutorialLockOnEnemyPositionXZ, 4, "Tutorial");
	debugParameter_->Register("EnemyHoldPositionXZ", settings_.tutorialEnemyHoldPositionXZ, 5, "Tutorial");
	debugParameter_->Register("EnemyRequiredHoldDuration", settings_.tutorialEnemyRequiredHoldDuration, 6, "Tutorial");
	debugParameter_->Apply();

	SetUpdateOrder(0);
}

void GameFlow::Initialize()
{
	ApplyDebugParameters();
	currentPhaseIndex_ = 0;
	debugPaused_ = false;

	// パラメータを反映した状態でフェーズシーケンスを構築
	BuildPhases();

	if (!phases_.empty())
	{
		phases_[0]->OnEnter(context_);
	}
	ApplyGameplayState();
}

void GameFlow::BuildPhases()
{
	phases_.clear();

	// OPフェーズ 
	phases_.push_back(std::make_unique<OpeningPhase>());

	// チュートリアルフェーズ
	phases_.push_back(std::make_unique<TutorialPhase>());

	// プレイ開始演出フェーズ
	phases_.push_back(std::make_unique<StartPlayingPhase>());

	// プレイフェーズ 
	phases_.push_back(std::make_unique<PlayingPhase>());

	// 打ち上げ演出フェーズ
	phases_.push_back(std::make_unique<LaunchPhase>());

	// リザルト画面フェーズ
	phases_.push_back(std::make_unique<ResultPhase>());
}

void GameFlow::Update()
{
	ApplyDebugParameters();

	if (debugPaused_ || currentPhaseIndex_ >= phases_.size())
	{
		return;
	}

	IGamePhase* current = phases_[currentPhaseIndex_].get();
	if (current->OnUpdate(context_))
	{
		AdvanceToNextPhase();
	}
}

void GameFlow::ApplyDebugParameters()
{
	if (debugParameter_)
	{
		debugParameter_->ApplyIfDirty();
	}
	settings_.openingDuration = (std::max)(settings_.openingDuration, 0.0f);
	settings_.startPlayingDuration = (std::max)(settings_.startPlayingDuration, 0.0f);
	settings_.gameDuration = (std::max)(settings_.gameDuration, 0.1f);
	settings_.launchDuration = (std::max)(settings_.launchDuration, 0.0f);
	settings_.tutorialRequiredHoldDuration =
		(std::max)(settings_.tutorialRequiredHoldDuration, 0.0f);
	settings_.tutorialEnemyRequiredHoldDuration =
		(std::max)(settings_.tutorialEnemyRequiredHoldDuration, 0.0f);
}

void GameFlow::BeginTutorialChargeEnergyStep()
{
	auto* tutorialPhase = dynamic_cast<TutorialPhase*>(GetCurrentPhase());
	if (tutorialPhase)
	{
		tutorialPhase->BeginChargeEnergyStep(context_);
	}
}

void GameFlow::BeginTutorialEnemyCollisionStep()
{
	auto* tutorialPhase = dynamic_cast<TutorialPhase*>(GetCurrentPhase());
	if (tutorialPhase)
	{
		tutorialPhase->BeginEnemyCollisionStep(context_);
	}
}

void GameFlow::BeginTutorialEnemyLockOnStep()
{
	auto* tutorialPhase = dynamic_cast<TutorialPhase*>(GetCurrentPhase());
	if (tutorialPhase)
	{
		tutorialPhase->BeginEnemyLockOnStep(context_);
	}
}

void GameFlow::BeginTutorialEnemyHoldStep()
{
	auto* tutorialPhase = dynamic_cast<TutorialPhase*>(GetCurrentPhase());
	if (tutorialPhase)
	{
		tutorialPhase->BeginEnemyHoldStep(context_);
	}
}

void GameFlow::AdvanceToNextPhase()
{
	if (currentPhaseIndex_ >= phases_.size()) return;

	phases_[currentPhaseIndex_]->OnExit(context_);
	currentPhaseIndex_++;

	if (currentPhaseIndex_ < phases_.size())
	{
		phases_[currentPhaseIndex_]->OnEnter(context_);
		ApplyGameplayState();
	}
}

void GameFlow::ChangePhase(size_t index)
{
	if (index >= phases_.size() || index == currentPhaseIndex_) return;

	if (currentPhaseIndex_ < phases_.size())
	{
		phases_[currentPhaseIndex_]->OnExit(context_);
	}

	currentPhaseIndex_ = index;
	phases_[currentPhaseIndex_]->OnEnter(context_);
	ApplyGameplayState();
}

void GameFlow::ApplyGameplayState()
{
	IGamePhase* current = GetCurrentPhase();
	const bool enabled = current ? (current->IsGameplayEnabled() && !debugPaused_) : false;
	const bool autoSpawnEnabled = current ? (current->IsAutoSpawnEnabled() && !debugPaused_) : false;

	if (context_.energySpawner)
	{
		context_.energySpawner->SetGameplayEnabled(enabled);
		context_.energySpawner->SetAutoSpawnEnabled(autoSpawnEnabled);
	}
	if (context_.enemyManager)
	{
		context_.enemyManager->SetGameplayEnabled(enabled);
		context_.enemyManager->SetAutoSpawnEnabled(autoSpawnEnabled);
	}
	if (context_.unitManager)      context_.unitManager->SetGameplayEnabled(enabled);
	if (context_.lockOnController) context_.lockOnController->SetGameplayEnabled(enabled);
}

void GameFlow::DebugUpdate()
{
	ApplyDebugParameters();
#ifdef USE_IMGUI
	if (ImGui::Begin("Game Flow"))
	{
		IGamePhase* current = GetCurrentPhase();
		ImGui::Text("Phase [%zu / %zu]: %s",
			currentPhaseIndex_ + 1, phases_.size(),
			current ? current->GetName() : "Finished");

		if (ImGui::Button("Skip Phase")) AdvanceToNextPhase();
		ImGui::SameLine();
		if (ImGui::Button(debugPaused_ ? "Resume" : "Pause"))
		{
			debugPaused_ = !debugPaused_;
			ApplyGameplayState();
		}

		ImGui::Separator();
		ImGui::Text("Phase List:");
		for (size_t i = 0; i < phases_.size(); ++i)
		{
			if (i == currentPhaseIndex_)
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "> %zu: %s", i, phases_[i]->GetName());
			else
				ImGui::Text("  %zu: %s", i, phases_[i]->GetName());

			ImGui::SameLine();
			char label[32];
			sprintf_s(label, "Jump##%zu", i);
			if (ImGui::Button(label)) ChangePhase(i);
		}
	}
	ImGui::End();
#endif
}

