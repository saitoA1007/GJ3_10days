#pragma once
#include <IGameObject.h>
#include <Vector2.h>
#include <DebugParameter.h>
#include <functional>
#include <utility>
#include "Enemy.h"
#include "EnemyRenderer.h"

class Field;
class Rocket;
class EnergySpawner;
class UnitManager;
class EnemyEffectManager;

class EnemyManager : public GameEngine::IGameObject {
public:

	EnemyManager(uint32_t maxEnemyNum, const GameEngine::Model* model, EnemyEffectManager* effectManager);

	void Initialize() override;
	void Update() override;
	void Draw() override;

	void DebugUpdate() override;

	void SetStage(const std::string& stageName);
	void Pop(int num, Vector2 position, EnemyType type);

	void SetOnEnemyDefeated(std::function<void()> callback) {
		onEnemyDefeated_ = std::move(callback);
	}

	// 外部システムの参照をまとめて登録
	void SetContext(Field* field, Rocket* rocket, EnergySpawner* energySpawner, UnitManager* unitManager);
	// カーソル位置から最も近いターゲット可能な敵を検索
	Enemy* FindNearestTargetable(const Vector3& position, float maxDistance) const;

	// ゲームプレイの更新有効フラグ切替
	void SetGameplayEnabled(bool enabled) { gameplayEnabled_ = enabled; }
	bool IsGameplayEnabled() const { return gameplayEnabled_; }

	// 現在アクティブな敵の数を取得
	size_t GetActiveCount() const { return activeEnemies_.size(); }
	int GetCurrentNum() const { return static_cast<int>(activeEnemies_.size()); }

	// 運搬ユニットを追跡中の敵の数を取得
	size_t GetCarrierTargetCount() const;

private:

	void LoadPreset();

	const uint32_t maxEnemyNum_ = 0;

	// 外部参照コンテキスト
	Enemy::Context context_;

	std::vector<int> freeEnemyIndices_;
	GameEngine::WorldTransforms worldTransforms_;

	//敵のプール
	std::vector<std::unique_ptr<Enemy>> enemies_;
	//アクティブな敵のマップ
	std::map<int, Enemy*> activeEnemies_;
	//死亡したとき専用アクションを起こす用のマップ
	std::map<int, Enemy*> deadEnemies_;
	std::function<void()> onEnemyDefeated_;

	EnemyRenderer renderer_ = EnemyRenderer(renderQueue_);

	float popTimer_ = 0.0f;
	bool gameplayEnabled_ = true;

private:
	// 演出管理
	EnemyEffectManager* effectManager_ = nullptr;

	GameEngine::DebugParameter debugParam_{ "EnemyManager" };

	bool debugPop_ = false;
	float popInterval_ = 3.0f;

	//固有の敵の設定
	float swingWidth_ = 1.0f;
	float snakeSpeed_ = 2.0f;
	float roundSpeed_ = 2.0f;

	//共通の設定
	std::vector<Enemy::Config> configList_;
	std::vector<std::string> enemyTypeNames_ = {
		"Straight_S",
		"Straight_M",
		"Straight_L",
		"Round",
		"Snake"
	};

	EnemyType currentType_ = EnemyType::Straight_S;

	const char* enemyTypeNamesForImGuiList_[static_cast<int>(EnemyType::Count)];

	float collisionRadius_ = 1.0f;

private:

	struct Preset {
		std::vector<std::vector<Transform>> enemyPositions; // 敵の出現位置のリスト
	};

	struct StagePreset {
		std::string name;
		float time = 0.0f;
		float rotation = 0.0f;
	};

	struct StageData {
		std::string name;
		std::vector<StagePreset> fases;
		float hpRatio = 1.0f;
		int minEnemyCount = 1;
	};

	std::unordered_map<std::string, EnemyType> typeMap_;
	std::unordered_map<std::string, Preset> presetDataMap_;
	std::unordered_map<std::string, StageData> stageDataMap_;

	std::string currentStageName_;
	float stageTimer_ = 0.0f;
	int currentFaseIndex_ = 0;
};
