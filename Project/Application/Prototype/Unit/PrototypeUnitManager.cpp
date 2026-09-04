#include "PrototypeUnitManager.h"

#include <algorithm>
#include <cassert>

#include "FPSCounter.h"
#include "ImGuiManager.h"

#include "Application/Prototype/Enemy/PrototypeEnemy.h"
#include "Application/Prototype/Energy/PrototypeEnergyPickup.h"
#include "Application/Prototype/Rocket/PrototypeRocket.h"

using namespace GameEngine;

namespace Prototype {

	UnitManager::UnitManager(Model* unitModel, Rocket* rocket, size_t capacity)
		: rocket_(rocket) {
		assert(unitModel != nullptr && "Prototype unit manager requires unit.obj");
		assert(rocket_ != nullptr && "Prototype unit manager requires a rocket");

		// Unitは倒れても再利用するため、最大候補数を固定プールとして確保する。
		const size_t safeCapacity = (std::max)(capacity, size_t{ 1 });
		units_.reserve(safeCapacity);
		for (size_t i = 0; i < safeCapacity; ++i) {
			units_.push_back(std::make_unique<Unit>(unitModel, rocket_, &settings_.unit));
		}

		debugParameter_ = std::make_unique<DebugParameter>("PrototypeUnit");
		debugParameter_->Register("UnitCount", settings_.unitCount, 0, "Manager");
		debugParameter_->Register("LaunchOffset", settings_.unit.launchOffset, 0, "Transform");
		debugParameter_->Register("Scale", settings_.unit.scale, 1, "Transform");
		debugParameter_->Register("CarryOffset", settings_.unit.carryOffset, 2, "Transform");
		debugParameter_->Register("NormalSpeed", settings_.unit.normalSpeed, 0, "Move");
		debugParameter_->Register("BoostedSpeed", settings_.unit.boostedSpeed, 1, "Move");
		debugParameter_->Register("PickupRadius", settings_.unit.pickupRadius, 2, "Move");
		debugParameter_->Register("DeliveryRadius", settings_.unit.deliveryRadius, 3, "Move");
		debugParameter_->Register("Radius", settings_.unit.collisionRadius, 0, "Collision");
		debugParameter_->Register("DrainPerSecond", settings_.unit.staminaDrainPerSecond, 0, "Stamina");
		debugParameter_->Register("DistanceDrainRate", settings_.unit.distanceDrainRate, 1, "Stamina");
		debugParameter_->Register("Normal", settings_.unit.normalColor, 0, "Color");
		debugParameter_->Register("WithStamina", settings_.unit.staminaColor, 1, "Color");
		debugParameter_->Apply();
		SanitizeSettings();
		SetUpdateOrder(20);
	}

	void UnitManager::Initialize() {
		ApplyDebugParameters();
		gameplayEnabled_ = true;
		for (auto& unit : units_) {
			unit->Initialize();
		}
	}

	void UnitManager::Update() {
		ApplyDebugParameters();
		ApplyUnitCount();
		if (!gameplayEnabled_) {
			return;
		}

		// capacity全体ではなく、Registerで指定された先頭unitCount体だけを稼働させる。
		for (size_t i = 0; i < GetUnitCount(); ++i) {
			units_[i]->Update(FpsCounter::gameDeltaTime);
		}
	}

	void UnitManager::DebugUpdate() {
		ApplyDebugParameters();
		ApplyUnitCount();
		for (size_t i = 0; i < GetUnitCount(); ++i) {
			units_[i]->RefreshVisual();
		}
		DrawDebugWindow();
	}

	void UnitManager::Draw() {
		for (size_t i = 0; i < GetUnitCount(); ++i) {
			units_[i]->Draw(renderQueue_);
		}
	}

	bool UnitManager::DispatchToEnergy(EnergyPickup* target, int32_t requestedEnergy) {
		if (!gameplayEnabled_ || !target || !target->IsTargetable()) {
			return false;
		}

		// 待機中の最初の1体へ依頼し、利用可能数を越えた派遣は拒否する。
		for (size_t i = 0; i < GetUnitCount(); ++i) {
			if (units_[i]->IsAvailable()) {
				return units_[i]->DispatchToEnergy(target, requestedEnergy);
			}
		}
		return false;
	}

