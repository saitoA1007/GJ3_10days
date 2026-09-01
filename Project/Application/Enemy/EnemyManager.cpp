#include "EnemyManager.h"
#include <LogManager.h>
#include <Application/Utils/ShigeFunc.h>
#include <RandomGenerator.h>

#include <numbers>
	
EnemyManager::EnemyManager(uint32_t maxEnemyNum, const GameEngine::Model* model) : maxEnemyNum_(maxEnemyNum) {
	renderer_.SetModel(model);
}

void EnemyManager::Initialize() {
	freeEnemyIndices_.clear();
	activeEnemies_.clear();
	deadEnemies_.clear();

	worldTransforms_.Initialize(maxEnemyNum_, {});

	enemies_.resize(maxEnemyNum_);
	freeEnemyIndices_.reserve(maxEnemyNum_);

	for (uint32_t i = 0; i < maxEnemyNum_; ++i) {
		if (!enemies_[i]) {
			enemies_[i] = std::make_unique<Enemy>(&worldTransforms_.transformDatas_[i]);
		}

		enemies_[i]->Initialize();
		freeEnemyIndices_.push_back(i);
	}

	renderer_.SetTransforms(&worldTransforms_);

	debugParam_.Register("Pop", debugPop_);
}

void EnemyManager::Update() {
	debugParam_.ApplyIfDirty();

#ifdef USE_IMGUI

	if (debugPop_) {
		debugPop_ = false;
		float range = RandomGenerator::Get(-20.f, 20.f);
		float theta = RandomGenerator::Get(0.f, 2.f * std::numbers::pi_v<float>);
		Pop(1, Vector2(range * std::cos(theta), range * std::sin(theta)));
	}

#endif

	for (const auto& [index, enemy] : deadEnemies_) {
		enemy->DeadUpdate();

		if (!enemy->IsActive()) {
			deadEnemies_.erase(index);
			freeEnemyIndices_.push_back(index);
		}
	}

	for (const auto& [index, enemy] : activeEnemies_) {
		enemy->Update();

		if (enemy->IsDead()) {
			// 敵が死亡した場合、アクティブリストから削除し、フリーリストに戻す
			deadEnemies_[index] = enemy;
			activeEnemies_.erase(index);
		}
	}

	worldTransforms_.UpdateTransformMatrix(maxEnemyNum_);
}

void EnemyManager::Draw() {
	renderer_.Draw();
}

void EnemyManager::Pop(int num, Vector2 position) {
	for (int i = 0; i < num; ++i) {
		if (freeEnemyIndices_.empty()) {
			// 敵のプールが空の場合は何もしない
			SF::error("[Manager::Pop()]: No free index in pool.", "Enemy");
			return;
		}
		// プールから敵を取得
		int index = freeEnemyIndices_.back();
		freeEnemyIndices_.pop_back();
		// 敵をアクティブにする
		activeEnemies_[index] = enemies_[index].get();
		enemies_[index]->SetActive(true);
		enemies_[index]->SetUp(position);
	}
}

int EnemyManager::GetCurrentNum() {
	return 0;
}
