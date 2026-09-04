#include "PrototypeEnemyManager.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "FPSCounter.h"
#include "ImGuiManager.h"
#include "MyMath.h"
#include "RandomGenerator.h"

#include "Application/Prototype/Energy/PrototypeEnergySpawner.h"
#include "Application/Prototype/Field/PrototypeField.h"
#include "Application/Prototype/Rocket/PrototypeRocket.h"
#include "Application/Prototype/Unit/PrototypeUnitManager.h"

using namespace GameEngine;

namespace Prototype {

	EnemyManager::EnemyManager(
		Model* enemyModel,
		Field* field,
		Rocket* rocket,
		EnergySpawner* energySpawner,
		UnitManager* unitManager,
		size_t capacity)
		: field_(field) {
		assert(enemyModel != nullptr && "Prototype enemy manager requires enemy.obj");
		assert(field_ != nullptr && "Prototype enemy manager requires a field");
		assert(rocket != nullptr && "Prototype enemy manager requires a rocket");
		assert(energySpawner != nullptr && "Prototype enemy manager requires an energy spawner");
		assert(unitManager != nullptr && "Prototype enemy manager requires a unit manager");

		const size_t safeCapacity = (std::max)(capacity, size_t{ 1 });
		enemies_.reserve(safeCapacity);
		for (size_t i = 0; i < safeCapacity; ++i) {
			enemies_.push_back(std::make_unique<Enemy>(
				enemyModel,
				rocket,
				energySpawner,
				unitManager,
				&settings_.enemy));
		}

		debugParameter_ = std::make_unique<DebugParameter>("PrototypeEnemy");
		debugParameter_->Register("Interval", settings_.spawnInterval, 0, "Spawn");
		debugParameter_->Register("InitialCount", settings_.initialCount, 1, "Spawn");
		debugParameter_->Register("MaxActiveCount", settings_.maxActiveCount, 2, "Spawn");
		debugParameter_->Register("Scale", settings_.enemy.scale, 0, "Transform");
		debugParameter_->Register("GroundHeight", settings_.enemy.groundHeight, 1, "Transform");
		debugParameter_->Register("Speed", settings_.enemy.moveSpeed, 0, "Move");
		debugParameter_->Register("SearchRadius", settings_.enemy.searchRadius, 0, "Target");
		debugParameter_->Register("Radius", settings_.enemy.collisionRadius, 0, "Collision");
		debugParameter_->Register("Body", settings_.enemy.color, 0, "Color");
		debugParameter_->Apply();
		SanitizeSettings();
		SetUpdateOrder(25);
	}

	void EnemyManager::Initialize() {
		ApplyDebugParameters();
		gameplayEnabled_ = true;
		spawnTimer_ = 0.0f;
		for (auto& enemy : enemies_) {
			enemy->Reset();
		}

		for (int32_t i = 0; i < settings_.initialCount; ++i) {
			SpawnOne();
		}
	}

	void EnemyManager::Update() {
		ApplyDebugParameters();
		if (!gameplayEnabled_) {
			return;
		}

		for (auto& enemy : enemies_) {
			enemy->Update(FpsCounter::gameDeltaTime);
		}

		spawnTimer_ += FpsCounter::gameDeltaTime;
		if (spawnTimer_ >= settings_.spawnInterval) {
			spawnTimer_ = 0.0f;
			SpawnOne();
		}
	}

	void EnemyManager::DebugUpdate() {
		ApplyDebugParameters();
		for (auto& enemy : enemies_) {
			enemy->RefreshVisual();
		}
		DrawDebugWindow();
	}

	void EnemyManager::Draw() {
		for (auto& enemy : enemies_) {
			enemy->Draw(renderQueue_);
		}
	}

	bool EnemyManager::SpawnOne() {
		if (!gameplayEnabled_) {
			return false;
		}
		if (GetActiveCount() >= static_cast<size_t>(settings_.maxActiveCount)) {
			return false;
		}

		auto available = std::find_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
			return !enemy->IsActive();
		});
		if (available == enemies_.end()) {
			return false;
		}

		(*available)->Spawn(MakeSpawnPosition());
		return true;
	}

	Enemy* EnemyManager::FindNearestTargetable(const Vector3& position, float maxDistance) const {
		Enemy* nearest = nullptr;
		const float safeMaxDistance = (std::max)(maxDistance, 0.0f);
		float nearestDistanceSquared = safeMaxDistance * safeMaxDistance;

		for (const auto& enemy : enemies_) {
			if (!enemy->IsTargetable()) {
				continue;
			}

			const Vector3 offset = enemy->GetPosition() - position;
			const float distanceSquared = offset.x * offset.x + offset.z * offset.z;
			if (distanceSquared <= nearestDistanceSquared) {
				nearest = enemy.get();
				nearestDistanceSquared = distanceSquared;
			}
		}

		return nearest;
	}

	size_t EnemyManager::GetActiveCount() const {
		return static_cast<size_t>(std::count_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
			return enemy->IsActive();
		}));
	}

	size_t EnemyManager::GetCarrierTargetCount() const {
		return static_cast<size_t>(std::count_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
			return enemy->IsActive() && enemy->IsTargetingCarrier();
		}));
	}

	void EnemyManager::ApplyDebugParameters() {
		debugParameter_->ApplyIfDirty();
		SanitizeSettings();
	}

	void EnemyManager::SanitizeSettings() {
		settings_.spawnInterval = (std::max)(settings_.spawnInterval, 0.1f);
		settings_.initialCount = (std::clamp)(
			settings_.initialCount,
			0,
			static_cast<int32_t>(enemies_.size()));
		settings_.maxActiveCount = (std::clamp)(
			settings_.maxActiveCount,
			1,
			static_cast<int32_t>(enemies_.size()));
		settings_.initialCount = (std::min)(settings_.initialCount, settings_.maxActiveCount);
		settings_.enemy.scale.x = (std::max)(settings_.enemy.scale.x, 0.0f);
		settings_.enemy.scale.y = (std::max)(settings_.enemy.scale.y, 0.0f);
		settings_.enemy.scale.z = (std::max)(settings_.enemy.scale.z, 0.0f);
		settings_.enemy.moveSpeed = (std::max)(settings_.enemy.moveSpeed, 0.0f);
		settings_.enemy.searchRadius = (std::max)(settings_.enemy.searchRadius, 0.0f);
		settings_.enemy.collisionRadius = (std::max)(settings_.enemy.collisionRadius, 0.0f);
	}

	Vector3 EnemyManager::MakeSpawnPosition() const {
		const float angle = RandomGenerator::Get<float>(0.0f, TWO_PI);
		const float radius = field_->GetRadius(FieldZone::OuterBuffer);
		const Vector3 center = field_->GetSettings().center;
		return {
			center.x + std::cos(angle) * radius,
			settings_.enemy.groundHeight,
			center.z + std::sin(angle) * radius,
		};
	}

	void EnemyManager::DrawDebugWindow() {
#ifdef USE_IMGUI
		if (!ImGui::Begin("Prototype Enemies")) {
			ImGui::End();
			return;
		}

		ImGui::Text("Active: %zu / %zu", GetActiveCount(), enemies_.size());
		ImGui::Text("Targeting Carrier: %zu", GetCarrierTargetCount());
		if (ImGui::Button("Spawn Enemy")) {
			SpawnOne();
		}

		ImGui::End();
#endif
	}
}
