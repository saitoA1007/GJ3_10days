#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "DebugParameter.h"
#include "IGameObject.h"

#include "PrototypeUnit.h"

namespace Prototype {
	class EnergyPickup;
	class Rocket;

	struct UnitManagerSettings {
		int32_t unitCount = 5;
		UnitSettings unit;
	};

	/// <summary>
	/// ロケット内の待機ユニットと出撃中ユニットをまとめて管理する
	/// </summary>
	class UnitManager final : public GameEngine::IGameObject {
	public:
		UnitManager(
			GameEngine::Model* unitModel,
			Rocket* rocket,
			size_t capacity = 16);
		~UnitManager() override = default;

		void Initialize() override;
		void Update() override;
		void DebugUpdate() override;
		void Draw() override;

		bool DispatchToEnergy(EnergyPickup* target, int32_t requestedEnergy);
		void RecallAll();

		size_t GetAvailableCount() const;
		size_t GetDeployedCount() const;
		size_t GetUnitCount() const { return static_cast<size_t>(settings_.unitCount); }
		const UnitManagerSettings& GetSettings() const { return settings_; }

	private:
		void ApplyDebugParameters();
		void SanitizeSettings();
		void ApplyUnitCount();
		void DrawDebugWindow();

		Rocket* rocket_ = nullptr;
		std::vector<std::unique_ptr<Unit>> units_;
		UnitManagerSettings settings_;
		std::unique_ptr<GameEngine::DebugParameter> debugParameter_;
	};
}
