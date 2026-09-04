#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#include "Collider.h"
#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"
#include "Vector3.h"

#include "RocketEnergy.h"

namespace Prototype {

	/// <summary>
	/// ロケットの配置とエネルギー消費に関する調整値
	/// </summary>
	struct RocketSettings {
		Vector3 position = { 0.0f, 0.25f, 0.0f };
		Vector3 scale = { 1.0f, 1.0f, 1.0f };
		float colliderRadius = 1.5f;
		float colliderOffsetY = 1.75f;
		int32_t initialEnergy = 0;
		int32_t enemyHitLoss = 10;
		int32_t debugEnergyAmount = 10;
	};

	/// <summary>
	/// フィールド中央に配置するプロトタイプ用ロケット
	/// </summary>
	class Rocket final : public GameEngine::IGameObject {
	public:
		using EnergyChangedCallback = std::function<void(const EnergyChange&)>;

		explicit Rocket(GameEngine::Model* model, const RocketSettings& settings = {});
		~Rocket() override = default;

		void Initialize() override;
		void Update() override;
		void DebugUpdate() override;
		void Draw() override;

		EnergyChange DepositEnergy(int32_t amount);
		EnergyChange AllocateEnergyToUnit(int32_t requestedAmount);
		EnergyChange ReceiveEnemyHit();
		void ResetEnergy();

		int32_t GetEnergy() const { return energy_.GetCurrent(); }
		bool HasEnergy() const { return !energy_.IsEmpty(); }
		const Vector3& GetPosition() const { return settings_.position; }
		const RocketSettings& GetSettings() const { return settings_; }
		GameEngine::SphereCollider& GetCollider() { return collider_; }

		void SetOnEnergyChanged(EnergyChangedCallback callback) {
			onEnergyChanged_ = std::move(callback);
		}

	private:
		void ApplyDebugParameters();
		void SanitizeSettings();
		void SyncComponents();
		void NotifyEnergyChanged(const EnergyChange& change);
		void OnCollisionEnter(const GameEngine::CollisionResult& result);
		void DrawDebugWindow();

		RocketSettings settings_;
		RocketEnergy energy_;
		std::unique_ptr<GameEngine::ModelComponent> modelComponent_;
		GameEngine::SphereCollider collider_;
		std::unique_ptr<GameEngine::DebugParameter> debugParameter_;
		EnergyChangedCallback onEnergyChanged_;
	};
}
