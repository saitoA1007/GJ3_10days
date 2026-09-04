#include "PrototypeRocket.h"

#include <algorithm>
#include <cassert>
#include <utility>

#include "Application/CollisionConfig.h"
#include "ImGuiManager.h"

using namespace GameEngine;

namespace Prototype {

	Rocket::Rocket(Model* model, const RocketSettings& settings)
		: settings_(settings), energy_(settings.initialEnergy) {
		assert(model != nullptr && "Prototype rocket requires rocket.obj");
		modelComponent_ = std::make_unique<ModelComponent>(model);
		modelComponent_->materialData_->enableLighting = true;

		collider_.SetCollisionAttribute(kCollisionAttributeRocket);
		collider_.SetCollisionMask(kCollisionAttributeEnemy);
		collider_.SetUserData({ static_cast<uint32_t>(CollisionTypeID::kRocket), this });
		collider_.SetOnCollisionEnterCallback([this](const CollisionResult& result) {
			OnCollisionEnter(result);
		});

		debugParameter_ = std::make_unique<DebugParameter>("PrototypeRocket");
		debugParameter_->Register("Position", settings_.position, 0, "Transform");
		debugParameter_->Register("Scale", settings_.scale, 1, "Transform");
		debugParameter_->Register("Radius", settings_.colliderRadius, 0, "Collider");
		debugParameter_->Register("OffsetY", settings_.colliderOffsetY, 1, "Collider");
		debugParameter_->Register("InitialEnergy", settings_.initialEnergy, 0, "Energy");
		debugParameter_->Register("EnemyHitLoss", settings_.enemyHitLoss, 1, "Energy");
		debugParameter_->Register("ChangeAmount", settings_.debugEnergyAmount, 0, "Debug");
		debugParameter_->Apply();
		SanitizeSettings();
	}

	void Rocket::Initialize() {
		ApplyDebugParameters();
		energy_.Reset(settings_.initialEnergy);
		collider_.SetActive(true);
		SyncComponents();
	}

	void Rocket::Update() {
		ApplyDebugParameters();
		SyncComponents();
	}

	void Rocket::DebugUpdate() {
		ApplyDebugParameters();
		SyncComponents();
		DrawDebugWindow();
	}

	void Rocket::Draw() {
		modelComponent_->DrawRaytracing(renderQueue_);
	}

	EnergyChange Rocket::DepositEnergy(int32_t amount) {
		const EnergyChange change = energy_.Add(amount, EnergyChangeReason::Delivery);
		NotifyEnergyChanged(change);
		return change;
	}

	EnergyChange Rocket::AllocateEnergyToUnit(int32_t requestedAmount) {
		const EnergyChange change = energy_.ConsumeUpTo(requestedAmount, EnergyChangeReason::UnitAllocation);
		NotifyEnergyChanged(change);
		return change;
	}

	EnergyChange Rocket::ReceiveEnemyHit() {
		const EnergyChange change = energy_.ConsumeUpTo(settings_.enemyHitLoss, EnergyChangeReason::EnemyHit);
		NotifyEnergyChanged(change);
		return change;
	}

	void Rocket::ResetEnergy() {
		NotifyEnergyChanged(energy_.Reset(settings_.initialEnergy));
	}

	void Rocket::ApplyDebugParameters() {
		debugParameter_->ApplyIfDirty();
		SanitizeSettings();
	}

	void Rocket::SanitizeSettings() {
		settings_.scale.x = (std::max)(settings_.scale.x, 0.0f);
		settings_.scale.y = (std::max)(settings_.scale.y, 0.0f);
		settings_.scale.z = (std::max)(settings_.scale.z, 0.0f);
		settings_.colliderRadius = (std::max)(settings_.colliderRadius, 0.0f);
		settings_.initialEnergy = (std::max)(settings_.initialEnergy, 0);
		settings_.enemyHitLoss = (std::max)(settings_.enemyHitLoss, 0);
		settings_.debugEnergyAmount = (std::max)(settings_.debugEnergyAmount, 0);
	}

	void Rocket::SyncComponents() {
		modelComponent_->worldTransform_.transform_.scale = settings_.scale;
		modelComponent_->worldTransform_.transform_.translate = settings_.position;
		modelComponent_->Update();

		collider_.SetWorldPosition({
			settings_.position.x,
			settings_.position.y + settings_.colliderOffsetY,
			settings_.position.z,
		});
		collider_.SetRadius(settings_.colliderRadius);
	}

	void Rocket::NotifyEnergyChanged(const EnergyChange& change) {
		if (change.Changed() && onEnergyChanged_) {
			onEnergyChanged_(change);
		}
	}

	void Rocket::OnCollisionEnter(const CollisionResult& result) {
		if (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kEnemy)) {
			ReceiveEnemyHit();
		}
	}

	void Rocket::DrawDebugWindow() {
#ifdef USE_IMGUI
		if (!ImGui::Begin("Prototype Rocket")) {
			ImGui::End();
			return;
		}

		ImGui::Text("Energy: %d", energy_.GetCurrent());
		ImGui::TextDisabled("ChangeAmount is configured in ParameterInspector.");

		if (ImGui::Button("Deposit")) {
			const EnergyChange change = energy_.Add(settings_.debugEnergyAmount, EnergyChangeReason::Debug);
			NotifyEnergyChanged(change);
		}
		ImGui::SameLine();
		if (ImGui::Button("Consume")) {
			const EnergyChange change = energy_.ConsumeUpTo(settings_.debugEnergyAmount, EnergyChangeReason::Debug);
			NotifyEnergyChanged(change);
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			ResetEnergy();
		}

		ImGui::End();
#endif
	}
}
