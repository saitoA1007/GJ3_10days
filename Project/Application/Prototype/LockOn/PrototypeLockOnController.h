#pragma once

#include <cstdint>
#include <memory>

#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"
#include "Vector3.h"
#include "Vector4.h"

namespace GameEngine {
	class Camera;
	class DebugRenderer;
	class Input;
	class InputCommand;
}

namespace Prototype {
	class Enemy;
	class EnemyManager;
	class EnergyPickup;
	class EnergySpawner;
	class Field;
	class Rocket;
	class UnitManager;

	struct LockOnSettings {
		float cursorSpeed = 12.0f;
		float selectionRadius = 1.5f;
		float groundHeight = 0.35f;
		float fieldEdgeMargin = 0.5f;
		float maxLockOnSeconds = 3.0f;
		float chargeStartSeconds = 0.2f;
		int32_t maxChargeEnergyCost = 30;
		float mouseMoveThreshold = 0.01f;
		Vector3 cursorModelScale = { 1.0f, 1.0f, 1.0f };
		float cursorModelHeightOffset = 0.06f;
		Vector4 cursorColor = { 0.25f, 1.00f, 0.45f, 1.0f };
		Vector4 targetColor = { 1.00f, 0.95f, 0.25f, 1.0f };
		Vector4 chargeColor = { 1.00f, 0.35f, 0.20f, 1.0f };
	};

	/// <summary>
	/// 2D入力をXZ平面上のカーソルへ変換し、長押しロックオンでユニットを出撃させる
	/// </summary>
	class LockOnController final : public GameEngine::IGameObject {
	public:
		LockOnController(
			GameEngine::Input* input,
			GameEngine::InputCommand* inputCommand,
			GameEngine::Camera* camera,
			GameEngine::Model* cursorModel,
			GameEngine::DebugRenderer* debugRenderer,
			Field* field,
			Rocket* rocket,
			EnergySpawner* energySpawner,
			EnemyManager* enemyManager,
			UnitManager* unitManager,
			const LockOnSettings& settings = {});
		~LockOnController() override = default;

		void Initialize() override;
		void Update() override;
		void DebugUpdate() override;
		void Draw() override;
		void SetGameplayEnabled(bool enabled);
		bool IsGameplayEnabled() const { return gameplayEnabled_; }

		const Vector3& GetCursorPosition() const { return cursorPosition_; }
		EnergyPickup* GetSelectedEnergy() const { return selectedEnergy_; }
		Enemy* GetSelectedEnemy() const { return selectedEnemy_; }
		float GetLockOnSeconds() const { return lockOnSeconds_; }
		bool IsCharging() const { return isCharging_; }

	private:
		void ApplyDebugParameters();
		void SanitizeSettings();
		void UpdateCursor(float deltaTime);
		bool TrySetCursorFromMouse();
		void ClampCursorToField();
		void SyncCursorModel();
		void UpdateSelection();
		void StartLockOn();
		void UpdateLockOn(float deltaTime);
		void CompleteLockOn();
		void CancelLockOn();
		float CalculateChargeRatio() const;
		int32_t CalculateRequestedEnergy() const;
		bool HasValidSelection() const;
		void SetSelection(EnergyPickup* energy, Enemy* enemy);
		void DrawLockOnGuide();
		void DrawDebugWindow();

		GameEngine::Input* input_ = nullptr;
		GameEngine::InputCommand* inputCommand_ = nullptr;
		GameEngine::Camera* camera_ = nullptr;
		std::unique_ptr<GameEngine::ModelComponent> cursorModel_;
		GameEngine::DebugRenderer* debugRenderer_ = nullptr;
		Field* field_ = nullptr;
		Rocket* rocket_ = nullptr;
		EnergySpawner* energySpawner_ = nullptr;
		EnemyManager* enemyManager_ = nullptr;
		UnitManager* unitManager_ = nullptr;
		LockOnSettings settings_;
		std::unique_ptr<GameEngine::DebugParameter> debugParameter_;
		Vector3 cursorPosition_ = {};
		EnergyPickup* selectedEnergy_ = nullptr;
		Enemy* selectedEnemy_ = nullptr;
		float lockOnSeconds_ = 0.0f;
		bool isCharging_ = false;
		bool gameplayEnabled_ = true;
	};
}
