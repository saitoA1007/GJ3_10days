#pragma once

#include <cstdint>
#include <memory>

#include "DebugParameter.h"
#include "IGameObject.h"

namespace Prototype {
	class EnemyManager;
	class EnergySpawner;
	class LockOnController;
	class Rocket;
	class UnitManager;

	/// @brief ゲーム開始前から結果表示までの進行状態。
	enum class GamePhase : uint8_t {
		Ready,     // 開始待機中。ゲームオブジェクトは停止
		Playing,   // 制限時間を減らしながら操作可能
		TimeUp,    // 時間切れ直後。最終エネルギーを確定済み
		Launching, // 将来実装するロケット発射演出用
		Result,    // 将来実装する結果表示用
	};

	/// @brief 1ゲームの時間に関する調整値。
	struct GameFlowSettings {
		float gameDuration = 60.0f; // 操作可能な制限時間（秒）
		float initialDelay = 3.0f;  // シーン開始からPlayingまでの待機時間（秒）
	};

	/// @brief プロトタイプの制限時間とゲームプレイ可否を一括管理する。
	class GameFlowController final : public GameEngine::IGameObject {
	public:
		/// @brief 時間切れ時に停止させる各ゲームシステムを受け取る。
		/// @param[in] rocket 最終エネルギーの取得先。
		/// @param[in] energySpawner エネルギー処理の停止対象。
		/// @param[in] enemyManager 敵処理の停止対象。
		/// @param[in] unitManager ユニット処理の停止対象。
		/// @param[in] lockOnController 入力処理の停止対象。
		/// @param[in] settings 制限時間と開始待機時間の初期設定。
		GameFlowController(
			Rocket* rocket,
			EnergySpawner* energySpawner,
			EnemyManager* enemyManager,
			UnitManager* unitManager,
			LockOnController* lockOnController,
			const GameFlowSettings& settings = {});
		~GameFlowController() override = default;

		/// @brief タイマーと最終値を初期化し、開始フェーズを決定する。
		void Initialize() override;

		/// @brief 現在フェーズに対応するタイマーを更新する。
		void Update() override;

		/// @brief Register設定とゲーム進行用ImGuiを更新する。
		void DebugUpdate() override;

		/// @brief 現在は通常描画を行わない。
		void Draw() override {}

		/// @brief ReadyまたはPlayingを即座に終了する。
		void ForceTimeUp();

		/// @brief 現在のゲームフェーズを取得する。
		/// @return 現在フェーズ。
		GamePhase GetPhase() const { return phase_; }

		/// @brief 操作可能なPlayingフェーズか判定する。
		/// @return Playingならtrue。
		bool IsPlaying() const { return phase_ == GamePhase::Playing; }

		/// @brief 時間切れフェーズか判定する。
		/// @return TimeUpならtrue。
		bool IsTimeUp() const { return phase_ == GamePhase::TimeUp; }

		/// @brief ImGuiによる一時停止中か取得する。
		/// @return 一時停止中ならtrue。
		bool IsDebugPaused() const { return debugPaused_; }

		/// @brief Playing終了までの残り秒数を取得する。
		/// @return 残り秒数。
		float GetRemainingTime() const { return remainingTime_; }

		/// @brief 制限時間に対する残り時間の割合を取得する。
		/// @return 0～1へ収めた残り時間割合。
		float GetRemainingRatio() const;

		/// @brief TimeUp時点で固定したロケットエネルギーを取得する。
		/// @return 最終エネルギー。
		int32_t GetFinalEnergy() const { return finalEnergy_; }

		/// @brief 現在のゲーム時間設定を取得する。
		/// @return ゲーム時間設定への参照。
		const GameFlowSettings& GetSettings() const { return settings_; }

	private:
		/// @brief Register値を反映して安全な範囲へ補正する。
		void ApplyDebugParameters();

		/// @brief 制限時間が0以下にならないよう補正する。
		void SanitizeSettings();

		/// @brief ReadyからPlayingへ移行して操作を許可する。
		void StartPlaying();

		/// @brief 最終値を保存してTimeUpへ移行する。
		void FinishPlaying();

		/// @brief 現在フェーズに合わせて各システムを一括停止・再開する。
		void ApplyGameplayState();

		/// @brief ImGui表示用のフェーズ名を取得する。
		/// @return 現在フェーズを表す文字列。
		const char* GetPhaseName() const;

		/// @brief 時間とフェーズ操作をImGuiへ表示する。
		void DrawDebugWindow();

		Rocket* rocket_ = nullptr;                                  // 最終エネルギーの取得先
		EnergySpawner* energySpawner_ = nullptr;                    // 生成・落下の停止対象
		EnemyManager* enemyManager_ = nullptr;                      // 敵生成・移動の停止対象
		UnitManager* unitManager_ = nullptr;                        // ユニット移動の停止対象
		LockOnController* lockOnController_ = nullptr;              // カーソル操作の停止対象
		GameFlowSettings settings_;                                 // Registerから編集される時間設定
		std::unique_ptr<GameEngine::DebugParameter> debugParameter_;// 設定とParameter Inspectorの接続
		GamePhase phase_ = GamePhase::Ready;                        // 現在のゲーム進行状態
		float initialDelayRemaining_ = 0.0f;                        // Ready終了までの残り秒数
		float remainingTime_ = 0.0f;                               // Playing終了までの残り秒数
		int32_t finalEnergy_ = 0;                                   // TimeUp時点で固定したロケットの値
		bool debugPaused_ = false;                                  // ImGuiによる一時停止中か
	};
}
