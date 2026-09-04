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

		modelComponent_ = std::make_unique<ModelComponent>(model);
		modelComponent_->materialData_->enableLighting = true;
	}

	void Unit::Initialize() {
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

		targetEnergy_->DropOnGround(position_);
		targetEnergy_ = nullptr;
		ReturnToStorageAfterDefeat();
		return true;
	}

	void Unit::Recall() {
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
		if (!targetEnergy_ || !targetEnergy_->IsReserved()) {
			Recall();
			return;
		}

		MoveTowards(targetEnergy_->GetPosition(), deltaTime);
		ConsumeStamina(deltaTime);

		const float pickupRadiusSquared = settings_->pickupRadius * settings_->pickupRadius;
		if (DistanceSquaredXZ(position_, targetEnergy_->GetPosition()) <= pickupRadiusSquared) {
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

		EnergyPickup* droppedEnergy = targetEnemy_->DefeatAndDropSmallEnergy();
		targetEnemy_ = nullptr;
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
			rocket_->DepositEnergy(targetEnergy_->Deliver());
			targetEnergy_ = nullptr;
			state_ = UnitState::Stored;
			stamina_ = 0.0f;
		}
	}

	void Unit::AllocateStamina(int32_t requestedEnergy) {
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
		modelComponent_->materialData_->color = stamina_ > 0.0f
			? settings_->staminaColor
			: settings_->normalColor;
		modelComponent_->Update();
	}
}
