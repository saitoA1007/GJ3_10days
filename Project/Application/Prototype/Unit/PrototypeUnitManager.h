#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "DebugParameter.h"
#include "IGameObject.h"

#include "PrototypeUnit.h"

namespace Prototype {
	class Enemy;
	class EnergyPickup;
	class Rocket;

	/// @brief 使用するユニット数と、全個体で共有する設定。
	struct UnitManagerSettings {
		int32_t unitCount = 5; // プールからゲームに参加させる個体数
		UnitSettings unit;     // 全個体が参照する共通設定
	};

	/// @brief ロケット内の待機ユニットと出撃中ユニットをまとめて管理する。
	class UnitManager final : public GameEngine::IGameObject {
	public:
		/// @brief 再出撃可能な固定数のユニットをプールとして確保する。
		/// @param[in] unitModel ユニットの描画モデル。
		/// @param[in] rocket 全個体が共有する出撃位置・帰還先。
		/// @param[in] capacity プールへ確保する最大個体数。
		UnitManager(
			GameEngine::Model* unitModel,
			Rocket* rocket,
			size_t capacity = 16);
		~UnitManager() override = default;

		/// @brief 設定された参加数を適用し、全個体を待機状態へ戻す。
		void Initialize() override;

		/// @brief 参加中かつ出撃中のユニットを更新する。
		void Update() override;

		/// @brief 停止中の見た目とデバッグUIを更新する。
		void DebugUpdate() override;

		/// @brief 参加中のユニットを描画する。
		void Draw() override;

		/// @brief ユニット移動の有効状態を切り替える。
		/// @param[in] enabled 有効にするならtrue。
		void SetGameplayEnabled(bool enabled) { gameplayEnabled_ = enabled; }

		/// @brief ゲームプレイ処理が有効か取得する。
		/// @return 有効ならtrue。
		bool IsGameplayEnabled() const { return gameplayEnabled_; }

		/// @brief 待機ユニット1体をエネルギー回収へ派遣する。
		/// @param[in] target 回収対象。
		/// @param[in] requestedEnergy スタミナへ割り当てる要求量。
		/// @return 派遣できた場合はtrue。
		bool DispatchToEnergy(EnergyPickup* target, int32_t requestedEnergy);

		/// @brief 待機ユニット1体を敵への攻撃へ派遣する。
		/// @param[in] target 攻撃対象。
		/// @param[in] requestedEnergy スタミナへ割り当てる要求量。
		/// @return 派遣できた場合はtrue。
		bool DispatchToEnemy(Enemy* target, int32_t requestedEnergy);

		/// @brief 指定位置の索敵範囲内にいる、最も近い運搬ユニットを探す。
		/// @param[in] position 検索中心のワールド座標。
		/// @param[in] maxDistance 検索する最大距離。
		/// @return 最も近い運搬ユニット。存在しなければnullptr。
		Unit* FindNearestCarryingUnit(const Vector3& position, float maxDistance) const;

		/// @brief 全ユニットの予約と運搬物を解放して待機状態へ戻す。
		void RecallAll();

		/// @brief 現在待機中で再出撃できるユニット数を取得する。
		/// @return 待機ユニット数。
		size_t GetAvailableCount() const;

		/// @brief 現在出撃中のユニット数を取得する。
		/// @return 出撃ユニット数。
		size_t GetDeployedCount() const;

		/// @brief ゲームへ参加させるユニット数を取得する。
		/// @return 参加ユニット数。
		size_t GetUnitCount() const { return static_cast<size_t>(settings_.unitCount); }

		/// @brief 現在のユニット管理設定を取得する。
		/// @return ユニット管理設定への参照。
		const UnitManagerSettings& GetSettings() const { return settings_; }

	private:
		/// @brief Register値を反映して安全な範囲へ補正する。
		void ApplyDebugParameters();

		/// @brief 個数・速度・半径などの不正値を補正する。
		void SanitizeSettings();

		/// @brief 有効数から外れた出撃中ユニットを回収する。
		void ApplyUnitCount();

		/// @brief 待機数・出撃数と全回収ボタンをImGuiへ表示する。
		void DrawDebugWindow();

		Rocket* rocket_ = nullptr;                                  // 全個体が共有する帰還先
		std::vector<std::unique_ptr<Unit>> units_;                   // 再利用するユニットのプール
		UnitManagerSettings settings_;                              // Registerから編集される共有設定
		std::unique_ptr<GameEngine::DebugParameter> debugParameter_;// 設定とParameter Inspectorの接続
		bool gameplayEnabled_ = true;                               // Ready・TimeUp・Pause中はfalse
	};
}
