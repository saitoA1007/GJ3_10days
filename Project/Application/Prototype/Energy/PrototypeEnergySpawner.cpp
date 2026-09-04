#include "PrototypeEnergySpawner.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "FPSCounter.h"
#include "ImGuiManager.h"
#include "MyMath.h"
#include "RandomGenerator.h"

using namespace GameEngine;

namespace {
	constexpr std::array<const char*, Prototype::kEnergySizeCount> kEnergySizeNames = {
		"Small",
		"Medium",
		"Large",
	};
	// 自然落下するEnergyは生成禁止帯を除いた3領域だけから抽選する。
	constexpr std::array<Prototype::FieldZone, Prototype::kEnergySizeCount> kEnergySpawnZones = {
		Prototype::FieldZone::Near,
		Prototype::FieldZone::Middle,
		Prototype::FieldZone::Far,
	};
}

namespace Prototype {

	EnergySpawner::EnergySpawner(Model* energyModel, Field* field, size_t capacity)
		: field_(field) {
		assert(energyModel != nullptr && "Prototype energy spawner requires energy.obj");
		assert(field_ != nullptr && "Prototype energy spawner requires a field");

		// Energyは生成頻度が高いため、固定プールを作って実行中の確保を避ける。
		const size_t safeCapacity = (std::max)(capacity, size_t{ 1 });
		pickups_.reserve(safeCapacity);
		for (size_t i = 0; i < safeCapacity; ++i) {
			pickups_.push_back(std::make_unique<EnergyPickup>(energyModel));
		}

		debugParameter_ = std::make_unique<DebugParameter>("PrototypeEnergy");
		debugParameter_->Register("SpawnInterval", settings_.spawnInterval, 0, "Spawn");
		debugParameter_->Register("FallHeight", settings_.fallHeight, 1, "Spawn");
		debugParameter_->Register("FallSpeed", settings_.fallSpeed, 2, "Spawn");
		debugParameter_->Register("GroundHeight", settings_.groundHeight, 3, "Spawn");
		debugParameter_->Register("MaxActiveCount", settings_.maxActiveCount, 4, "Spawn");
		debugParameter_->Register("InitialCountPerZone", settings_.initialCountPerZone, 5, "Spawn");

		for (size_t i = 0; i < typeSettings_.size(); ++i) {
			const std::string group = std::string("Type/") + kEnergySizeNames[i];
			debugParameter_->Register("Scale", typeSettings_[i].scale, 0, group);
			debugParameter_->Register("Value", typeSettings_[i].value, 1, group);
			debugParameter_->Register("Color", typeSettings_[i].color, 2, group);
		}

		debugParameter_->Apply();
		SanitizeSettings();
		SetUpdateOrder(10);
	}

	void EnergySpawner::Initialize() {
		ApplyDebugParameters();
		gameplayEnabled_ = true;
		spawnTimer_ = 0.0f;
		// シーン再初期化時に落下・予約・運搬状態を残さない。
		for (auto& pickup : pickups_) {
			pickup->Reset();
		}

		// 各サイズを最初から確認できるよう、3つの生成領域へ同数ずつ置く。
		for (int32_t i = 0; i < settings_.initialCountPerZone; ++i) {
			SpawnInZone(FieldZone::Near);
			SpawnInZone(FieldZone::Middle);
			SpawnInZone(FieldZone::Far);
		}
	}

	void EnergySpawner::Update() {
		ApplyDebugParameters();
		if (!gameplayEnabled_) {
			return;
		}

		// gameplayEnabled_がfalseの間は落下も生成タイマーも完全に停止する。
		UpdatePickups(FpsCounter::gameDeltaTime);

		spawnTimer_ += FpsCounter::gameDeltaTime;
		if (spawnTimer_ >= settings_.spawnInterval) {
			spawnTimer_ = 0.0f;
			SpawnRandom();
		}
	}

	void EnergySpawner::DebugUpdate() {
		ApplyDebugParameters();
		UpdatePickups(0.0f);
		DrawDebugWindow();
	}

	void EnergySpawner::Draw() {
		for (auto& pickup : pickups_) {
			pickup->Draw(renderQueue_);
		}
	}

	bool EnergySpawner::SpawnInZone(FieldZone zone) {
		if (!gameplayEnabled_) {
			return false;
		}
		// Centerと3つのBufferには自然生成しない。
		if (zone != FieldZone::Near && zone != FieldZone::Middle && zone != FieldZone::Far) {
			return false;
		}
		if (GetActiveCount() >= static_cast<size_t>(settings_.maxActiveCount)) {
			return false;
		}

		// 非アクティブな個体だけをプールから再利用する。
		auto available = std::find_if(pickups_.begin(), pickups_.end(), [](const auto& pickup) {
			return !pickup->IsActive();
		});
		if (available == pickups_.end()) {
			return false;
		}

		const EnergySize size = GetEnergySize(zone);
		(*available)->Spawn(
			size,
			MakeSpawnPosition(zone),
			settings_.fallHeight,
			settings_.fallSpeed,
			typeSettings_[static_cast<size_t>(size)]);
		return true;
	}

