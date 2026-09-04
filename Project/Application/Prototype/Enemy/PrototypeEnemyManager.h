#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "DebugParameter.h"
#include "IGameObject.h"

#include "PrototypeEnemy.h"

namespace Prototype {
	class Field;
	class EnergySpawner;
	class Rocket;
	class UnitManager;

	struct EnemyManagerSettings {
		float spawnInterval = 4.0f;
		int32_t initialCount = 3;
		int32_t maxActiveCount = 20;
		EnemySettings enemy;
	};

	/// <summary>
	/// フィールド最外周から敵を定期生成して管理する
	/// </summary>
	class EnemyManager final : public GameEngine::IGameObject {
	public:
		EnemyManager(
			GameEngine::Model* enemyModel,
			Field* field,
			Rocket* rocket,
			EnergySpawner* energySpawner,
			UnitManager* unitManager,
			size_t capacity = 64);
		~EnemyManager() override = default;

		void Initialize() override;
		void Update() override;
		void DebugUpdate() override;
		void Draw() override;
		void SetGameplayEnabled(bool enabled) { gameplayEnabled_ = enabled; }
		bool IsGameplayEnabled() const { return gameplayEnabled_; }

		bool SpawnOne();
		Enemy* FindNearestTargetable(const Vector3& position, float maxDistance) const;
		size_t GetActiveCount() const;
		size_t GetCarrierTargetCount() const;
		const EnemyManagerSettings& GetSettings() const { return settings_; }

	private:
		void ApplyDebugParameters();
		void SanitizeSettings();
		Vector3 MakeSpawnPosition() const;
		void DrawDebugWindow();

		Field* field_ = nullptr;
		std::vector<std::unique_ptr<Enemy>> enemies_;
		EnemyManagerSettings settings_;
		std::unique_ptr<GameEngine::DebugParameter> debugParameter_;
		float spawnTimer_ = 0.0f;
		bool gameplayEnabled_ = true;
	};
}
