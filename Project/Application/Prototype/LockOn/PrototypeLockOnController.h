#pragma once

#include <cstdint>
#include <memory>

#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"
#include "Vector3.h"
#include "Vector4.h"

namespace GameEngine {
	class Camera;
	class DebugRenderer;
	class Input;
	class InputCommand;
}

namespace Prototype {
	class Enemy;
	class EnemyManager;
	class EnergyPickup;
	class EnergySpawner;
	class Field;
	class Rocket;
	class UnitManager;

	/// @brief カーソル、対象選択、長押しチャージに関する調整値。
	struct LockOnSettings {
		float cursorSpeed = 12.0f;                              // キー・スティック操作時の秒速
		float selectionRadius = 1.5f;                          // カーソル周囲の対象検索半径
		float groundHeight = 0.35f;                            // カーソルが移動するXZ平面のY座標
		float fieldEdgeMargin = 0.5f;                          // カーソルを最外周から内側へ制限する余白
		float maxLockOnSeconds = 3.0f;                         // 最大チャージへ到達する保持時間
		float chargeStartSeconds = 0.2f;                       // 単発クリックを0消費にする短押し猶予
		int32_t maxChargeEnergyCost = 30;                      // 最大チャージ時に要求するエネルギー
		float mouseMoveThreshold = 0.01f;                      // マウス移動として扱う最小デルタ
		Vector3 cursorModelScale = { 1.0f, 1.0f, 1.0f };       // cursor.objの表示倍率
		float cursorModelHeightOffset = 0.06f;                 // 地面への埋まりを防ぐ追加Y座標
		Vector4 cursorColor = { 0.25f, 1.00f, 0.45f, 1.0f };  // 通常カーソル色
		Vector4 targetColor = { 1.00f, 0.95f, 0.25f, 1.0f };  // 選択対象ガイド色
		Vector4 chargeColor = { 1.00f, 0.35f, 0.20f, 1.0f };  // チャージ量ガイド色
	};

	/// @brief 2D入力をXZ平面上のカーソルへ変換し、長押しロックオンでユニットを出撃させる。
	class LockOnController final : public GameEngine::IGameObject {
	public:
		/// @brief 入力をフィールド座標へ変換し、対象検索とユニット派遣を仲介する。
		/// @param[in] input マウス座標と移動量の取得元。
		/// @param[in] inputCommand デバイス共通コマンドの取得元。
		/// @param[in] camera マウスレイの逆変換に使うカメラ。
		/// @param[in] cursorModel カーソルの描画モデル。
		/// @param[in] debugRenderer 選択範囲とチャージ量の描画先。
		/// @param[in] field カーソル移動範囲の参照先。
		/// @param[in] rocket 派遣元かつエネルギー消費元。
		/// @param[in] energySpawner エネルギーの検索先。
		/// @param[in] enemyManager 敵の検索先。
		/// @param[in] unitManager ユニット派遣の依頼先。
		/// @param[in] settings カーソルとチャージの初期設定。
		LockOnController(
			GameEngine::Input* input,
			GameEngine::InputCommand* inputCommand,
			GameEngine::Camera* camera,
			GameEngine::Model* cursorModel,
			GameEngine::DebugRenderer* debugRenderer,
			Field* field,
			Rocket* rocket,
			EnergySpawner* energySpawner,
			EnemyManager* enemyManager,
			UnitManager* unitManager,
			const LockOnSettings& settings = {});
		~LockOnController() override = default;

		/// @brief カーソルをロケット位置へ戻し、選択とチャージを解除する。
		void Initialize() override;

		/// @brief カーソル、対象選択、チャージ入力を更新する。
		void Update() override;

		/// @brief 停止中の見た目とデバッグ表示を更新する。
		void DebugUpdate() override;

		/// @brief ゲームプレイ中だけカーソルモデルを描画する。
		void Draw() override;

		/// @brief 操作の有効状態を切り替え、無効化時はロックオンを解除する。
		/// @param[in] enabled 有効にするならtrue。
		void SetGameplayEnabled(bool enabled);

		/// @brief ゲームプレイ操作が有効か取得する。
		/// @return 有効ならtrue。
		bool IsGameplayEnabled() const { return gameplayEnabled_; }

		/// @brief 現在のカーソル座標を取得する。
		/// @return カーソル座標への参照。
		const Vector3& GetCursorPosition() const { return cursorPosition_; }

		/// @brief 現在選択中のエネルギーを取得する。
		/// @return 選択中のエネルギー。未選択ならnullptr。
		EnergyPickup* GetSelectedEnergy() const { return selectedEnergy_; }