	EnergyPickup* EnergySpawner::SpawnOnGround(EnergySize size, const Vector3& position) {
		const size_t sizeIndex = static_cast<size_t>(size);
		if (sizeIndex >= typeSettings_.size()) {
			return nullptr;
		}

		auto available = std::find_if(pickups_.begin(), pickups_.end(), [](const auto& pickup) {
			return !pickup->IsActive();
		});
		if (available == pickups_.end()) {
			return nullptr;
		}

		// 呼び出し元のY座標に関係なく、Energy用の地面高さへ揃える。
		Vector3 groundPosition = position;
		groundPosition.y = settings_.groundHeight;
		(*available)->SpawnOnGround(size, groundPosition, typeSettings_[sizeIndex]);
		return available->get();
	}

	EnergyPickup* EnergySpawner::FindNearestAvailable(const Vector3& position, float maxDistance) {
		EnergyPickup* nearest = nullptr;
		const float safeMaxDistance = (std::max)(maxDistance, 0.0f);
		float nearestDistanceSquared = safeMaxDistance * safeMaxDistance;

		// Falling・Reserved・Carriedはロックオン候補に含めない。
		for (auto& pickup : pickups_) {
			if (!pickup->IsTargetable()) {
				continue;
			}

			const Vector3 offset = pickup->GetPosition() - position;
			const float distanceSquared = offset.x * offset.x + offset.z * offset.z;
			if (distanceSquared <= nearestDistanceSquared) {
				nearest = pickup.get();
				nearestDistanceSquared = distanceSquared;
			}
		}

		return nearest;
	}

	size_t EnergySpawner::GetActiveCount() const {
		return static_cast<size_t>(std::count_if(pickups_.begin(), pickups_.end(), [](const auto& pickup) {
			return pickup->IsActive();
		}));
	}

	void EnergySpawner::ApplyDebugParameters() {
		debugParameter_->ApplyIfDirty();
		SanitizeSettings();
	}

	void EnergySpawner::SanitizeSettings() {
		settings_.spawnInterval = (std::max)(settings_.spawnInterval, 0.1f);
		settings_.fallHeight = (std::max)(settings_.fallHeight, 0.0f);
		settings_.fallSpeed = (std::max)(settings_.fallSpeed, 0.01f);
		settings_.maxActiveCount = (std::clamp)(
			settings_.maxActiveCount,
			1,
			static_cast<int32_t>(pickups_.size()));
		settings_.initialCountPerZone = (std::clamp)(
			settings_.initialCountPerZone,
			0,
			settings_.maxActiveCount / 3);

		for (auto& type : typeSettings_) {
			type.scale = (std::max)(type.scale, 0.0f);
			type.value = (std::max)(type.value, 0);
		}
	}

	void EnergySpawner::UpdatePickups(float deltaTime) {
		for (auto& pickup : pickups_) {
			if (!pickup->IsActive()) {
				continue;
			}

			// Register変更を既に存在する個体にも即時反映する。
			pickup->ApplyTypeSettings(typeSettings_[static_cast<size_t>(pickup->GetSize())]);
			pickup->Update(deltaTime);
		}
	}

	void EnergySpawner::SpawnRandom() {
		const int zoneIndex = RandomGenerator::Get<int>(0, static_cast<int>(kEnergySpawnZones.size()) - 1);
		SpawnInZone(kEnergySpawnZones[static_cast<size_t>(zoneIndex)]);
	}

	Vector3 EnergySpawner::MakeSpawnPosition(FieldZone zone) const {
		// 各生成領域の直前にあるBufferを内周として、生成可能な円環を求める。
		FieldZone innerZone = FieldZone::Center;
		switch (zone) {
		case FieldZone::Middle:
			innerZone = FieldZone::NearBuffer;
			break;
		case FieldZone::Far:
			innerZone = FieldZone::MiddleBuffer;
			break;
		case FieldZone::Near:
		default:
			break;
		}

		const float innerRadius = field_->GetRadius(innerZone);
		const float outerRadius = field_->GetRadius(zone);
		const float minRadius = (std::min)(innerRadius, outerRadius);
		const float maxRadius = (std::max)(innerRadius, outerRadius);

		// 半径そのものではなく二乗を一様抽選し、円環の外側へ偏る問題を防ぐ。
		const float radiusSquared = RandomGenerator::Get<float>(
			minRadius * minRadius,
			maxRadius * maxRadius);
		const float radius = std::sqrt(radiusSquared);
		const float angle = RandomGenerator::Get<float>(0.0f, TWO_PI);
		const Vector3 center = field_->GetSettings().center;

		return {
			center.x + std::cos(angle) * radius,
			settings_.groundHeight,
			center.z + std::sin(angle) * radius,
		};
	}

	EnergySize EnergySpawner::GetEnergySize(FieldZone zone) const {
		switch (zone) {
		case FieldZone::Near:
			return EnergySize::Small;
		case FieldZone::Middle:
			return EnergySize::Medium;
		case FieldZone::Far:
			return EnergySize::Large;
		default:
			return EnergySize::Small;
		}
	}

	void EnergySpawner::DrawDebugWindow() {
#ifdef USE_IMGUI
		if (!ImGui::Begin("Prototype Energy")) {
			ImGui::End();
			return;
		}

		ImGui::Text("Active: %zu / %zu", GetActiveCount(), pickups_.size());
		if (ImGui::Button("Spawn Near / Small")) {
			SpawnInZone(FieldZone::Near);
		}
		if (ImGui::Button("Spawn Middle / Medium")) {
			SpawnInZone(FieldZone::Middle);
		}
		if (ImGui::Button("Spawn Far / Large")) {
			SpawnInZone(FieldZone::Far);
		}

		ImGui::End();
#endif
	}
}
