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

	/// @brief フィールド中心からの距離で区切られるゲームプレイ領域。
	enum class FieldZone : uint8_t {
		Center,       // ロケットを配置する中心領域
		Near,         // Small Energyの生成領域
		NearBuffer,   // エネルギーを生成しないNearとMiddleの間
		Middle,       // Medium Energyの生成領域
		MiddleBuffer, // エネルギーを生成しないMiddleとFarの間
		Far,          // Large Energyの生成領域
		OuterBuffer,  // エネルギーを生成しない最外周。敵の出現位置
		Outside,      // フィールド外
	};

	// Outsideを除いた、描画する円形モデルの枚数。
	inline constexpr size_t kFieldZoneCount = 7;
	static_assert(static_cast<size_t>(FieldZone::Outside) == kFieldZoneCount);

	/// @brief 表示と領域判定で共有するフィールド設定。
	struct FieldSettings {
		Vector3 center = { 0.0f, 0.0f, 0.0f }; // すべての円が共有する中心座標
		// FieldZone と同じく、中心から外側へ向かう順番。
		std::array<float, kFieldZoneCount> radii = {
			5.0f,  // Center
			10.0f, // Near
			15.0f, // NearBuffer
			20.0f, // Middle
			25.0f, // MiddleBuffer
			30.0f, // Far
			35.0f, // OuterBuffer
		};
		std::array<Vector4, kFieldZoneCount> colors = {
			Vector4{ 0.86f, 0.92f, 1.00f, 1.0f }, // Center
			Vector4{ 0.28f, 0.68f, 0.48f, 1.0f }, // Near
			Vector4{ 0.16f, 0.24f, 0.22f, 1.0f }, // NearBuffer
			Vector4{ 0.86f, 0.62f, 0.24f, 1.0f }, // Middle
			Vector4{ 0.28f, 0.22f, 0.16f, 1.0f }, // MiddleBuffer
			Vector4{ 0.68f, 0.28f, 0.34f, 1.0f }, // Far
			Vector4{ 0.24f, 0.16f, 0.18f, 1.0f }, // OuterBuffer
		};
		float layerHeight = 0.06f; // 重なりを避けるため、内側の円を持ち上げる高さ差
	};

	/// @brief プロトタイプ用の7層円形フィールド。
	class Field final : public GameEngine::IGameObject {
	public:
		/// @brief 同じ円モデルを7枚用意し、各領域の半径と色を登録する。
		/// @param[in] circleModel XZ平面上の円形モデル。
		/// @param[in] settings 中心、半径、色、高さ差の初期設定。
		explicit Field(GameEngine::Model* circleModel, const FieldSettings& settings = {});
		~Field() override = default;

		/// @brief 初期設定を全円モデルへ反映する。
		void Initialize() override;

		/// @brief Register変更があれば全円モデルへ反映する。
		void Update() override;

		/// @brief デバッグ停止中もRegister変更を反映する。
		void DebugUpdate() override;

		/// @brief 外側から内側の順に7枚の円を描画する。
		void Draw() override;

		/// @brief 中心からのXZ距離に対応する領域を取得する。
		/// @param[in] worldPosition 判定するワールド座標。
		/// @return 所属領域。最外周の外側ならOutside。
		FieldZone GetZone(const Vector3& worldPosition) const;

		/// @brief 座標が最外周円の内側か判定する。
		/// @param[in] worldPosition 判定するワールド座標。
		/// @return フィールド内ならtrue。
		bool Contains(const Vector3& worldPosition) const;

		/// @brief 指定領域の外周半径を取得する。
		/// @param[in] zone 半径を取得する領域。
		/// @return 領域の半径。Outsideなど範囲外なら0。
		float GetRadius(FieldZone zone) const;

		/// @brief 現在のフィールド設定を取得する。
		/// @return フィールド設定への参照。
		const FieldSettings& GetSettings() const { return settings_; }

	private:
		/// @brief 半径・色・重なり順を7枚のモデルへ反映する。
		void ApplySettings();

		FieldSettings settings_; // 表示と領域判定の両方で共有する設定
		std::array<std::unique_ptr<GameEngine::ModelComponent>, kFieldZoneCount> zoneModels_; // 外側から重ねる円モデル
		std::unique_ptr<GameEngine::DebugParameter> debugParameter_; // 半径・色とParameter Inspectorの接続
	};
}
