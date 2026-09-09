#pragma once

#include <cstdint>
#include <memory>

#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"
#include "Vector3.h"
#include "Vector4.h"

namespace GameEngine
{
	class Camera;
	class DebugRenderer;
	class Input;
	class InputCommand;
}

class Enemy;
class EnemyManager;
class EnergyPickup;
class EnergySpawner;
class Field;
class Rocket;
class Unit;
class UnitManager;

/// @brief カーソル、対象選択、長押しチャージに関する調整値。
struct LockOnSettings
{
	float cursorSpeed = 12.0f;                              // キー・スティック操作時の秒速
	float selectionRadius = 1.5f;                          // カーソル周囲の対象検索半径
	float groundHeight = 0.35f;                            // カーソルが移動するXZ平面のY座標
	float fieldEdgeMargin = 0.5f;                          // カーソルを最外周から内側へ制限する余白
	float maxLockOnSeconds = 3.0f;                         // 最大チャージへ到達する保持時間
	float chargeStartSeconds = 0.2f;                       // 単発クリックを0消費にする短押し猶予
	int32_t maxChargeEnergyCost = 30;                      // 最大チャージ時に要求するエネルギー
	float mouseMoveThreshold = 0.01f;                      // マウス移動として扱う最小デルタ
	Vector3 cursorModelScale = { 1.0f, 1.0f, 1.0f };       // SelectionRadiusへ掛ける軸別の表示補正倍率
	float cursorModelHeightOffset = 0.06f;                 // 地面への埋まりを防ぐ追加Y座標
	Vector4 cursorColor = { 0.25f, 1.00f, 0.45f, 1.0f };  // 通常カーソル色
	Vector4 targetColor = { 1.00f, 0.95f, 0.25f, 1.0f };  // 選択対象ガイド色
	Vector4 chargeColor = { 1.00f, 0.35f, 0.20f, 1.0f };  // チャージ量ガイド色
	Vector4 injectColor = { 1.0f, 0.8f, 0.2f, 1.0f };
	float injectRate = 20.0f;
};

/// @brief 2D入力をXZ平面上のカーソルへ変換し、長押しロックオンでユニットを出撃させる。
class LockOnController final : public GameEngine::IGameObject
{
public:
	/// @brief 入力をフィールド座標へ変換し、対象検索とユニット派遣を仲介する。
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

	/// @brief 離した際に派遣を成立させる最低ホールド秒数を設定する。
	void SetMinimumDispatchHoldSeconds(float seconds);

	/// @brief Enemyをロックオン候補に含めるか切り替える。
	void SetEnemySelectionEnabled(bool enabled);

	/// @brief チュートリアル中の操作対象を指定したEnergyまたはEnemyだけに制限する。
	/// @details 両方nullptrならカーソル移動以外の操作を禁止する。
	void SetTutorialAllowedTargets(EnergyPickup* energy, Enemy* enemy);

	/// @brief チュートリアル専用の操作制限を解除する。
	void ClearTutorialInteractionRestriction();

	/// @brief Text14中に長押し判定する静止Unitを設定する。
	void SetTutorialHoldTarget(Unit* unit);
	bool IsTutorialHoldTargetHovered() const { return tutorialHoldTargetHovered_; }

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

	/// @brief 論理位置、選択半径に連動するスケール、色をcursor.objへ反映する。
	void SyncCursorModel();

	void SyncChargeModel();

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

	/// @brief 最大チャージ中は通常色と赤色を交互に返す。
	/// @return 現在のチャージリング表示色。
	Vector4 GetChargeDisplayColor() const;

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

	GameEngine::Input* input_ = nullptr;                        
	GameEngine::InputCommand* inputCommand_ = nullptr;          
	GameEngine::Camera* camera_ = nullptr;                      
	std::unique_ptr<GameEngine::ModelComponent> cursorModel_;   
	std::unique_ptr<GameEngine::ModelComponent> chargeModel_;
	GameEngine::DebugRenderer* debugRenderer_ = nullptr;        
	Field* field_ = nullptr;                                    
	Rocket* rocket_ = nullptr;                                  
	EnergySpawner* energySpawner_ = nullptr;                    
	EnemyManager* enemyManager_ = nullptr;                      
	UnitManager* unitManager_ = nullptr;                        
	LockOnSettings settings_;                                   
	std::unique_ptr<GameEngine::DebugParameter> debugParameter_;
	Vector3 cursorPosition_ = {};                               
	EnergyPickup* selectedEnergy_ = nullptr;                    
	Enemy* selectedEnemy_ = nullptr;                            
	EnergyPickup* tutorialAllowedEnergy_ = nullptr;
	Enemy* tutorialAllowedEnemy_ = nullptr;
	Unit* tutorialHoldTarget_ = nullptr;
	float lockOnSeconds_ = 0.0f;                                
	float maxChargeBlinkElapsedTime_ = 0.0f;
	float minimumDispatchHoldSeconds_ = 0.0f;
	bool isCharging_ = false;                                   
	bool gameplayEnabled_ = true;  
	bool enemySelectionEnabled_ = true;
	int32_t chargedEnergy_ = 0;

	bool isInjecting_ = false;    
	float injectAnimTimer_ = 0.0f;
	float injectAccumulator_ = 0.0f;
	bool suppressLockOn_ = false;
	bool hasUnitInRadius_ = false;
	bool tutorialHoldTargetHovered_ = false;
	bool tutorialInteractionRestricted_ = false;
};

