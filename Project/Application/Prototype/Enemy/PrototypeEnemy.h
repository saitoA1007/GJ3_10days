#pragma once

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
	class EnergySpawner;
	class Rocket;
	class Unit;
	class UnitManager;

	struct EnemySettings {
		Vector3 scale = { 1.0f, 1.0f, 1.0f };
		Vector4 color = { 1.0f, 0.35f, 0.35f, 1.0f };
		float groundHeight = 0.25f;
		float moveSpeed = 2.0f;
		float searchRadius = 8.0f;
		float collisionRadius = 0.8f;
	};

	/// <summary>
	/// ロケットまたは索敵した運搬ユニットへ向かうプロトタイプ用の敵
	/// </summary>
	class Enemy final {
	public:
		Enemy(
			GameEngine::Model* model,
			Rocket* rocket,
			EnergySpawner* energySpawner,
			UnitManager* unitManager,
			const EnemySettings* settings);

		void Spawn(const Vector3& position);
		void Reset();
		void Update(float deltaTime);
		void RefreshVisual();
		void Draw(GameEngine::RenderQueue* renderQueue);

		bool IsActive() const { return isActive_; }
		bool IsTargetable() const { return isActive_ && !isReservedForAttack_; }
		bool IsTargetingCarrier() const { return targetUnit_ != nullptr; }
		const Vector3& GetPosition() const { return position_; }
		float GetCollisionRadius() const { return settings_->collisionRadius; }
		float GetDisplayScale() const;

		bool TryReserveForAttack();
		void CancelAttackReservation();
		EnergyPickup* DefeatAndDropSmallEnergy();
		void SetHighlighted(bool highlighted);

	private:
		void UpdateTarget();
		void MoveTowards(const Vector3& target, float deltaTime);
		bool TryHitTargetUnit();
		bool TryHitRocket();
		float DistanceSquaredXZ(const Vector3& a, const Vector3& b) const;
		void SyncModel();

		Rocket* rocket_ = nullptr;
		EnergySpawner* energySpawner_ = nullptr;
		UnitManager* unitManager_ = nullptr;
		const EnemySettings* settings_ = nullptr;
		std::unique_ptr<GameEngine::ModelComponent> modelComponent_;
		Unit* targetUnit_ = nullptr;
		Vector3 position_ = {};
		bool isActive_ = false;
		bool isReservedForAttack_ = false;
		bool isHighlighted_ = false;
	};
}
