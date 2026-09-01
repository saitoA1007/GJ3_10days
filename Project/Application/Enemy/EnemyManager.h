#pragma once
#include <IGameObject.h>
#include <Vector2.h>
#include <DebugParameter.h>
#include "Enemy.h"
#include "EnemyRenderer.h"

class EnemyManager : public GameEngine::IGameObject {
public:

	EnemyManager(uint32_t maxEnemyNum, const GameEngine::Model* model);

	void Initialize() override;
	void Update() override;
	void Draw() override;

	void Pop(int num, Vector2 position);

	int GetCurrentNum();

private:

	const uint32_t maxEnemyNum_ = 0;

	std::vector<int> freeEnemyIndices_;
	GameEngine::WorldTransforms worldTransforms_;

	//敵のプール
	std::vector<std::unique_ptr<Enemy>> enemies_;
	//アクティブな敵のマップ
	std::map<int, Enemy*> activeEnemies_;
	//死亡したとき専用アクションを起こす用のマップ
	std::map<int, Enemy*> deadEnemies_;

	EnemyRenderer renderer_ = EnemyRenderer(renderQueue_);

	float popTimer_ = 0.0f;

private:

	GameEngine::DebugParameter debugParam_{ "EnemyManager" };

	bool debugPop_ = false;
	Enemy::Config enemyConfig_;
	float popInterval_ = 3.0f;
};
