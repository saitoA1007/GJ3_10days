#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#include "Collider.h"
#include "DebugParameter.h"
#include "EasingManager.h"
#include "IGameObject.h"
#include "ModelComponent.h"
#include "Vector3.h"

#include "RocketEnergy.h"

// 前方宣言
class FieldEffect; // ロケットに対してアクションがあった時のフィールドの演出

/// @brief ロケットの配置とエネルギー消費に関する調整値。
struct RocketSettings 
{
	Vector3 scale = { 1.0f, 1.0f, 1.0f };     // Rocket.gltfの表示倍率
	Vector3 rotation = { 0.0f, 0.0f, 0.0f };  // Rocket.gltfの回転（ラジアン）
	bool entranceEnabled = true;                // GameScene開始時の登場移動を再生するか
	Vector3 entranceStartPosition = { 0.0f, 30.25f, 0.0f }; // 登場移動のA座標
	Vector3 position = { 0.0f, 0.25f, 0.0f }; // 登場移動のB座標と、完了後のワールド座標
	float entranceDuration = 2.0f;              // A座標からB座標までの移動時間（秒）
	EaseType entranceEaseType = EaseType::kEaseOutCubic; // 登場移動の補間方法
	float colliderRadius = 1.5f;                // 敵到達判定の球半径
	float colliderOffsetY = 1.75f;              // モデル原点から球中心までのY差
	int32_t initialEnergy = 0;                   // シーン開始時の保有量
	int32_t requiredEnergy = 100;
	int32_t enemyHitLoss = 10;                   // 敵1体の到達で失う量
	float deliveryScaleMultiplier = 1.2f;       // Energy納品時に到達する表示倍率
	float enemyHitScaleMultiplier = 0.8f;       // 敵追突時に到達する表示倍率
	float scaleAnimationDuration = 0.25f;        // 1回拡縮して通常サイズへ戻るまでの秒数
	int32_t scaleAnimationRepeatCount = 3;       // 1イベントで拡縮を繰り返す回数
	int32_t debugEnergyAmount = 10;              // ImGuiの増減ボタンで使う量
};

/// @brief フィールド中央に配置するプロトタイプ用ロケット。
class Rocket final : public GameEngine::IGameObject 
{
public:
	/// @brief エネルギー変化をUIや演出へ通知するコールバック。
	using EnergyChangedCallback = std::function<void(const EnergyChange&)>;

	/// @brief ロケットモデル、衝突判定、エネルギー管理を準備する。
	/// @param[in] model ロケットの描画モデル。
	/// @param[in] settings 配置・当たり判定・エネルギーの初期設定。
	explicit Rocket(GameEngine::Model* model, FieldEffect* fieldEffect, const RocketSettings& settings = {});
	~Rocket() override = default;

	/// @brief エネルギーを初期値へ戻し、モデルと当たり判定を同期する。
	void Initialize() override;

	/// @brief Register設定をモデルと当たり判定へ反映する。
	void Update() override;

	/// @brief 停止中の見た目とデバッグUIを更新する。
	void DebugUpdate() override;

	/// @brief ロケットモデルを描画する。
	void Draw() override;

	/// @brief 登場移動をA座標から再生し直す。
	void StartEntrance();

	/// @brief 登場移動が再生中か取得する。
	/// @return A座標からB座標へ移動中ならtrue。
	bool IsEntrancePlaying() const { return isEntrancePlaying_; }

	/// @brief イージングを反映した登場移動の進行率を取得する。
	/// @return 開始時は0、完了時または演出無効時は1。
	float GetEntranceProgress() const;

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

	/// @brief シーン初期化後に敵がロケットへ到達した回数を取得する。
	uint64_t GetEnemyHitCount() const { return enemyHitCount_; }

	/// @brief エネルギーをInitialEnergyへ戻す。
	void ResetEnergy();

	/// @brief 現在の保有エネルギーを取得する。
	/// @return 現在値。
	int32_t GetEnergy() const { return energy_.GetCurrent(); }

	/// @brief 使用できるエネルギーが残っているか判定する。
	/// @return 1以上ならtrue。
	bool HasEnergy() const { return !energy_.IsEmpty(); }

	/// @brief ロケットの現在のワールド座標を取得する。
	/// @return 登場移動を反映した現在座標への参照。
	const Vector3& GetPosition() const { return currentPosition_; }

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

	/// @brief クリア・打ち上げに必要な目標エネルギー量を取得
	int32_t GetRequiredEnergy() const { return settings_.requiredEnergy; }

private:
	/// @brief Register値を反映して安全な範囲へ補正する。
	void ApplyDebugParameters();

	/// @brief スケール・半径・エネルギー設定の不正値を補正する。
	void SanitizeSettings();

	/// @brief 登場移動を経過時間だけ進める。
	/// @param[in] deltaTime 前フレームからの経過秒数。
	void UpdateEntrance(float deltaTime);

	/// @brief 納品・被ダメージ時のScaleアニメーションを開始する。
	/// @param[in] peakMultiplier アニメーション中間点でのScale倍率。
	void StartScaleAnimation(float peakMultiplier);

	/// @brief Scaleアニメーションを経過時間だけ進める。
	/// @param[in] deltaTime 前フレームからの経過秒数。
	void UpdateScaleAnimation(float deltaTime);

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
	Vector3 currentPosition_ = {};                              // 登場移動を反映した現在座標
	float entranceElapsedTime_ = 0.0f;                          // 登場移動の経過秒数
	bool isEntrancePlaying_ = false;                            // A座標からB座標へ移動中か
	float scaleAnimationElapsedTime_ = 0.0f;                    // 拡縮アニメーションの経過秒数
	float scaleAnimationPeakMultiplier_ = 1.0f;                 // 拡縮の中間点で到達する倍率
	float currentScaleMultiplier_ = 1.0f;                       // 現在モデルへ適用する倍率
	bool isScaleAnimating_ = false;                             // 納品・被ダメージの拡縮中か
	RocketEnergy energy_;                                      // 値の加算・安全な消費を担当する小クラス
	std::unique_ptr<GameEngine::ModelComponent> modelComponent_;// Rocket.gltfの描画情報
	GameEngine::SphereCollider collider_;                       // 敵の到達検出に使う球Collider
	std::unique_ptr<GameEngine::DebugParameter> debugParameter_;// 設定とParameter Inspectorの接続
	EnergyChangedCallback onEnergyChanged_;                     // UIや演出向けの任意通知先
	uint64_t enemyHitCount_ = 0;                                // Enemy到達通知の累計

	// ロケットにアクションがあった時のフィールドの演出
	FieldEffect* fieldEffect_ = nullptr;
};

