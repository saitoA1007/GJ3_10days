#pragma once
#include <IGameObject.h>
#include <Vector2.h>
#include <DebugParameter.h>
#include <functional>
#include <utility>
#include "Enemy.h"
#include "EnemyRenderer.h"

class EnemyManager : public GameEngine::IGameObject {
public:

	EnemyManager(uint32_t maxEnemyNum, const GameEngine::Model* model);

	void Initialize() override;
	void Update() override;
	void Draw() override;

	void DebugUpdate() override;

	void SetStage(const std::string& stageName);
	void Pop(int num, Vector2 position, EnemyType type);

	int GetCurrentNum();

	void SetOnEnemyDefeated(std::function<void()> callback) {
		onEnemyDefeated_ = std::move(callback);
	}

private:

	void LoadPreset();

	const uint32_t maxEnemyNum_ = 0;

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

private:

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

	struct StageData {
		std::string name;
		std::vector<Preset> fases;
		float hpRatio = 1.0f;
		int minEnemyCount = 1;
	};

	std::unordered_map<std::string, Preset> stageDataMap_;
	StageData currentStageData_;
};
