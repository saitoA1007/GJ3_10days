#pragma once

#include <cstdint>
#include <memory>

#include "DebugParameter.h"
#include "IGameObject.h"

namespace Prototype {
	class EnemyManager;
	class EnergySpawner;
	class LockOnController;
	class Rocket;
	class UnitManager;

	enum class GamePhase : uint8_t {
		Ready,
		Playing,
		TimeUp,
		Launching,
		Result,
	};

	struct GameFlowSettings {
		float gameDuration = 60.0f;
		float initialDelay = 3.0f;
	};

	/// <summary>
	/// プロトタイプの制限時間とゲームプレイ可否を一括管理する
	/// </summary>
	class GameFlowController final : public GameEngine::IGameObject {
	public:
		GameFlowController(
			Rocket* rocket,
			EnergySpawner* energySpawner,
			EnemyManager* enemyManager,
			UnitManager* unitManager,
			LockOnController* lockOnController,
			const GameFlowSettings& settings = {});
		~GameFlowController() override = default;

		void Initialize() override;
		void Update() override;
		void DebugUpdate() override;
		void Draw() override {}

		void ForceTimeUp();
		GamePhase GetPhase() const { return phase_; }
		bool IsPlaying() const { return phase_ == GamePhase::Playing; }
		bool IsTimeUp() const { return phase_ == GamePhase::TimeUp; }
		bool IsDebugPaused() const { return debugPaused_; }
		float GetRemainingTime() const { return remainingTime_; }
		float GetRemainingRatio() const;
		int32_t GetFinalEnergy() const { return finalEnergy_; }
		const GameFlowSettings& GetSettings() const { return settings_; }

	private:
		void ApplyDebugParameters();
		void SanitizeSettings();
		void StartPlaying();
		void FinishPlaying();
		void ApplyGameplayState();
		const char* GetPhaseName() const;
		void DrawDebugWindow();

		Rocket* rocket_ = nullptr;
		EnergySpawner* energySpawner_ = nullptr;
		EnemyManager* enemyManager_ = nullptr;
		UnitManager* unitManager_ = nullptr;
		LockOnController* lockOnController_ = nullptr;
		GameFlowSettings settings_;
		std::unique_ptr<GameEngine::DebugParameter> debugParameter_;
		GamePhase phase_ = GamePhase::Ready;
		float initialDelayRemaining_ = 0.0f;
		float remainingTime_ = 0.0f;
		int32_t finalEnergy_ = 0;
		bool debugPaused_ = false;
	};
}
