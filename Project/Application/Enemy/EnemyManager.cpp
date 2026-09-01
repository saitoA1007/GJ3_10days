#include "EnemyManager.h"
#include <LogManager.h>
#include <Application/Utils/ShigeFunc.h>
#include <RandomGenerator.h>
#include <FPSCounter.h>

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
	debugParam_.Register("PopInterval", popInterval_);
	debugParam_.Register("Speed", enemyConfig_.speed_, 0, "EnemyConfig");
	debugParam_.Register("HP", enemyConfig_.hp, 0, "EnemyConfig");
	debugParam_.Register("NormalColor", enemyConfig_.normalColor_, 0, "EnemyConfig");
	debugParam_.Register("HitColor", enemyConfig_.hitColor_, 0, "EnemyConfig");

	debugParam_.Apply();
}

void EnemyManager::Update() {
	debugParam_.ApplyIfDirty();

	auto getRandomPos = [](float fieldSize)->Vector2 {
		float range = RandomGenerator::Get(fieldSize / 2.f, fieldSize);
		float theta = RandomGenerator::Get(0.f, 2.f * std::numbers::pi_v<float>);
		return Vector2(range * std::cos(theta), range * std::sin(theta));
		};

	const float fieldSize = 20.f;

#ifdef USE_IMGUI

	if (debugPop_) {
		debugPop_ = false;
		Pop(1, getRandomPos(fieldSize));
	}

#endif

	//出現処理
	popTimer_ += GameEngine::FpsCounter::deltaTime;
	if (popTimer_ > popInterval_) {
		popTimer_ = 0.0f;
		Pop(1, getRandomPos(fieldSize));
	}

	//更新処理
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
		enemies_[index]->SetUp(position, enemyConfig_);
	}
}

int EnemyManager::GetCurrentNum() {
	return 0;
}
