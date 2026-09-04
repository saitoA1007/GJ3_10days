#include "EnemyManager.h"
#include <LogManager.h>
#include <Application/Utils/ShigeFunc.h>
#include <RandomGenerator.h>
#include <FPSCounter.h>
#include <ImGuiManager.h>
#include <Application/Utils/Binary/BinaryManager.h>

#include <numbers>

EnemyManager::EnemyManager(uint32_t maxEnemyNum, const GameEngine::Model* model) : maxEnemyNum_(maxEnemyNum) {
	renderer_.SetModel(model);
	worldTransforms_.Initialize(maxEnemyNum_, {});

	enemies_.resize(maxEnemyNum_);
	for (uint32_t i = 0; i < maxEnemyNum_; ++i) {
		enemies_[i] = std::make_unique<Enemy>(&worldTransforms_.transformDatas_[i]);
	}

	auto presetFiles = SF::SearchFiles("Assets/Binary/Preset/", ".bin");

	typeMap_["Straight_S"] = EnemyType::Straight_S;
	typeMap_["Straight_M"] = EnemyType::Straight_M;
	typeMap_["Straight_L"] = EnemyType::Straight_L;
	typeMap_["Round"] = EnemyType::Round;
	typeMap_["Snake"] = EnemyType::Snake;

	LoadPreset();
}

void EnemyManager::Initialize() {
	freeEnemyIndices_.clear();
	activeEnemies_.clear();
	deadEnemies_.clear();

	freeEnemyIndices_.reserve(maxEnemyNum_);
	configList_.resize(static_cast<int>(EnemyType::Count));

	for (uint32_t i = 0; i < maxEnemyNum_; ++i) {
		enemies_[i]->Initialize();
		freeEnemyIndices_.push_back(i);
	}

	renderer_.SetTransforms(&worldTransforms_);

	debugParam_.Register("Pop", debugPop_);
	debugParam_.Register("PopInterval", popInterval_);
	debugParam_.Register("CollisionRadius", collisionRadius_);

	for (int i = 0; i < static_cast<int>(EnemyType::Count); ++i) {
		std::string label = std::to_string(i) + "_" + enemyTypeNames_[i];
		debugParam_.Register("Speed", configList_[i].speed_, 0, label);
		debugParam_.Register("HP", configList_[i].hp, 0, label);
		debugParam_.Register("Size", configList_[i].size_, 0, label);
		debugParam_.Register("NormalColor", configList_[i].normalColor_, 0, label);
		debugParam_.Register("HitColor", configList_[i].hitColor_, 0, label);

		enemyTypeNamesForImGuiList_[i] = enemyTypeNames_[i].c_str();

		if (i == static_cast<int>(EnemyType::Snake)) {
			debugParam_.Register("SwingWidth", swingWidth_, 0, label);
			debugParam_.Register("SnakeSpeed", snakeSpeed_, 0, label);
		}

		if (i == static_cast<int>(EnemyType::Round)) {
			debugParam_.Register("RoundSpeed", roundSpeed_, 0, label);
		}
	}

	debugParam_.Apply();
}

void EnemyManager::Update() {
	debugParam_.ApplyIfDirty();
	Enemy::SetCollisionRadius(collisionRadius_);

	auto getRandomPos = [](float fieldSize)->Vector2 {
		float range = RandomGenerator::Get(fieldSize / 2.f, fieldSize);
		float theta = RandomGenerator::Get(0.f, 2.f * std::numbers::pi_v<float>);
		return Vector2(range * std::cos(theta), range * std::sin(theta));
		};

	const float fieldSize = 20.f;

#ifdef USE_IMGUI

	if (debugPop_) {
		debugPop_ = false;
		Pop(1, getRandomPos(fieldSize), currentType_);
	}

#endif

	//出現処理
	if (stageTimer_ == 0.0f && GameEngine::FpsCounter::deltaTime > 0.0f) {
		auto& fase = stageDataMap_[currentStageName_].fases[currentFaseIndex_];
		for (int i = 0; i < (int)presetDataMap_[fase.name].enemyPositions.size(); ++i) {
			EnemyType type = static_cast<EnemyType>(i);
			for (const auto& transform : presetDataMap_[fase.name].enemyPositions[i]) {
				Vector2 pos = { transform.translate.x, transform.translate.z };
				Pop(1, pos, type);
			}
		}
	}

	stageTimer_ += GameEngine::FpsCounter::deltaTime;
	if (stageDataMap_[currentStageName_].fases[currentFaseIndex_].time <= stageTimer_) {
		currentFaseIndex_++;
		if (currentFaseIndex_ >= stageDataMap_[currentStageName_].fases.size()) {
			currentFaseIndex_ = 0;
		}
		stageTimer_ = 0.0f;
	}

	//更新処理
	for (auto it = deadEnemies_.begin(); it != deadEnemies_.end();) {
		const auto [index, enemy] = *it;
		enemy->DeadUpdate();

		if (!enemy->IsActive()) {
			it = deadEnemies_.erase(it);
			freeEnemyIndices_.push_back(index);
		} else {
			++it;
		}
	}

	for (auto it = activeEnemies_.begin(); it != activeEnemies_.end();) {
		const auto [index, enemy] = *it;
		if (!enemy->IsDead()) {
			enemy->Update();
		}

		if (enemy->IsDead()) {
			// 死亡処理用のリストへ移し、攻撃による撃破だけを一度通知する。
			deadEnemies_[index] = enemy;
			it = activeEnemies_.erase(it);
			if (enemy->WasDefeated() && onEnemyDefeated_) {
				onEnemyDefeated_();
			}
		} else {
			++it;
		}
	}

	worldTransforms_.UpdateTransformMatrix(maxEnemyNum_);
}

