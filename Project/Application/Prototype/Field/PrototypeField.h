#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "IGameObject.h"
#include "DebugParameter.h"
#include "ModelComponent.h"
#include "Vector3.h"
#include "Vector4.h"

namespace Prototype {

	/// <summary>
	/// フィールド中心からの距離で区切られるゲームプレイ領域
	/// </summary>
	enum class FieldZone : uint8_t {
		Center,
		Near,
		Middle,
		Far,
		Outside,
	};

	inline constexpr size_t kFieldZoneCount = 4;
	static_assert(static_cast<size_t>(FieldZone::Outside) == kFieldZoneCount);

	/// <summary>
	/// 表示と領域判定で共有するフィールド設定
	/// </summary>
	struct FieldSettings {
		Vector3 center = { 0.0f, 0.0f, 0.0f };
		// FieldZone と同じく、中心から外側へ向かう順番。
		std::array<float, kFieldZoneCount> radii = {
			3.0f,  // Center
			8.0f,  // Near
			14.0f, // Middle
			20.0f, // Far
		};
		std::array<Vector4, kFieldZoneCount> colors = {
			Vector4{ 0.86f, 0.92f, 1.00f, 1.0f }, // Center
			Vector4{ 0.28f, 0.68f, 0.48f, 1.0f }, // Near
			Vector4{ 0.86f, 0.62f, 0.24f, 1.0f }, // Middle
			Vector4{ 0.68f, 0.28f, 0.34f, 1.0f }, // Far
		};
		float layerHeight = 0.06f;
	};

	/// <summary>
	/// プロトタイプ用の円形フィールド
	/// </summary>
	class Field final : public GameEngine::IGameObject {
	public:
		explicit Field(GameEngine::Model* circleModel, const FieldSettings& settings = {});
		~Field() override = default;

		void Initialize() override;
		void Update() override;
		void DebugUpdate() override;
		void Draw() override;

		FieldZone GetZone(const Vector3& worldPosition) const;
		bool Contains(const Vector3& worldPosition) const;
		float GetRadius(FieldZone zone) const;

		const FieldSettings& GetSettings() const { return settings_; }

	private:
		void ApplySettings();

		FieldSettings settings_;
		std::array<std::unique_ptr<GameEngine::ModelComponent>, kFieldZoneCount> zoneModels_;
		std::unique_ptr<GameEngine::DebugParameter> debugParameter_;
	};
}
