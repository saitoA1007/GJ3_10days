#include "PrototypeUnit.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "RenderQueue.h"

#include "Application/Prototype/Enemy/PrototypeEnemy.h"
#include "Application/Prototype/Energy/PrototypeEnergyPickup.h"
#include "Application/Prototype/Rocket/PrototypeRocket.h"

using namespace GameEngine;

namespace Prototype {

	Unit::Unit(Model* model, Rocket* rocket, const UnitSettings* settings)
		: rocket_(rocket), settings_(settings) {
		assert(model != nullptr && "Prototype unit requires unit.obj");
		assert(rocket_ != nullptr && "Prototype unit requires a rocket");
		assert(settings_ != nullptr && "Prototype unit requires settings");

		// 設定はManager側で共有し、個体は行動状態と描画情報だけを所有する。
		modelComponent_ = std::make_unique<ModelComponent>(model);
		modelComponent_->materialData_->enableLighting = true;
	}

	void Unit::Initialize() {
		// 再初期化前に確保していたEnergyがあれば、消さずに地面へ戻す。
		if (targetEnergy_ && targetEnergy_->IsActive()) {
			targetEnergy_->DropOnGround(targetEnergy_->GetPosition());
		}

		targetEnergy_ = nullptr;
		targetEnemy_ = nullptr;
		state_ = UnitState::Stored;
		stamina_ = 0.0f;
		position_ = rocket_->GetPosition() + settings_->launchOffset;
		SyncModel();
	}

	void Unit::Update(float deltaTime) {
		// 各状態の責務を分け、遷移は到達・衝突が成立した関数内だけで行う。
		switch (state_) {
		case UnitState::Stored:
			return;
		case UnitState::MovingToEnergy:
			UpdateMovingToEnergy(deltaTime);
			break;
		case UnitState::MovingToEnemy:
			UpdateMovingToEnemy(deltaTime);
			break;
		case UnitState::ReturningToRocket:
			UpdateReturningToRocket(deltaTime);
			break;
		}

		SyncModel();
	}

	void Unit::Draw(RenderQueue* renderQueue) {
		if (IsDeployed()) {
			modelComponent_->DrawRaytracing(renderQueue);
		}
	}

	void Unit::RefreshVisual() {
		if (targetEnergy_ && targetEnergy_->IsCarried()) {
			targetEnergy_->SetCarriedPosition(position_ + settings_->carryOffset);
		}
		SyncModel();
	}

	bool Unit::DispatchToEnergy(EnergyPickup* target, int32_t requestedEnergy) {
		// 対象予約が成功してからEnergyを消費し、派遣失敗時の誤消費を防ぐ。
		if (!target || !IsAvailable() || !target->TryReserve()) {
			return false;
		}

		AllocateStamina(requestedEnergy);
		position_ = rocket_->GetPosition() + settings_->launchOffset;
		targetEnergy_ = target;
		targetEnemy_ = nullptr;
		state_ = UnitState::MovingToEnergy;
		SyncModel();
		return true;
	}

	bool Unit::DispatchToEnemy(Enemy* target, int32_t requestedEnergy) {
		// 敵も予約制にし、同じ敵へ複数体が同時出撃するのを防ぐ。
		if (!target || !IsAvailable() || !target->TryReserveForAttack()) {
			return false;
		}

		AllocateStamina(requestedEnergy);
		position_ = rocket_->GetPosition() + settings_->launchOffset;
		targetEnergy_ = nullptr;
		targetEnemy_ = target;
		state_ = UnitState::MovingToEnemy;
		SyncModel();
		return true;
	}

	bool Unit::DefeatAndDropEnergy() {
		if (!IsCarryingEnergy()) {
			return false;
		}

		// 運搬物だけを接触地点へ残し、Unitはプールから削除せず再出撃可能にする。
		targetEnergy_->DropOnGround(position_);
		targetEnergy_ = nullptr;
		ReturnToStorageAfterDefeat();
		return true;
	}

	void Unit::Recall() {
		// 対象側に残った予約を必ず解除し、選択不能なオブジェクトを作らない。
		if (targetEnergy_ && targetEnergy_->IsActive()) {
			targetEnergy_->DropOnGround(targetEnergy_->GetPosition());
		}
		if (targetEnemy_) {
			targetEnemy_->CancelAttackReservation();
		}

		targetEnergy_ = nullptr;
		targetEnemy_ = nullptr;
		state_ = UnitState::Stored;
		stamina_ = 0.0f;
		position_ = rocket_->GetPosition() + settings_->launchOffset;
		SyncModel();
	}

	bool Unit::IsCarryingEnergy() const {
		return state_ == UnitState::ReturningToRocket &&
			targetEnergy_ != nullptr &&
			targetEnergy_->IsCarried();
	}

	void Unit::UpdateMovingToEnergy(float deltaTime) {
		// 他処理で予約が解除された場合は、無効なポインタを追わず帰還扱いにする。
		if (!targetEnergy_ || !targetEnergy_->IsReserved()) {
			Recall();
			return;
		}

		MoveTowards(targetEnergy_->GetPosition(), deltaTime);
		ConsumeStamina(deltaTime);

		const float pickupRadiusSquared = settings_->pickupRadius * settings_->pickupRadius;
		if (DistanceSquaredXZ(position_, targetEnergy_->GetPosition()) <= pickupRadiusSquared) {
			// 回収成立後は同じEnergyを保持したまま帰還状態へ遷移する。
			if (targetEnergy_->BeginCarry()) {
				targetEnergy_->SetCarriedPosition(position_ + settings_->carryOffset);
				state_ = UnitState::ReturningToRocket;
			} else {
				Recall();
			}
		}
	}

