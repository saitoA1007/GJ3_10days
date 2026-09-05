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
	class Enemy;
	class EnergyPickup;
	class Rocket;

	/// @brief ユニット1体の行動状態。
	enum class UnitState : uint8_t {
		Stored,            // ロケット内で待機中。再出撃可能
		MovingToEnergy,    // 予約したEnergyへ移動中
		MovingToEnemy,     // 予約したEnemyへ移動中
		ReturningToRocket, // Energyを頭上に載せて帰還中
	};

	/// @brief 全ユニットで共有する移動・スタミナ・見た目の設定。
	struct UnitSettings {
		Vector3 launchOffset = { 0.0f, 0.0f, 0.0f };       // ロケット位置から出撃位置までの差
		Vector3 scale = { 0.7f, 0.7f, 0.7f };              // unit.objの表示倍率
		Vector3 carryOffset = { 0.0f, 1.45f, 0.0f };       // ユニット位置から頭上Energyまでの差
		float normalSpeed = 1.5f;                          // スタミナ切れ時の秒速
		float boostedSpeed = 5.0f;                         // スタミナがある間の秒速
		float pickupRadius = 0.45f;                        // Energy回収が成立する距離
		float deliveryRadius = 1.6f;                       // ロケットへ納品が成立する距離
		float collisionRadius = 0.6f;                      // EnemyとのXZ平面上の当たり判定半径
		float staminaDrainPerSecond = 2.0f;                // 基本スタミナ消費量/秒
		float distanceDrainRate = 0.08f;                   // ロケットからの距離による消費倍率
		Vector4 normalColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // スタミナ切れ時の色
		Vector4 staminaColor = { 0.25f, 0.85f, 1.0f, 1.0f }; // スタミナがある間の水色
	};

	/// @brief エネルギー回収または敵攻撃へ派遣されるユニット。
	class Unit final {
	public:
		/// @brief 描画モデルと、エネルギー消費元となるロケットを受け取る。
		/// @param[in] model ユニットの描画モデル。
		/// @param[in] rocket 出撃位置・帰還先・エネルギー消費元。
		/// @param[in] settings UnitManagerが所有する共有設定。
		Unit(GameEngine::Model* model, Rocket* rocket, const UnitSettings* settings);

		/// @brief 待機状態と初期位置へ戻す。
		void Initialize();

		/// @brief 現在状態に対応する移動処理を進める。
		/// @param[in] deltaTime 前フレームからの経過秒数。
		void Update(float deltaTime);

		/// @brief 停止中も設定変更と運搬物位置を反映する。
		void RefreshVisual();

		/// @brief 出撃中のユニットを描画する。
		/// @param[in] renderQueue 描画命令の登録先。
		void Draw(GameEngine::RenderQueue* renderQueue);

		/// @brief エネルギーを予約して回収へ出撃する。
		/// @param[in] target 回収対象。
		/// @param[in] requestedEnergy スタミナへ割り当てる要求量。
		/// @return 対象を予約して出撃できた場合はtrue。
		bool DispatchToEnergy(EnergyPickup* target, int32_t requestedEnergy);

		/// @brief 敵を予約して攻撃へ出撃する。
		/// @param[in] target 攻撃対象。
		/// @param[in] requestedEnergy スタミナへ割り当てる要求量。
		/// @return 対象を予約して出撃できた場合はtrue。
		bool DispatchToEnemy(Enemy* target, int32_t requestedEnergy);

		/// @brief 運搬物をその場へ落とし、ユニットを待機状態へ戻す。
		/// @return 運搬中のユニットを倒せた場合はtrue。
		bool DefeatAndDropEnergy();

		/// @brief 予約を解放して強制的に待機状態へ戻す。
		void Recall();

		/// @brief 再出撃できる待機状態か判定する。
		/// @return Storedならtrue。
		bool IsAvailable() const { return state_ == UnitState::Stored; }

		/// @brief フィールド上へ出撃中か判定する。
		/// @return 移動または帰還状態ならtrue。
		bool IsDeployed() const {
			return state_ == UnitState::MovingToEnergy ||
				state_ == UnitState::MovingToEnemy ||
				state_ == UnitState::ReturningToRocket;
		}
		/// @brief エネルギーを持って帰還中か判定する。
		/// @return Carried状態のエネルギーを保持していればtrue。
		bool IsCarryingEnergy() const;

		/// @brief 現在の行動状態を取得する。
		/// @return 現在状態。
		UnitState GetState() const { return state_; }

		/// @brief 現在のワールド座標を取得する。
		/// @return 現在位置への参照。
		const Vector3& GetPosition() const { return position_; }

		/// @brief 敵との当たり判定半径を取得する。
		/// @return XZ平面上の当たり判定半径。
		float GetCollisionRadius() const { return settings_->collisionRadius; }

		/// @brief 現在のスタミナ残量を取得する。
		/// @return スタミナ残量。
		float GetStamina() const { return stamina_; }

		/// @brief 回収対象または運搬中のエネルギーを取得する。
		/// @return 対象エネルギー。存在しなければnullptr。
		EnergyPickup* GetTargetEnergy() const { return targetEnergy_; }

		/// @brief 現在の攻撃対象を取得する。
		/// @return 対象の敵。存在しなければnullptr。
		Enemy* GetTargetEnemy() const { return targetEnemy_; }

	private:
		/// @brief 対象へ接近し、範囲内で運搬を開始する。
		/// @param[in] deltaTime 前フレームからの経過秒数。
		void UpdateMovingToEnergy(float deltaTime);

		/// @brief 敵へ接近し、スタミナに応じて勝敗を処理する。
		/// @param[in] deltaTime 前フレームからの経過秒数。
		void UpdateMovingToEnemy(float deltaTime);

		/// @brief 運搬物を追従させ、ロケットへ納品する。
		/// @param[in] deltaTime 前フレームからの経過秒数。
		void UpdateReturningToRocket(float deltaTime);

		/// @brief ロケットが実際に渡せた量をスタミナへ変換する。
		/// @param[in] requestedEnergy スタミナへ変換する要求量。
		void AllocateStamina(int32_t requestedEnergy);

		/// @brief 倒れた個体を消費せず待機状態へ戻す。
		void ReturnToStorageAfterDefeat();

		/// @brief スタミナ状態に応じた速度でXZ平面上を移動する。
		/// @param[in] target 移動先のワールド座標。
		/// @param[in] deltaTime 前フレームからの経過秒数。
		void MoveTowards(const Vector3& target, float deltaTime);

		/// @brief ロケットからの距離に応じてスタミナを減らす。
		/// @param[in] deltaTime 前フレームからの経過秒数。
		void ConsumeStamina(float deltaTime);

		/// @brief 2点間のXZ平面上の距離の二乗を求める。
		/// @param[in] a 1点目。
		/// @param[in] b 2点目。
		/// @return XZ距離の二乗。
		float DistanceSquaredXZ(const Vector3& a, const Vector3& b) const;

		/// @brief 位置・向き・スタミナ色をモデルへ反映する。
		void SyncModel();

		Rocket* rocket_ = nullptr;                                   // 出撃位置・帰還先・Energy消費元
		const UnitSettings* settings_ = nullptr;                     // Managerが所有する共有設定
		std::unique_ptr<GameEngine::ModelComponent> modelComponent_; // unit.objの描画情報
		UnitState state_ = UnitState::Stored;                        // 現在の行動状態
		EnergyPickup* targetEnergy_ = nullptr;                       // 回収対象または運搬中のEnergy
		Enemy* targetEnemy_ = nullptr;                               // 攻撃対象のEnemy
		Vector3 position_ = {};                                      // 現在のワールド座標
		float stamina_ = 0.0f;                                      // 高速移動に使える残量
	};
}