	bool UnitManager::DispatchToEnemy(Enemy* target, int32_t requestedEnergy) {
		if (!gameplayEnabled_ || !target || !target->IsTargetable()) {
			return false;
		}

		for (size_t i = 0; i < GetUnitCount(); ++i) {
			if (units_[i]->IsAvailable()) {
				return units_[i]->DispatchToEnemy(target, requestedEnergy);
			}
		}
		return false;
	}

	Unit* UnitManager::FindNearestCarryingUnit(const Vector3& position, float maxDistance) const {
		Unit* nearest = nullptr;
		const float safeMaxDistance = (std::max)(maxDistance, 0.0f);
		float nearestDistanceSquared = safeMaxDistance * safeMaxDistance;

		// Enemyの索敵用。sqrtを避けてXZ距離の二乗で最短個体を比較する。
		for (size_t i = 0; i < GetUnitCount(); ++i) {
			Unit* unit = units_[i].get();
			if (!unit->IsCarryingEnergy()) {
				continue;
			}

			const Vector3 offset = unit->GetPosition() - position;
			const float distanceSquared = offset.x * offset.x + offset.z * offset.z;
			if (distanceSquared <= nearestDistanceSquared) {
				nearest = unit;
				nearestDistanceSquared = distanceSquared;
			}
		}

		return nearest;
	}

	void UnitManager::RecallAll() {
		for (auto& unit : units_) {
			unit->Recall();
		}
	}

	size_t UnitManager::GetAvailableCount() const {
		size_t count = 0;
		for (size_t i = 0; i < GetUnitCount(); ++i) {
			if (units_[i]->IsAvailable()) {
				++count;
			}
		}
		return count;
	}

	size_t UnitManager::GetDeployedCount() const {
		return static_cast<size_t>(std::count_if(
			units_.begin(),
			units_.begin() + GetUnitCount(),
			[](const auto& unit) { return unit->IsDeployed(); }));
	}

	void UnitManager::ApplyDebugParameters() {
		debugParameter_->ApplyIfDirty();
		SanitizeSettings();
	}

	void UnitManager::SanitizeSettings() {
		settings_.unitCount = (std::clamp)(settings_.unitCount, 1, static_cast<int32_t>(units_.size()));
		settings_.unit.scale.x = (std::max)(settings_.unit.scale.x, 0.0f);
		settings_.unit.scale.y = (std::max)(settings_.unit.scale.y, 0.0f);
		settings_.unit.scale.z = (std::max)(settings_.unit.scale.z, 0.0f);
		settings_.unit.normalSpeed = (std::max)(settings_.unit.normalSpeed, 0.0f);
		settings_.unit.boostedSpeed = (std::max)(settings_.unit.boostedSpeed, settings_.unit.normalSpeed);
		settings_.unit.pickupRadius = (std::max)(settings_.unit.pickupRadius, 0.0f);
		settings_.unit.deliveryRadius = (std::max)(settings_.unit.deliveryRadius, 0.0f);
		settings_.unit.collisionRadius = (std::max)(settings_.unit.collisionRadius, 0.0f);
		settings_.unit.staminaDrainPerSecond = (std::max)(settings_.unit.staminaDrainPerSecond, 0.0f);
		settings_.unit.distanceDrainRate = (std::max)(settings_.unit.distanceDrainRate, 0.0f);
	}

	void UnitManager::ApplyUnitCount() {
		// 実行中に参加数を減らした場合、範囲外になった個体の予約を安全に解放する。
		for (size_t i = GetUnitCount(); i < units_.size(); ++i) {
			if (!units_[i]->IsAvailable()) {
				units_[i]->Recall();
			}
		}
	}

	void UnitManager::DrawDebugWindow() {
#ifdef USE_IMGUI
		if (!ImGui::Begin("Prototype Units")) {
			ImGui::End();
			return;
		}

		ImGui::Text("Stored: %zu", GetAvailableCount());
		ImGui::Text("Deployed: %zu", GetDeployedCount());
		if (ImGui::Button("Recall All")) {
			RecallAll();
		}

		ImGui::End();
#endif
	}
}
