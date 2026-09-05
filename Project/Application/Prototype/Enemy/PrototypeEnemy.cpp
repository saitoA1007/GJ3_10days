#include "PrototypeEnemy.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "RenderQueue.h"

#include "Application/Prototype/Energy/PrototypeEnergyPickup.h"
#include "Application/Prototype/Energy/PrototypeEnergySpawner.h"
#include "Application/Prototype/Field/PrototypeField.h"
#include "Application/Prototype/Rocket/PrototypeRocket.h"
#include "Application/Prototype/Unit/PrototypeUnit.h"
#include "Application/Prototype/Unit/PrototypeUnitManager.h"

using namespace GameEngine;

namespace Prototype {

	Enemy::Enemy(
		Model* model,
		Field* field,
		Rocket* rocket,
		EnergySpawner* energySpawner,
		UnitManager* unitManager,
		const EnemySettings* settings)
		: field_(field), rocket_(rocket), energySpawner_(energySpawner), unitManager_(unitManager), settings_(settings) {
		assert(model != nullptr && "Prototype enemy requires enemy.obj");
		assert(field_ != nullptr && "Prototype enemy requires a field");
		assert(rocket_ != nullptr && "Prototype enemy requires a rocket");
		assert(energySpawner_ != nullptr && "Prototype enemy requires an energy spawner");
		assert(unitManager_ != nullptr && "Prototype enemy requires a unit manager");
		assert(settings_ != nullptr && "Prototype enemy requires settings");

		// 設定はManagerが共有し、個体ごとには状態とModelComponentだけを持つ。
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

		// 運搬中ユニットが索敵範囲にいる間だけ、ロケットより優先して追跡する。
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
		// ユニットとの接触がなければ、最終目的地であるロケットへの到達を判定する。
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

		// 複数ユニットが同じ敵へ派遣されないよう、選択候補から一時的に外す。
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

	EnergyPickup* Enemy::DefeatAndDropEnergy() {
		if (!isActive_) {
			return nullptr;
		}

		// Reset前に撃破地点と距離帯を保存し、その地点へEnergyを再生成する。
		const Vector3 dropPosition = position_;
		const EnergySize dropSize = GetDropEnergySize();
		Reset();
		return energySpawner_->SpawnOnGround(dropSize, dropPosition);
	}

	EnergySize Enemy::GetDropEnergySize() const {
		// 7層を距離に応じた3段階へまとめる。生成禁止帯でも敵ドロップは発生する。
		switch (field_->GetZone(position_)) {
		case FieldZone::Center:
		case FieldZone::Near:
			return EnergySize::Small;
		case FieldZone::NearBuffer:
		case FieldZone::Middle:
			return EnergySize::Medium;
		case FieldZone::MiddleBuffer:
		case FieldZone::Far:
		case FieldZone::OuterBuffer:
		case FieldZone::Outside:
		default:
			return EnergySize::Large;
		}
	}

	void Enemy::SetHighlighted(bool highlighted) {
		if (!isActive_ || isHighlighted_ == highlighted) {
			return;
		}

		isHighlighted_ = highlighted;
		SyncModel();
	}

	void Enemy::UpdateTarget() {
		// 現在の対象がまだ運搬中なら追跡を継続し、毎フレームの対象ぶれを防ぐ。
		if (targetUnit_ && targetUnit_->IsCarryingEnergy()) {
			return;
		}

		// 対象が消えた、納品した、倒された場合は範囲内から選び直す。
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

		// 運搬Energyは接触地点へ戻り、ユニット本体は待機状態へ即時復帰する。
		const bool defeated = targetUnit_->DefeatAndDropEnergy();
		targetUnit_ = nullptr;
		return defeated;
	}

	bool Enemy::TryHitRocket() {
		const float hitDistance = settings_->collisionRadius + rocket_->GetSettings().colliderRadius;
		if (DistanceSquaredXZ(position_, rocket_->GetPosition()) > hitDistance * hitDistance) {
			return false;
		}

		// 到達した敵はロケットのEnergyを減らした後、プールへ戻る。
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