void EnemyManager::Draw() {
	renderer_.Draw();
}

void EnemyManager::DebugUpdate() {
#ifdef USE_IMGUI
	ImGui::Begin("EnemyPop");
	static int currentTypeIndex = 0;
	ImGui::ListBox("Type", &currentTypeIndex, enemyTypeNamesForImGuiList_, static_cast<int>(EnemyType::Count));
	ImGui::End();

	currentType_ = static_cast<EnemyType>(currentTypeIndex);
#endif
}

void EnemyManager::SetStage(const std::string& stageName) {
	const auto& it = stageDataMap_.find(stageName);
	if (it == stageDataMap_.end()) {
		SF::error("[EnemyManager::SetStage()]: Stage not found: " + stageName, "Enemy");
		return;
	}

	currentStageName_ = stageName;
	stageTimer_ = 0.0f;
	currentFaseIndex_ = 0;
}

void EnemyManager::Pop(int num, Vector2 position, EnemyType type) {
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
		enemies_[index]->SetUp(position, configList_[static_cast<int>(type)], type);

		if (type == EnemyType::Snake) {
			enemies_[index]->SetSnake(swingWidth_, snakeSpeed_);
		} else if (type == EnemyType::Round) {
			enemies_[index]->SetRound(roundSpeed_);
		}
	}
}

int EnemyManager::GetCurrentNum() {
	return 0;
}

void EnemyManager::LoadPreset() {
	auto files = SF::SearchFiles("Resources/Binary/Preset/", ".bin");

	BinaryManager bin;
	for (const auto& file : files) {
		std::filesystem::path path = file;
		if (!bin.Boot("Preset/" + file)) {
			SF::error("[EnemyManager::LoadPreset()]: Failed to load preset file: " + file, "Enemy");
			continue;
		}

		//最初に識別用の文字列が入っているので読み飛ばす。
		bin.Reverse<std::string>();

		Preset preset;
		preset.enemyPositions.resize(size_t(EnemyType::Count));
		
		uint32_t enemyCount = bin.Reverse<uint32_t>();
		for (uint32_t i = 0; i < enemyCount; ++i) {
			std::string enemyTypeName = bin.Reverse<std::string>();
			int enemyType = static_cast<int>(typeMap_[enemyTypeName]);

			uint32_t positionCount = bin.Reverse<uint32_t>();
			std::vector<Transform> positions;
			for (uint32_t j = 0; j < positionCount; ++j) {
				Transform pos = bin.Reverse<Transform>();
				positions.push_back(pos);
			}

			preset.enemyPositions[enemyType] = positions;
		}
		presetDataMap_[path.stem().string()] = preset;
	}

	auto stageFiles = SF::SearchFiles("Resources/Binary/StageData/", ".bin");
	for (const auto& file : stageFiles) {
		if (!bin.Boot("StageData/" + file)) {
			SF::error("[EnemyManager::LoadPreset()]: Failed to load stage data file: " + file, "Enemy");
			continue;
		}

		std::filesystem::path path = file;

		StageData stageData;

		int presetNum = bin.Reverse<int>();
		stageData.fases.resize(presetNum);
		for (int i = 0; i < presetNum; ++i) {
			stageData.fases[i].name = bin.Reverse<std::string>();
			stageData.fases[i].time = bin.Reverse<float>();
			stageData.fases[i].rotation = bin.Reverse<float>();
		}

		if (stageData.fases.empty()) {
			SF::error("[EnemyManager::LoadPreset()]: Stage data file has no fases: " + file, "Enemy");
			continue;
		}

		stageData.hpRatio = bin.Reverse<float>();
		stageData.minEnemyCount = bin.Reverse<int>();

		stageDataMap_[path.stem().string()] = stageData;
	}
}
