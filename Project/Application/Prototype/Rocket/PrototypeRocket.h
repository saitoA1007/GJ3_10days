#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#include "Collider.h"
#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"
#include "Vector3.h"

#include "RocketEnergy.h"

namespace Prototype {

	/// @brief ロケットの配置とエネルギー消費に関する調整値。
	struct RocketSettings {
		Vector3 position = { 0.0f, 0.25f, 0.0f }; // フィールド中央のワールド座標
		Vector3 scale = { 1.0f, 1.0f, 1.0f };     // rocket.objの表示倍率
		float colliderRadius = 1.5f;                // 敵到達判定の球半径
		float colliderOffsetY = 1.75f;              // モデル原点から球中心までのY差
		int32_t initialEnergy = 0;                   // シーン開始時の保有量
		int32_t enemyHitLoss = 10;                   // 敵1体の到達で失う量
		int32_t debugEnergyAmount = 10;              // ImGuiの増減ボタンで使う量
	};

	/// @brief フィールド中央に配置するプロトタイプ用ロケット。
	class Rocket final : public GameEngine::IGameObject {
	public:
		/// @brief エネルギー変化をUIや演出へ通知するコールバック。
		using EnergyChangedCallback = std::function<void(const EnergyChange&)>;

		/// @brief ロケットモデル、衝突判定、エネルギー管理を準備する。
		/// @param[in] model ロケットの描画モデル。
		/// @param[in] settings 配置・当たり判定・エネルギーの初期設定。
		explicit Rocket(GameEngine::Model* model, const RocketSettings& settings = {});
		~Rocket() override = default;

		/// @brief エネルギーを初期値へ戻し、モデルと当たり判定を同期する。
		void Initialize() override;

		/// @brief Register設定をモデルと当たり判定へ反映する。
		void Update() override;

		/// @brief 停止中の見た目とデバッグUIを更新する。
		void DebugUpdate() override;

		/// @brief ロケットモデルを描画する。
		void Draw() override;

		/// @brief ユニットが届けたエネルギーを加算する。
		/// @param[in] amount 加算する量。
		/// @return 加算前後と実際の差分。
		EnergyChange DepositEnergy(int32_t amount);

		/// @brief 保有量の範囲内でユニットのスタミナ用エネルギーを渡す。
		/// @param[in] requestedAmount ユニットからの要求量。
		/// @return 消費前後と実際に渡した負の差分。
		EnergyChange AllocateEnergyToUnit(int32_t requestedAmount);

		/// @brief 敵1体の到達によるエネルギー減少を適用する。
		/// @return 減少前後と実際の差分。
		EnergyChange ReceiveEnemyHit();

		/// @brief エネルギーをInitialEnergyへ戻す。
		void ResetEnergy();

		/// @brief 現在の保有エネルギーを取得する。
		/// @return 現在値。
		int32_t GetEnergy() const { return energy_.GetCurrent(); }

		/// @brief 使用できるエネルギーが残っているか判定する。
		/// @return 1以上ならtrue。
		bool HasEnergy() const { return !energy_.IsEmpty(); }

		/// @brief ロケットのワールド座標を取得する。
		/// @return 設定座標への参照。
		const Vector3& GetPosition() const { return settings_.position; }

		/// @brief 現在のロケット設定を取得する。
		/// @return ロケット設定への参照。
		const RocketSettings& GetSettings() const { return settings_; }

		/// @brief ロケットの球Colliderを取得する。
		/// @return Colliderへの参照。
		GameEngine::SphereCollider& GetCollider() { return collider_; }

		/// @brief 実際にエネルギーが増減した際の通知先を設定する。
		/// @param[in] callback 増減結果を受け取る関数。
		void SetOnEnergyChanged(EnergyChangedCallback callback) {
			onEnergyChanged_ = std::move(callback);
		}

	private:
		/// @brief Register値を反映して安全な範囲へ補正する。
		void ApplyDebugParameters();

		/// @brief スケール・半径・エネルギー設定の不正値を補正する。
		void SanitizeSettings();

		/// @brief 設定位置をモデルとColliderへ反映する。
		void SyncComponents();

		/// @brief 変化がある場合だけ登録済みコールバックを呼ぶ。
		/// @param[in] change エネルギーの増減結果。
		void NotifyEnergyChanged(const EnergyChange& change);

		/// @brief Collider経由の敵接触を処理する。
		/// @param[in] result 接触相手の情報。
		void OnCollisionEnter(const GameEngine::CollisionResult& result);

		/// @brief 現在値と手動増減ボタンをImGuiへ表示する。
		void DrawDebugWindow();

		RocketSettings settings_;                                  // Registerから編集される設定
		RocketEnergy energy_;                                      // 値の加算・安全な消費を担当する小クラス
		std::unique_ptr<GameEngine::ModelComponent> modelComponent_;// rocket.objの描画情報
		GameEngine::SphereCollider collider_;                       // 敵の到達検出に使う球Collider
		std::unique_ptr<GameEngine::DebugParameter> debugParameter_;// 設定とParameter Inspectorの接続
		EnergyChangedCallback onEnergyChanged_;                     // UIや演出向けの任意通知先
	};
}
