#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "DebugParameter.h"
#include "IGameObject.h"

#include "Application/Prototype/Field/PrototypeField.h"
#include "PrototypeEnergyPickup.h"

namespace Prototype {

	/// <summary>
	/// 
	/// </summary>
	struct EnergySpawnSettings {
		float spawnInterval = 2.5f;
		float fallHeight = 10.0f;
		float fallSpeed = 5.0f;
		float groundHeight = 0.25f;
		int32_t maxActiveCount = 30;
		int32_t initialCountPerZone = 1;
	};

	/// <summary>
	/// 各フィールド領域の空からエネルギーを定期生成する
	/// </summary>
	class EnergySpawner final : public GameEngine::IGameObject {
	public:
		EnergySpawner(
			GameEngine::Model* energyModel,
			Field* field,
			size_t capacity = 64);
		~EnergySpawner() override = default;

		void Initialize() override;
		void Update() override;
		void DebugUpdate() override;
		void Draw() override;
		void SetGameplayEnabled(bool enabled) { gameplayEnabled_ = enabled; }
		bool IsGameplayEnabled() const { return gameplayEnabled_; }

		bool SpawnInZone(FieldZone zone);
		EnergyPickup* SpawnOnGround(EnergySize size, const Vector3& position);
		EnergyPickup* FindNearestAvailable(const Vector3& position, float maxDistance);

		size_t GetActiveCount() const;
		size_t GetCapacity() const { return pickups_.size(); }
		const EnergySpawnSettings& GetSettings() const { return settings_; }

	private:
		void ApplyDebugParameters();
		void SanitizeSettings();
		void UpdatePickups(float deltaTime);
		void SpawnRandom();
		Vector3 MakeSpawnPosition(FieldZone zone) const;
		EnergySize GetEnergySize(FieldZone zone) const;
		void DrawDebugWindow();

		Field* field_ = nullptr;
		std::vector<std::unique_ptr<EnergyPickup>> pickups_;
		EnergySpawnSettings settings_;
		std::array<EnergyTypeSettings, kEnergySizeCount> typeSettings_ = {
			EnergyTypeSettings{ 0.45f, 10, { 1.00f, 0.88f, 0.20f, 1.0f } },
			EnergyTypeSettings{ 0.70f, 25, { 0.25f, 0.85f, 1.00f, 1.0f } },
			EnergyTypeSettings{ 1.00f, 50, { 0.92f, 0.35f, 1.00f, 1.0f } },
		};
		std::unique_ptr<GameEngine::DebugParameter> debugParameter_;
		float spawnTimer_ = 0.0f;
		bool gameplayEnabled_ = true;
	};
}
