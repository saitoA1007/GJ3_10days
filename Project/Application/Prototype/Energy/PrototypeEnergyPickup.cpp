#include "PrototypeEnergyPickup.h"

#include <algorithm>
#include <cassert>

#include "Model.h"
#include "RenderQueue.h"

using namespace GameEngine;

namespace Prototype {

	EnergyPickup::EnergyPickup(Model* model) {
		assert(model != nullptr && "Prototype energy requires energy.obj");
		modelComponent_ = std::make_unique<ModelComponent>(model);
		modelComponent_->materialData_->enableLighting = true;
	}

	void EnergyPickup::Spawn(
		EnergySize size,
		const Vector3& groundPosition,
		float fallHeight,
		float fallSpeed,
		const EnergyTypeSettings& typeSettings) {
		size_ = size;
		state_ = EnergyState::Falling;
		typeSettings_ = typeSettings;
		groundY_ = groundPosition.y;
		fallSpeed_ = (std::max)(fallSpeed, 0.0f);
		position_ = groundPosition;
		position_.y += (std::max)(fallHeight, 0.0f);
		SyncModel();
	}

	void EnergyPickup::SpawnOnGround(
		EnergySize size,
		const Vector3& groundPosition,
		const EnergyTypeSettings& typeSettings) {
		size_ = size;
		state_ = EnergyState::OnGround;
		typeSettings_ = typeSettings;
		groundY_ = groundPosition.y;
		fallSpeed_ = 0.0f;
		position_ = groundPosition;
		isHighlighted_ = false;
		SyncModel();
	}

	void EnergyPickup::Reset() {
		state_ = EnergyState::Inactive;
		position_ = {};
		groundY_ = 0.0f;
		fallSpeed_ = 0.0f;
		isHighlighted_ = false;
	}

	void EnergyPickup::Update(float deltaTime) {
		if (!IsActive()) {
			return;
		}

		if (state_ == EnergyState::Falling) {
			position_.y -= fallSpeed_ * (std::max)(deltaTime, 0.0f);
			if (position_.y <= groundY_) {
				position_.y = groundY_;
				state_ = EnergyState::OnGround;
			}
		}

		SyncModel();
	}

	void EnergyPickup::Draw(RenderQueue* renderQueue) {
		if (IsActive()) {
			modelComponent_->DrawRaytracing(renderQueue);
		}
	}

	bool EnergyPickup::TryReserve() {
		if (!IsTargetable()) {
			return false;
		}

		state_ = EnergyState::Reserved;
		return true;
	}

	bool EnergyPickup::BeginCarry() {
		if (!IsReserved()) {
			return false;
		}

		state_ = EnergyState::Carried;
		return true;
	}

	void EnergyPickup::SetCarriedPosition(const Vector3& position) {
		if (!IsCarried()) {
			return;
		}

		position_ = position;
		SyncModel();
	}

	void EnergyPickup::DropOnGround(const Vector3& position) {
		if (!IsReserved() && !IsCarried()) {
			return;
		}

		position_ = position;
		position_.y = groundY_;
		state_ = EnergyState::OnGround;
		SyncModel();
	}

	int32_t EnergyPickup::Deliver() {
		if (!IsCarried()) {
			return 0;
		}

		const int32_t deliveredValue = typeSettings_.value;
		Reset();
		return deliveredValue;
	}

	void EnergyPickup::ApplyTypeSettings(const EnergyTypeSettings& settings) {
		typeSettings_ = settings;
		typeSettings_.scale = (std::max)(typeSettings_.scale, 0.0f);
		typeSettings_.value = (std::max)(typeSettings_.value, 0);
		if (IsActive()) {
			SyncModel();
		}
	}

	void EnergyPickup::SetHighlighted(bool highlighted) {
		if (isHighlighted_ == highlighted) {
			return;
		}

		isHighlighted_ = highlighted;
		if (IsActive()) {
			SyncModel();
		}
	}

	void EnergyPickup::SyncModel() {
		const float scale = typeSettings_.scale;
		modelComponent_->worldTransform_.transform_.scale = { scale, scale, scale };
		modelComponent_->worldTransform_.transform_.translate = position_;
		Vector4 color = typeSettings_.color;
		if (isHighlighted_) {
			color.x = color.x + (1.0f - color.x) * 0.65f;
			color.y = color.y + (1.0f - color.y) * 0.65f;
			color.z = color.z + (1.0f - color.z) * 0.65f;
		}
		modelComponent_->materialData_->color = color;
		modelComponent_->Update();
	}
}
