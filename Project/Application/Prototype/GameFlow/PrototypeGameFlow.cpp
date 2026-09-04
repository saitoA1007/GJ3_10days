#include "PrototypeGameFlow.h"

#include <algorithm>
#include <cassert>

#include "FPSCounter.h"
#include "ImGuiManager.h"

#include "Application/Prototype/Enemy/PrototypeEnemyManager.h"
#include "Application/Prototype/Energy/PrototypeEnergySpawner.h"
#include "Application/Prototype/LockOn/PrototypeLockOnController.h"
#include "Application/Prototype/Rocket/PrototypeRocket.h"
#include "Application/Prototype/Unit/PrototypeUnitManager.h"

using namespace GameEngine;

namespace Prototype {

	GameFlowController::GameFlowController(
		Rocket* rocket,
		EnergySpawner* energySpawner,
		EnemyManager* enemyManager,
		UnitManager* unitManager,
		LockOnController* lockOnController,
		const GameFlowSettings& settings)
		: rocket_(rocket),
		energySpawner_(energySpawner),
		enemyManager_(enemyManager),
		unitManager_(unitManager),
		lockOnController_(lockOnController),
		settings_(settings) {
		assert(rocket_ != nullptr && "Prototype game flow requires a rocket");
		assert(energySpawner_ != nullptr && "Prototype game flow requires an energy spawner");
		assert(enemyManager_ != nullptr && "Prototype game flow requires an enemy manager");
		assert(unitManager_ != nullptr && "Prototype game flow requires a unit manager");
		assert(lockOnController_ != nullptr && "Prototype game flow requires a lock-on controller");

		debugParameter_ = std::make_unique<DebugParameter>("PrototypeGameFlow");
		debugParameter_->Register("GameDuration", settings_.gameDuration, 0, "Time");
		debugParameter_->Register("InitialDelay", settings_.initialDelay, 1, "Time");
		debugParameter_->Apply();
		SanitizeSettings();
		SetUpdateOrder(0);
	}

	void GameFlowController::Initialize() {
		ApplyDebugParameters();
		initialDelayRemaining_ = settings_.initialDelay;
		remainingTime_ = settings_.gameDuration;
		finalEnergy_ = 0;
		debugPaused_ = false;
		phase_ = initialDelayRemaining_ > 0.0f ? GamePhase::Ready : GamePhase::Playing;
		ApplyGameplayState();
	}

	void GameFlowController::Update() {
		ApplyDebugParameters();
		if (debugPaused_) {
			return;
		}

		const float deltaTime = (std::max)(FpsCounter::gameDeltaTime, 0.0f);
		switch (phase_) {
		case GamePhase::Ready:
			initialDelayRemaining_ = (std::max)(initialDelayRemaining_ - deltaTime, 0.0f);
			if (initialDelayRemaining_ <= 0.0f) {
				StartPlaying();
			}
			break;
		case GamePhase::Playing:
			remainingTime_ = (std::max)(remainingTime_ - deltaTime, 0.0f);
			if (remainingTime_ <= 0.0f) {
				FinishPlaying();
			}
			break;
		case GamePhase::TimeUp:
		case GamePhase::Launching:
		case GamePhase::Result:
			break;
		}
	}

	void GameFlowController::DebugUpdate() {
		ApplyDebugParameters();
		DrawDebugWindow();
	}

	void GameFlowController::ForceTimeUp() {
		if (phase_ == GamePhase::Ready || phase_ == GamePhase::Playing) {
			FinishPlaying();
		}
	}

	float GameFlowController::GetRemainingRatio() const {
		if (settings_.gameDuration <= 0.0f) {
			return 0.0f;
		}
		return (std::clamp)(remainingTime_ / settings_.gameDuration, 0.0f, 1.0f);
	}

	void GameFlowController::ApplyDebugParameters() {
		debugParameter_->ApplyIfDirty();
		SanitizeSettings();
	}

	void GameFlowController::SanitizeSettings() {
		settings_.gameDuration = (std::max)(settings_.gameDuration, 0.1f);
		settings_.initialDelay = (std::max)(settings_.initialDelay, 0.0f);
	}

	void GameFlowController::StartPlaying() {
		phase_ = GamePhase::Playing;
		ApplyGameplayState();
	}

	void GameFlowController::FinishPlaying() {
		remainingTime_ = 0.0f;
		finalEnergy_ = rocket_->GetEnergy();
		phase_ = GamePhase::TimeUp;
		debugPaused_ = false;
		ApplyGameplayState();
	}

	void GameFlowController::ApplyGameplayState() {
		const bool enabled = phase_ == GamePhase::Playing && !debugPaused_;
		energySpawner_->SetGameplayEnabled(enabled);
		enemyManager_->SetGameplayEnabled(enabled);
		unitManager_->SetGameplayEnabled(enabled);
		lockOnController_->SetGameplayEnabled(enabled);
	}

	const char* GameFlowController::GetPhaseName() const {
		switch (phase_) {
		case GamePhase::Ready:
			return "Ready";
		case GamePhase::Playing:
			return "Playing";
		case GamePhase::TimeUp:
			return "TimeUp";
		case GamePhase::Launching:
			return "Launching";
		case GamePhase::Result:
			return "Result";
		default:
			return "Unknown";
		}
	}

	void GameFlowController::DrawDebugWindow() {
#ifdef USE_IMGUI
		if (!ImGui::Begin("Prototype Game Flow")) {
			ImGui::End();
			return;
		}

		ImGui::Text("Phase: %s", GetPhaseName());
		if (phase_ == GamePhase::Ready) {
			ImGui::Text("Starts In: %.2f sec", initialDelayRemaining_);
		}
		ImGui::Text("Remaining: %.2f / %.2f sec", remainingTime_, settings_.gameDuration);
		if (phase_ == GamePhase::TimeUp) {
			ImGui::Text("Final Energy: %d", finalEnergy_);
		}

		if (phase_ == GamePhase::Ready || phase_ == GamePhase::Playing) {
			if (ImGui::Button(debugPaused_ ? "Resume" : "Pause")) {
				debugPaused_ = !debugPaused_;
				ApplyGameplayState();
			}
			ImGui::SameLine();
			if (ImGui::Button("Force Time Up")) {
				ForceTimeUp();
			}
		}

		ImGui::End();
#endif
	}
}
