#pragma once
#include <string>
#include <memory>

class Rocket;
class EnergySpawner;
class EnemyManager;
class UnitManager;
class LockOnController;

struct GameFlowSettings
{
	float openingDuration = 3.0f; // OP時間
	float gameDuration = 60.0f;   // 制限時間
	float launchDuration = 3.0f;   // 打ち上げ時間
};

// 各システムへの参照を一括保持するコンテキスト
struct GameFlowContext
{
	Rocket* rocket = nullptr;
	EnergySpawner* energySpawner = nullptr;
	EnemyManager* enemyManager = nullptr;
	UnitManager* unitManager = nullptr;
	LockOnController* lockOnController = nullptr;
	GameFlowSettings* settings = nullptr;

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
};