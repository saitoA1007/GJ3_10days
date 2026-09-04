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

	/// @brief 敵全体の生成数と、全個体で共有する敵設定。
	struct EnemyManagerSettings {
		float spawnInterval = 4.0f;  // 通常生成の間隔（秒）
		int32_t initialCount = 3;     // ゲーム開始時に生成する数
		int32_t maxActiveCount = 20;  // 同時に存在できる上限
		EnemySettings enemy;          // 全個体が参照する共通設定
	};

	/// @brief フィールド最外周から敵を定期生成して管理する。
	class EnemyManager final : public GameEngine::IGameObject {
	public:
		/// @brief 固定数の敵をプールとして確保し、依存オブジェクトを各個体へ渡す。
		/// @param[in] enemyModel 敵の描画モデル。
		/// @param[in] field 出現位置とドロップ距離帯の参照先。
		/// @param[in] rocket 敵の最終移動対象。
		/// @param[in] energySpawner 撃破時のドロップ生成先。
		/// @param[in] unitManager 運搬ユニットの検索先。
		/// @param[in] capacity プールへ確保する最大個体数。
		EnemyManager(
			GameEngine::Model* enemyModel,
			Field* field,
			Rocket* rocket,
			EnergySpawner* energySpawner,
			UnitManager* unitManager,
			size_t capacity = 64);
		~EnemyManager() override = default;

		/// @brief 全個体をリセットし、初期数を最外周へ生成する。
		void Initialize() override;

		/// @brief 敵の移動と定期生成を更新する。
		void Update() override;

		/// @brief 停止中の見た目とデバッグUIを更新する。
		void DebugUpdate() override;

		/// @brief プール内のアクティブな敵を描画する。
		void Draw() override;

		/// @brief 敵の生成と移動の有効状態を切り替える。
		/// @param[in] enabled 有効にするならtrue。
		void SetGameplayEnabled(bool enabled) { gameplayEnabled_ = enabled; }

		/// @brief ゲームプレイ処理が有効か取得する。
		/// @return 有効ならtrue。
		bool IsGameplayEnabled() const { return gameplayEnabled_; }

		/// @brief 未使用の個体を最外周へ1体生成する。
		/// @return 生成できた場合はtrue。
		bool SpawnOne();

		/// @brief 指定位置から範囲内にいる、未予約で最も近い敵を探す。
		/// @param[in] position 検索中心のワールド座標。
		/// @param[in] maxDistance 検索する最大距離。
		/// @return 条件を満たす最も近い敵。存在しなければnullptr。
		Enemy* FindNearestTargetable(const Vector3& position, float maxDistance) const;

		/// @brief 現在アクティブな敵の数を取得する。
		/// @return アクティブ数。
		size_t GetActiveCount() const;

		/// @brief 運搬ユニットを追跡中の敵の数を取得する。
		/// @return 運搬ユニット追跡中の敵数。
		size_t GetCarrierTargetCount() const;

		/// @brief 現在の敵管理設定を取得する。
		/// @return 敵管理設定への参照。
		const EnemyManagerSettings& GetSettings() const { return settings_; }

	private:
		/// @brief Register値を反映して安全な範囲へ補正する。
		void ApplyDebugParameters();

		/// @brief 個体数や速度などの不正値を補正する。
		void SanitizeSettings();

		/// @brief 最外周円上のランダム座標を作る。
		/// @return 敵の出現座標。
		Vector3 MakeSpawnPosition() const;

		/// @brief 稼働数と手動生成ボタンをImGuiへ表示する。
		void DrawDebugWindow();

		Field* field_ = nullptr;                                    // 外周半径と中心座標の参照先
		std::vector<std::unique_ptr<Enemy>> enemies_;               // 再利用する敵オブジェクトのプール
		EnemyManagerSettings settings_;                             // Registerから編集される共有設定
		std::unique_ptr<GameEngine::DebugParameter> debugParameter_;// 設定とParameter Inspectorの接続
		float spawnTimer_ = 0.0f;                                   // 次回生成までに経過した秒数
		bool gameplayEnabled_ = true;                               // Ready・TimeUp・Pause中はfalse
	};
}
