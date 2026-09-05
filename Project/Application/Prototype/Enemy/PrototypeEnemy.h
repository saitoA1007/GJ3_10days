#pragma once

#include <cstdint>
#include <memory>

#include "ModelComponent.h"
#include "Vector3.h"
#include "Vector4.h"

namespace GameEngine {
	class Model;
	class RenderQueue;
}

namespace Prototype {
	enum class EnergySize : uint8_t;
	class EnergyPickup;
	class EnergySpawner;
	class Field;
	class Rocket;
	class Unit;
	class UnitManager;

	/// @brief 敵1体の見た目・移動・索敵・当たり判定に関する調整値。
	struct EnemySettings {
		Vector3 scale = { 1.0f, 1.0f, 1.0f };             // モデルの表示倍率
		Vector4 color = { 1.0f, 0.35f, 0.35f, 1.0f };     // 通常時のモデル色
		float groundHeight = 0.25f;                        // XZ地面上に配置するY座標
		float moveSpeed = 2.0f;                            // 1秒あたりの移動距離
		float searchRadius = 8.0f;                         // 運搬中ユニットを探す範囲
		float collisionRadius = 0.8f;                      // XZ平面上の円形当たり判定半径
	};

	/// @brief ロケットまたは索敵した運搬ユニットへ向かうプロトタイプ用の敵。
	class Enemy final {
	public:
		/// @brief 敵が参照する各管理オブジェクトを受け取り、描画モデルを準備する。
		/// @param[in] model 敵の描画モデル。
		/// @param[in] field 撃破位置の距離帯を判定するフィールド。
		/// @param[in] rocket 通常時に目指すロケット。
		/// @param[in] energySpawner 撃破時のエネルギー生成先。
		/// @param[in] unitManager 運搬ユニットの検索先。
		/// @param[in] settings EnemyManagerが所有する共有設定。
		Enemy(
			GameEngine::Model* model,
			Field* field,
			Rocket* rocket,
			EnergySpawner* energySpawner,
			UnitManager* unitManager,
			const EnemySettings* settings);

		/// @brief 指定位置に敵を再出現させる。
		/// @param[in] position 出現させるワールド座標。
		void Spawn(const Vector3& position);

		/// @brief 敵を非アクティブ状態へ戻す。
		void Reset();

		/// @brief 索敵・移動・衝突処理を進める。
		/// @param[in] deltaTime 前フレームからの経過秒数。
		void Update(float deltaTime);

		/// @brief 停止中も設定変更をモデルへ反映する。
		void RefreshVisual();

		/// @brief アクティブな敵だけ描画する。
		/// @param[in] renderQueue 描画命令の登録先。
		void Draw(GameEngine::RenderQueue* renderQueue);

		/// @brief フィールド上に存在するかを取得する。
		/// @return アクティブならtrue。
		bool IsActive() const { return isActive_; }

		/// @brief 新しい派遣ユニットから攻撃予約できるかを取得する。
		/// @return アクティブかつ未予約ならtrue。
		bool IsTargetable() const { return isActive_ && !isReservedForAttack_; }

		/// @brief 運搬ユニットを追跡中かを取得する。
		/// @return 運搬ユニットを追跡中ならtrue。
		bool IsTargetingCarrier() const { return targetUnit_ != nullptr; }

		/// @brief 現在のワールド座標を取得する。
		/// @return 現在位置への参照。
		const Vector3& GetPosition() const { return position_; }

		/// @brief XZ平面上の当たり判定半径を取得する。
		/// @return 当たり判定半径。
		float GetCollisionRadius() const { return settings_->collisionRadius; }

		/// @brief 選択ガイドに使用する最大表示スケールを取得する。
		/// @return XYZスケールの最大値。
		float GetDisplayScale() const;

		/// @brief 派遣ユニット用にこの敵を予約する。
		/// @return 予約できた場合はtrue。
		bool TryReserveForAttack();

		/// @brief ユニットが到達不能になった場合などに攻撃予約を解除する。
		void CancelAttackReservation();

		/// @brief 敵を消し、撃破位置に応じたエネルギーを生成する。
		/// @return 生成したエネルギー。プールが満杯ならnullptr。
		EnergyPickup* DefeatAndDropEnergy();

		/// @brief カーソル選択中の見た目を切り替える。
		/// @param[in] highlighted 選択中として表示するならtrue。
		void SetHighlighted(bool highlighted);

	private:
		/// @brief 撃破地点のフィールド領域からドロップサイズを決める。
		/// @return 撃破地点に対応するエネルギーサイズ。
		EnergySize GetDropEnergySize() const;

		/// @brief 有効な運搬ユニットを優先対象として選び直す。
		void UpdateTarget();

		/// @brief XZ平面上で対象へ接近する。
		/// @param[in] target 移動先のワールド座標。
		/// @param[in] deltaTime 前フレームからの経過秒数。
		void MoveTowards(const Vector3& target, float deltaTime);

		/// @brief 運搬ユニットとの接触を処理する。
		/// @return 運搬ユニットを倒した場合はtrue。
		bool TryHitTargetUnit();

		/// @brief ロケットとの接触と敵の消滅を処理する。
		/// @return ロケットへ到達した場合はtrue。
		bool TryHitRocket();

		/// @brief 2点間のXZ平面上の距離の二乗を求める。
		/// @param[in] a 1点目。
		/// @param[in] b 2点目。
		/// @return XZ距離の二乗。
		float DistanceSquaredXZ(const Vector3& a, const Vector3& b) const;

		/// @brief 論理位置・選択状態を描画モデルへ反映する。
		void SyncModel();

		Field* field_ = nullptr;                                     // 距離帯判定に使用するフィールド
		Rocket* rocket_ = nullptr;                                   // 通常時の移動対象
		EnergySpawner* energySpawner_ = nullptr;                     // 撃破時のドロップ生成先
		UnitManager* unitManager_ = nullptr;                         // 運搬ユニットの検索先
		const EnemySettings* settings_ = nullptr;                    // Managerが所有する共有設定
		std::unique_ptr<GameEngine::ModelComponent> modelComponent_; // 敵モデルの描画情報
		Unit* targetUnit_ = nullptr;                                 // 現在追跡している運搬ユニット。いなければnullptr
		Vector3 position_ = {};                                      // 敵のワールド座標
		bool isActive_ = false;                                      // オブジェクトプール内で使用中か
		bool isReservedForAttack_ = false;                           // 派遣ユニット1体に攻撃予約されているか
		bool isHighlighted_ = false;                                 // カーソル選択中か
	};
}