		/// @brief 現在選択中の敵を取得する。
		/// @return 選択中の敵。未選択ならnullptr。
		Enemy* GetSelectedEnemy() const { return selectedEnemy_; }

		/// @brief 現在の入力保持時間を取得する。
		/// @return 入力を保持した秒数。
		float GetLockOnSeconds() const { return lockOnSeconds_; }

		/// @brief ロックオン入力を保持中か取得する。
		/// @return チャージ中ならtrue。
		bool IsCharging() const { return isCharging_; }

	private:
		/// @brief Register値を反映して安全な範囲へ補正する。
		void ApplyDebugParameters();

		/// @brief 半径・時間・スケールの不正値を補正する。
		void SanitizeSettings();

		/// @brief マウスまたは2D入力からカーソル位置を更新する。
		/// @param[in] deltaTime 前フレームからの経過秒数。
		void UpdateCursor(float deltaTime);

		/// @brief スクリーン座標のマウスを地面へレイ投影する。
		/// @return 地面との交点を計算できた場合はtrue。
		bool TrySetCursorFromMouse();

		/// @brief カーソルを最外周円の内側へ収める。
		void ClampCursorToField();

		/// @brief 論理位置と色をcursor.objへ反映する。
		void SyncCursorModel();

		/// @brief 範囲内で最も近いEnergyまたはEnemyを選ぶ。
		void UpdateSelection();

		/// @brief 有効な対象に対するチャージを開始する。
		void StartLockOn();

		/// @brief 長押し時間を加算し、入力を離したときに派遣する。
		/// @param[in] deltaTime 前フレームからの経過秒数。
		void UpdateLockOn(float deltaTime);

		/// @brief チャージ量を消費要求へ変換してユニットを派遣する。
		void CompleteLockOn();

		/// @brief 対象とチャージ状態を破棄する。
		void CancelLockOn();

		/// @brief 短押し猶予を除いたチャージ率を計算する。
		/// @return 0～1へ収めたチャージ率。
		float CalculateChargeRatio() const;

		/// @brief チャージ率から整数の消費要求量を計算する。
		/// @return 小数点以下を切り捨てた消費要求量。
		int32_t CalculateRequestedEnergy() const;

		/// @brief 現在の対象がまだ派遣可能か調べる。
		/// @return EnergyまたはEnemyのどちらかが有効ならtrue。
		bool HasValidSelection() const;

		/// @brief 選択解除と強調表示を一括で切り替える。
		/// @param[in] energy 新しく選択するエネルギー。選ばない場合はnullptr。
		/// @param[in] enemy 新しく選択する敵。選ばない場合はnullptr。
		void SetSelection(EnergyPickup* energy, Enemy* enemy);

		/// @brief 検索範囲・選択対象・チャージ量をデバッグ描画する。
		void DrawLockOnGuide();

		/// @brief 選択・チャージ状態をImGuiへ表示する。
		void DrawDebugWindow();

		GameEngine::Input* input_ = nullptr;                           // マウス座標と移動量の取得元
		GameEngine::InputCommand* inputCommand_ = nullptr;             // デバイス共通コマンドの取得元
		GameEngine::Camera* camera_ = nullptr;                         // マウスレイの逆変換に使うカメラ
		std::unique_ptr<GameEngine::ModelComponent> cursorModel_;      // cursor.objの描画情報
		GameEngine::DebugRenderer* debugRenderer_ = nullptr;           // ロックオン範囲のデバッグ描画先
		Field* field_ = nullptr;                                       // カーソル移動範囲の参照先
		Rocket* rocket_ = nullptr;                                     // 派遣線の始点と消費元
		EnergySpawner* energySpawner_ = nullptr;                       // 選択可能なEnergyの検索先
		EnemyManager* enemyManager_ = nullptr;                         // 選択可能なEnemyの検索先
		UnitManager* unitManager_ = nullptr;                           // 派遣処理の依頼先
		LockOnSettings settings_;                                      // Registerから編集される設定
		std::unique_ptr<GameEngine::DebugParameter> debugParameter_;   // 設定とParameter Inspectorの接続
		Vector3 cursorPosition_ = {};                                  // フィールドXZ平面上のカーソル座標
		EnergyPickup* selectedEnergy_ = nullptr;                       // 現在選択中のEnergy
		Enemy* selectedEnemy_ = nullptr;                               // 現在選択中のEnemy
		float lockOnSeconds_ = 0.0f;                                  // 今回の入力を保持した秒数
		bool isCharging_ = false;                                      // ロックオン入力を保持中か
		bool gameplayEnabled_ = true;                                 // Ready・TimeUp・Pause中はfalse
	};
}
