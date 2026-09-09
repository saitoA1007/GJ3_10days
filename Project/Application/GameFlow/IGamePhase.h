#pragma once
#include <string>
#include <memory>

#include "Vector2.h"

#include <Application/Result/ResultStringManager.h>

namespace GameEngine
{
	class InputCommand;
}

class Rocket;
class EnergySpawner;
class EnemyManager;
class UnitManager;
class LockOnController;
class ResultMovieManager; // リザルトのムービー管理
class TutorialCameraModelView;
class StartPlayingView;
class HalfTimeView;
class TimeUI;
class UnitCountUI;

struct GameFlowSettings
{
	float openingDuration = 3.0f; // OP時間
	float startPlayingDuration = 3.0f; // プレイ開始演出時間
	float gameDuration = 60.0f;   // 制限時間
	float launchDuration = 3.0f;   // 打ち上げ時間
	Vector2 tutorialEnergyPositionXZ = { 0.0f, -6.0f }; // チュートリアル用EnergyのX・Z座標
	Vector2 tutorialMediumEnergyPositionXZ = { 0.0f, -6.0f }; // 長押し工程用Medium EnergyのX・Z座標
	Vector2 tutorialEnemyPositionXZ = { 0.0f, -18.0f }; // ロケット衝突工程用EnemyのX・Z座標
	Vector2 tutorialLockOnEnemyPositionXZ = { 0.0f, -12.0f }; // ロックオン工程用EnemyのX・Z座標
	Vector2 tutorialEnemyHoldPositionXZ = { 0.0f, -12.0f }; // 長押し工程用EnemyのX・Z座標
	float tutorialRequiredHoldDuration = 1.0f; // Medium Energyを長押しする必要時間
	float tutorialEnemyRequiredHoldDuration = 1.0f; // Enemyを長押しする必要時間
};

// 各システムへの参照を一括保持するコンテキスト
struct GameFlowContext
{
	Rocket* rocket = nullptr;
	EnergySpawner* energySpawner = nullptr;
	EnemyManager* enemyManager = nullptr;
	UnitManager* unitManager = nullptr;
	LockOnController* lockOnController = nullptr;
	GameEngine::InputCommand* inputCommand = nullptr;
	GameFlowSettings* settings = nullptr;
	ResultMovieManager* resultMovieManager_ = nullptr;
	TutorialCameraModelView* tutorialLogoView = nullptr;
	TutorialCameraModelView* tutorialLogo2View = nullptr;
	StartPlayingView* startPlayingView = nullptr;
	HalfTimeView* halfTimeView = nullptr;
	TimeUI* timeUI = nullptr;
	ResultStringManager* resultMessage = nullptr;
	UnitCountUI* unitCountUI = nullptr;

	int32_t finalEnergy = 0; // スコア用
};

// 各フェーズの基底インターフェース
class IGamePhase
{
public:
	virtual ~IGamePhase() = default;

	// フェーズ開始時
	virtual void OnEnter(GameFlowContext& context) {}

	// フレーム更新
	// 次のフェーズへ遷移する場合は true
	virtual bool OnUpdate(GameFlowContext& context) = 0;

	// フェーズ終了時
	virtual void OnExit(GameFlowContext& context) {}

	// デバッグ表示用
	virtual const char* GetName() const = 0;

	// このフェーズ中にプレイヤーのゲーム操作を許可するか
	virtual bool IsGameplayEnabled() const { return false; }

	// このフェーズ中に敵とEnergyの自動生成を許可するか
	virtual bool IsAutoSpawnEnabled() const { return false; }

	// このフェーズ中にGameSceneのメインカメラを使用するか
	virtual bool UsesGameSceneCamera() const { return true; }
};
