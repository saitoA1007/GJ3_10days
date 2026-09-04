#pragma once

#include <cstdint>
#include <memory>

#include "ModelComponent.h"
#include "Vector3.h"
#include "Vector4.h"

namespace GameEngine {
	class Model;
	class RenderQueue;
}

namespace Prototype {
	class EnergyPickup;
	class Rocket;

	enum class UnitState : uint8_t {
		Stored,
		MovingToEnergy,
		ReturningToRocket,
	};

	struct UnitSettings {
		Vector3 launchOffset = { 0.0f, 0.0f, 0.0f };
		Vector3 scale = { 0.7f, 0.7f, 0.7f };
		Vector3 carryOffset = { 0.0f, 1.45f, 0.0f };
		float normalSpeed = 1.5f;
		float boostedSpeed = 5.0f;
		float pickupRadius = 0.45f;
		float deliveryRadius = 1.6f;
		float staminaDrainPerSecond = 2.0f;
		float distanceDrainRate = 0.08f;
		Vector4 normalColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 staminaColor = { 0.25f, 0.85f, 1.0f, 1.0f };
	};

	/// <summary>
	/// 指定されたエネルギーを回収してロケットへ帰還するユニット
	/// </summary>
	class Unit final {
	public:
		Unit(GameEngine::Model* model, Rocket* rocket, const UnitSettings* settings);

		void Initialize();
		void Update(float deltaTime);
		void RefreshVisual();
		void Draw(GameEngine::RenderQueue* renderQueue);

		bool DispatchToEnergy(EnergyPickup* target, int32_t requestedEnergy);
		void Recall();

		bool IsAvailable() const { return state_ == UnitState::Stored; }
		UnitState GetState() const { return state_; }
		const Vector3& GetPosition() const { return position_; }
		float GetStamina() const { return stamina_; }
		EnergyPickup* GetTargetEnergy() const { return targetEnergy_; }

	private:
		void UpdateMovingToEnergy(float deltaTime);
		void UpdateReturningToRocket(float deltaTime);
		void MoveTowards(const Vector3& target, float deltaTime);
		void ConsumeStamina(float deltaTime);
		float DistanceSquaredXZ(const Vector3& a, const Vector3& b) const;
		void SyncModel();

		Rocket* rocket_ = nullptr;
		const UnitSettings* settings_ = nullptr;
		std::unique_ptr<GameEngine::ModelComponent> modelComponent_;
		UnitState state_ = UnitState::Stored;
		EnergyPickup* targetEnergy_ = nullptr;
		Vector3 position_ = {};
		float stamina_ = 0.0f;
	};
}
