#include "PrototypeEnemy.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "RenderQueue.h"

#include "Application/Prototype/Energy/PrototypeEnergyPickup.h"
#include "Application/Prototype/Energy/PrototypeEnergySpawner.h"
#include "Application/Prototype/Rocket/PrototypeRocket.h"
#include "Application/Prototype/Unit/PrototypeUnit.h"
#include "Application/Prototype/Unit/PrototypeUnitManager.h"

using namespace GameEngine;

namespace Prototype {

	Enemy::Enemy(
		Model* model,
		Rocket* rocket,
		EnergySpawner* energySpawner,
		UnitManager* unitManager,
		const EnemySettings* settings)
		: rocket_(rocket), energySpawner_(energySpawner), unitManager_(unitManager), settings_(settings) {
		assert(model != nullptr && "Prototype enemy requires enemy.obj");
		assert(rocket_ != nullptr && "Prototype enemy requires a rocket");
		assert(energySpawner_ != nullptr && "Prototype enemy requires an energy spawner");
		assert(unitManager_ != nullptr && "Prototype enemy requires a unit manager");
		assert(settings_ != nullptr && "Prototype enemy requires settings");

		modelComponent_ = std::make_unique<ModelComponent>(model);
		modelComponent_->materialData_->enableLighting = true;
	}

	void Enemy::Spawn(const Vector3& position) {
		position_ = position;
		targetUnit_ = nullptr;
		isActive_ = true;
		isReservedForAttack_ = false;
		isHighlighted_ = false;
		SyncModel();
	}

	void Enemy::Reset() {
		targetUnit_ = nullptr;
		isActive_ = false;
		isReservedForAttack_ = false;
		isHighlighted_ = false;
	}

	void Enemy::Update(float deltaTime) {
		if (!isActive_) {
			return;
		}

		UpdateTarget();
		const Vector3 targetPosition = targetUnit_
			? targetUnit_->GetPosition()
			: rocket_->GetPosition();
		MoveTowards(targetPosition, deltaTime);

		// 運搬ユニットを倒したフレームは、その場から次フレームにロケット追跡へ戻る。
		if (TryHitTargetUnit()) {
			SyncModel();
			return;
		}
		if (TryHitRocket()) {
			return;
		}

		SyncModel();
	}

	void Enemy::RefreshVisual() {
		if (isActive_) {
			SyncModel();
		}
	}

	void Enemy::Draw(RenderQueue* renderQueue) {
		if (isActive_) {
			modelComponent_->DrawRaytracing(renderQueue);
		}
	}

	float Enemy::GetDisplayScale() const {
		return (std::max)({ settings_->scale.x, settings_->scale.y, settings_->scale.z });
	}

	bool Enemy::TryReserveForAttack() {
		if (!IsTargetable()) {
			return false;
		}

		isReservedForAttack_ = true;
		isHighlighted_ = false;
		SyncModel();
		return true;
	}

	void Enemy::CancelAttackReservation() {
		if (!isActive_) {
			return;
		}

		isReservedForAttack_ = false;
		SyncModel();
	}

	EnergyPickup* Enemy::DefeatAndDropSmallEnergy() {
		if (!isActive_) {
			return nullptr;
		}

		const Vector3 dropPosition = position_;
		Reset();
		return energySpawner_->SpawnOnGround(EnergySize::Small, dropPosition);
	}

	void Enemy::SetHighlighted(bool highlighted) {
		if (!isActive_ || isHighlighted_ == highlighted) {
			return;
		}

		isHighlighted_ = highlighted;
		SyncModel();
	}

	void Enemy::UpdateTarget() {
		if (targetUnit_ && targetUnit_->IsCarryingEnergy()) {
			return;
		}

		targetUnit_ = unitManager_->FindNearestCarryingUnit(position_, settings_->searchRadius);
	}

	void Enemy::MoveTowards(const Vector3& target, float deltaTime) {
		Vector3 direction = target - position_;
		direction.y = 0.0f;
		const float distance = direction.Length();
		if (distance <= 0.0001f) {
			return;
		}

		direction.Normalize();
		const float moveDistance = (std::min)(
			settings_->moveSpeed * (std::max)(deltaTime, 0.0f),
			distance);
		position_ += direction * moveDistance;
		modelComponent_->worldTransform_.transform_.rotate.y = std::atan2(direction.x, direction.z);
	}

	bool Enemy::TryHitTargetUnit() {
		if (!targetUnit_ || !targetUnit_->IsCarryingEnergy()) {
			targetUnit_ = nullptr;
			return false;
		}

		const float hitDistance = settings_->collisionRadius + targetUnit_->GetCollisionRadius();
		if (DistanceSquaredXZ(position_, targetUnit_->GetPosition()) > hitDistance * hitDistance) {
			return false;
		}

		const bool defeated = targetUnit_->DefeatAndDropEnergy();
		targetUnit_ = nullptr;
		return defeated;
	}

	bool Enemy::TryHitRocket() {
		const float hitDistance = settings_->collisionRadius + rocket_->GetSettings().colliderRadius;
		if (DistanceSquaredXZ(position_, rocket_->GetPosition()) > hitDistance * hitDistance) {
			return false;
		}

		rocket_->ReceiveEnemyHit();
		Reset();
		return true;
	}

	float Enemy::DistanceSquaredXZ(const Vector3& a, const Vector3& b) const {
		const float x = a.x - b.x;
		const float z = a.z - b.z;
		return x * x + z * z;
	}

	void Enemy::SyncModel() {
		modelComponent_->worldTransform_.transform_.scale = settings_->scale;
		modelComponent_->worldTransform_.transform_.translate = position_;
		Vector4 color = settings_->color;
		if (isHighlighted_) {
			color.x = color.x + (1.0f - color.x) * 0.65f;
			color.y = color.y + (1.0f - color.y) * 0.65f;
			color.z = color.z + (1.0f - color.z) * 0.65f;
		}
		modelComponent_->materialData_->color = color;
		modelComponent_->Update();
	}
}