	void Unit::UpdateMovingToEnemy(float deltaTime) {
		if (!targetEnemy_ || !targetEnemy_->IsActive()) {
			Recall();
			return;
		}

		MoveTowards(targetEnemy_->GetPosition(), deltaTime);
		ConsumeStamina(deltaTime);

		const float hitDistance = settings_->collisionRadius + targetEnemy_->GetCollisionRadius();
		if (DistanceSquaredXZ(position_, targetEnemy_->GetPosition()) > hitDistance * hitDistance) {
			return;
		}

		// 衝突時点の距離帯に対応したEnergyを敵から生成する。
		EnergyPickup* droppedEnergy = targetEnemy_->DefeatAndDropEnergy();
		targetEnemy_ = nullptr;
		// スタミナが残っていれば勝利して即運搬、なければ相打ちでEnergyだけを残す。
		if (stamina_ > 0.0f &&
			droppedEnergy != nullptr &&
			droppedEnergy->TryReserve() &&
			droppedEnergy->BeginCarry()) {
			targetEnergy_ = droppedEnergy;
			targetEnergy_->SetCarriedPosition(position_ + settings_->carryOffset);
			state_ = UnitState::ReturningToRocket;
			return;
		}

		ReturnToStorageAfterDefeat();
	}

	void Unit::UpdateReturningToRocket(float deltaTime) {
		if (!targetEnergy_ || !targetEnergy_->IsCarried()) {
			Recall();
			return;
		}

		MoveTowards(rocket_->GetPosition(), deltaTime);
		ConsumeStamina(deltaTime);
		targetEnergy_->SetCarriedPosition(position_ + settings_->carryOffset);

		const float deliveryRadiusSquared = settings_->deliveryRadius * settings_->deliveryRadius;
		if (DistanceSquaredXZ(position_, rocket_->GetPosition()) <= deliveryRadiusSquared) {
			// DeliverはEnergy個体をプールへ戻し、返された獲得量だけをRocketへ加算する。
			rocket_->DepositEnergy(targetEnergy_->Deliver());
			targetEnergy_ = nullptr;
			state_ = UnitState::Stored;
			stamina_ = 0.0f;
		}
	}

	void Unit::AllocateStamina(int32_t requestedEnergy) {
		// EnergyChange.amountは消費時に負数なので、符号を反転してスタミナ残量にする。
		const EnergyChange allocated = rocket_->AllocateEnergyToUnit((std::max)(requestedEnergy, 0));
		stamina_ = static_cast<float>(-allocated.amount);
	}

	void Unit::ReturnToStorageAfterDefeat() {
		targetEnergy_ = nullptr;
		targetEnemy_ = nullptr;
		state_ = UnitState::Stored;
		stamina_ = 0.0f;
		position_ = rocket_->GetPosition() + settings_->launchOffset;
		SyncModel();
	}

	void Unit::MoveTowards(const Vector3& target, float deltaTime) {
		Vector3 direction = target - position_;
		direction.y = 0.0f;
		const float distance = direction.Length();
		if (distance <= 0.0001f) {
			return;
		}

		direction.Normalize();
		// スタミナが1以上ではなく、わずかでも残っていれば高速移動を選ぶ。
		const float speed = stamina_ > 0.0f ? settings_->boostedSpeed : settings_->normalSpeed;
		const float moveDistance = (std::min)(speed * (std::max)(deltaTime, 0.0f), distance);
		position_ += direction * moveDistance;
		modelComponent_->worldTransform_.transform_.rotate.y = std::atan2(direction.x, direction.z);
	}

	void Unit::ConsumeStamina(float deltaTime) {
		if (stamina_ <= 0.0f) {
			return;
		}

		const float distanceFromRocket = std::sqrt(DistanceSquaredXZ(position_, rocket_->GetPosition()));
		// ロケットから離れるほど倍率を線形に増やし、遠距離派遣のコストを高くする。
		const float distanceMultiplier = 1.0f + distanceFromRocket * settings_->distanceDrainRate;
		const float consumed = settings_->staminaDrainPerSecond * distanceMultiplier * (std::max)(deltaTime, 0.0f);
		stamina_ = (std::max)(stamina_ - consumed, 0.0f);
	}

	float Unit::DistanceSquaredXZ(const Vector3& a, const Vector3& b) const {
		const float x = a.x - b.x;
		const float z = a.z - b.z;
		return x * x + z * z;
	}

	void Unit::SyncModel() {
		modelComponent_->worldTransform_.transform_.scale = settings_->scale;
		modelComponent_->worldTransform_.transform_.translate = position_;
		// 水色なら高速移動可能、通常色ならスタミナ切れであることを示す。
		modelComponent_->materialData_->color = stamina_ > 0.0f
			? settings_->staminaColor
			: settings_->normalColor;
		modelComponent_->Update();
	}
}
