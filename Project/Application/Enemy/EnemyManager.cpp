#include "EnemyManager.h"
#include <algorithm>
#include <LogManager.h>
#include <Application/Utils/ShigeFunc.h>
#include <RandomGenerator.h>
#include <FPSCounter.h>
#include <ImGuiManager.h>
#include <AudioManager.h>
#include <Application/Utils/Binary/BinaryManager.h>
#include <Application/Field/Field.h>
#include "EnemyEffectManager.h"

#include <numbers>

namespace
{
	constexpr const char* kEnemySpawnSoundName = "enemySpawn.mp3";
	constexpr float kEnemySpawnSoundVolume = 1.0f;
}

EnemyManager::EnemyManager(uint32_t maxEnemyNum, EnemyEffectManager* effectManager, GameEngine::TextureManager* textureManager, GameEngine::ModelManager* modelManager) : maxEnemyNum_(maxEnemyNum) {
	// 敵の演出管理機能を取得
	effectManager_ = effectManager;

	commonEffect_ = std::make_unique<EnemyCommonEffect>(textureManager, modelManager);

	auto model = modelManager->GetNameByModel("Enemy.obj");

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

void EnemyManager::SetContext(Field* field, Rocket* rocket, EnergySpawner* energySpawner, UnitManager* unitManager) {
	context_.field = field;
	context_.rocket = rocket;
	context_.energySpawner = energySpawner;
	context_.unitManager = unitManager;

	// 全個体へコンテキストを伝達
	for (auto& enemy : enemies_) {
		enemy->SetContext(context_);
	}
}

void EnemyManager::Initialize() {
	gameplayEnabled_ = true;
	autoSpawnEnabled_ = false;
	playingTimelineActive_ = false;
	configList_.resize(static_cast<int>(EnemyType::Count));
	ResetAll();

	renderer_.SetTransforms(&worldTransforms_);

	debugParam_.Register("Pop", debugPop_);
	debugParam_.Register("PopInterval", popInterval_);
	debugParam_.Register("CollisionRadius", collisionRadius_);
	debugParam_.Register("StageSpawnEnabled", stageSpawnEnabled_, 0, "Spawn");
	debugParam_.Register("EventCount", timelineEventCount_, 0, "PlayingTimeline");

	for (int i = 0; i < static_cast<int>(EnemyType::Count); ++i) {
		std::string label = std::to_string(i) + "_" + enemyTypeNames_[i];
		debugParam_.Register("Speed", configList_[i].speed_, 0, label);
		debugParam_.Register("HP", configList_[i].hp, 0, label);
		debugParam_.Register("Size", configList_[i].size_, 0, label);
		debugParam_.Register("NormalColor", configList_[i].normalColor_, 0, label);
		debugParam_.Register("HighlightColor", configList_[i].highlightColor_, 0, label);
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
	InitializeTimelineEvents();
}

void EnemyManager::ResetAll()
{
	freeEnemyIndices_.clear();
	activeEnemies_.clear();
	activeEnemyVector_.clear();
	deadEnemies_.clear();
	freeEnemyIndices_.reserve(maxEnemyNum_);

	for (uint32_t i = 0; i < maxEnemyNum_; ++i)
	{
		enemies_[i]->Initialize();
		enemies_[i]->SetContext(context_);
		freeEnemyIndices_.push_back(static_cast<int>(i));
	}

	popTimer_ = 0.0f;
	stageTimer_ = 0.0f;
	currentFaseIndex_ = 0;
	playingTimelineElapsed_ = 0.0f;
	for (auto& event : timelineEvents_) {
		event->hasSpawned = false;
	}
	if (commonEffect_)
	{
		commonEffect_->Initialize();
	}
	worldTransforms_.UpdateTransformMatrix(maxEnemyNum_);
}

void EnemyManager::Update() {
	ApplyDebugParameters();
	Enemy::SetCollisionRadius(collisionRadius_);

	// ゲームプレイが無効なら更新をスキップ
	if (!gameplayEnabled_) {
		return;
	}

	UpdatePlayingTimeline(GameEngine::FpsCounter::deltaTime);

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

	// 出現処理（ステージ名が設定されていてデータが存在する場合のみ実行）
	if (autoSpawnEnabled_ && stageSpawnEnabled_ && !currentStageName_.empty() && stageDataMap_.contains(currentStageName_)) {
		auto& stage = stageDataMap_[currentStageName_];

		if (currentFaseIndex_ < stage.fases.size()) {
			SF::warn("[EnemyManager::Update()]: FaseIndex go to Unknown Number", "Enemy");
			currentFaseIndex_ = 0;
		}

		if (!stage.fases.empty()) {

			if (stageTimer_ == 0.0f && GameEngine::FpsCounter::deltaTime > 0.0f) {
				auto& fase = stage.fases[currentFaseIndex_];
				if (presetDataMap_.contains(fase.name)) {
					for (int i = 0; i < (int)presetDataMap_[fase.name].enemyPositions.size(); ++i) {
						EnemyType type = static_cast<EnemyType>(i);
						for (const auto& transform : presetDataMap_[fase.name].enemyPositions[i]) {
							Vector2 pos = { transform.translate.x, transform.translate.z };
							Pop(1, pos, type);
						}
					}
				}
			}

			stageTimer_ += GameEngine::FpsCounter::deltaTime;

			//次のフェーズに進む条件をチェック
			if (stage.fases[currentFaseIndex_].time <= stageTimer_ || (int)activeEnemies_.size() < stage.minEnemyCount) {
				currentFaseIndex_++;
				if (currentFaseIndex_ >= stage.fases.size()) {
					currentFaseIndex_ = 0;
				}

				stageTimer_ = 0.0f;
			}
		}
	}

	// 死亡後アクション更新
	for (auto it = deadEnemies_.begin(); it != deadEnemies_.end();) {
		const auto [index, enemy] = *it;
		enemy->DeadUpdate();

		if (!enemy->IsActive()) {
			it = deadEnemies_.erase(it);
			freeEnemyIndices_.push_back(index);
		}
		else {
			++it;
		}
	}

	// アクティブ敵の更新処理
	for (auto it = activeEnemies_.begin(); it != activeEnemies_.end();) {
		const auto [index, enemy] = *it;
		if (!enemy->IsDead()) {
			enemy->Update();
			commonEffect_->SetPosition(enemy->GetEffectID(), enemy->GetWorldMatrix());
		}

		if (enemy->IsDead()) {
			deadEnemies_[index] = enemy;
			it = activeEnemies_.erase(it);
			if (enemy->WasDefeated() && onEnemyDefeated_) {
				onEnemyDefeated_();
			}

			commonEffect_->ReleaseEffectID(enemy->GetEffectID());

			effectManager_->StartDeadEffect(enemy->GetPosition());
		}
		else {
			++it;
		}
	}

	activeEnemyVector_.clear();
	activeEnemyVector_.reserve(activeEnemies_.size());
	for (const auto& [index, enemy] : activeEnemies_) {
		activeEnemyVector_.push_back(enemy);
	}

	worldTransforms_.UpdateTransformMatrix(maxEnemyNum_);

	commonEffect_->Update();
}

void EnemyManager::Draw() {
	renderer_.Draw();
	commonEffect_->Draw();
}

void EnemyManager::DebugUpdate() {
	ApplyDebugParameters();
#ifdef USE_IMGUI
	ImGui::Begin("EnemyPop");
	static int currentTypeIndex = 0;
	ImGui::ListBox("Type", &currentTypeIndex, enemyTypeNamesForImGuiList_, static_cast<int>(EnemyType::Count));
	ImGui::End();

	currentType_ = static_cast<EnemyType>(currentTypeIndex);
#endif
}

void EnemyManager::BeginPlayingTimeline() {
	playingTimelineElapsed_ = 0.0f;
	playingTimelineActive_ = true;
	for (auto& event : timelineEvents_) {
		event->hasSpawned = false;
	}
}

void EnemyManager::EndPlayingTimeline() {
	playingTimelineActive_ = false;
}

void EnemyManager::ApplyDebugParameters() {
	debugParam_.ApplyIfDirty();

	const int32_t clampedEventCount = (std::clamp)(
		timelineEventCount_,
		0,
		static_cast<int32_t>(maxEnemyNum_));
	if (clampedEventCount != timelineEventCount_) {
		timelineEventCount_ = clampedEventCount;
		RegisterTimelineEventCount();
	}
	if (static_cast<size_t>(timelineEventCount_) != timelineEvents_.size()) {
		ResizeTimelineEvents(static_cast<size_t>(timelineEventCount_));
		debugParam_.Apply();
	}

	SanitizeTimelineEvents();
}

void EnemyManager::UpdatePlayingTimeline(float deltaTime) {
	if (!playingTimelineActive_) {
		return;
	}

	playingTimelineElapsed_ += (std::max)(deltaTime, 0.0f);
	for (auto& event : timelineEvents_) {
		if (event->hasSpawned || playingTimelineElapsed_ < event->timeSeconds) {
			continue;
		}
		if (freeEnemyIndices_.empty()) {
			continue;
		}

		// タイムラインでは種類を増やさず、直進する標準敵だけを生成する。
		event->hasSpawned =
			Pop(1, MakeTimelineSpawnPosition(event->arcPosition), EnemyType::Straight_S) != nullptr;
	}
}

Vector2 EnemyManager::MakeTimelineSpawnPosition(float arcPosition) const {
	const float normalizedPosition = (std::clamp)(arcPosition, 0.0f, 1.0f);
	const float angle = std::numbers::pi_v<float> + std::numbers::pi_v<float> * normalizedPosition;
	const float radius = context_.field
		? context_.field->GetRadius(FieldZone::OuterBuffer)
		: 35.0f;
	const Vector3 center = context_.field
		? context_.field->GetSettings().center
		: Vector3{};

	return {
		center.x + std::cos(angle) * radius,
		center.z + std::sin(angle) * radius,
	};
}

void EnemyManager::InitializeTimelineEvents() {
	const int32_t clampedCount = (std::clamp)(
		timelineEventCount_,
		0,
		static_cast<int32_t>(maxEnemyNum_));
	if (clampedCount != timelineEventCount_) {
		timelineEventCount_ = clampedCount;
		RegisterTimelineEventCount();
	}

	ResizeTimelineEvents(static_cast<size_t>(timelineEventCount_));
	debugParam_.Apply();
	SanitizeTimelineEvents();
}

void EnemyManager::ResizeTimelineEvents(size_t count) {
	const size_t safeCount = (std::min)(count, static_cast<size_t>(maxEnemyNum_));
	while (timelineEvents_.size() > safeCount) {
		const size_t removedIndex = timelineEvents_.size() - 1;
		char groupName[64];
		sprintf_s(groupName, "PlayingTimeline/Events/Event%03zu", removedIndex);
		debugParam_.RemoveGroup(groupName);
		timelineEvents_.pop_back();
	}

	while (timelineEvents_.size() < safeCount) {
		const size_t index = timelineEvents_.size();
		auto event = std::make_unique<ScheduledEnemySpawnEvent>();
		if (!timelineEvents_.empty()) {
			event->timeSeconds = timelineEvents_.back()->timeSeconds + 1.0f;
		}
		timelineEvents_.push_back(std::move(event));
		RegisterTimelineEvent(index);
	}

	timelineEventCount_ = static_cast<int32_t>(timelineEvents_.size());
}

void EnemyManager::RegisterTimelineEvent(size_t index) {
	if (index >= timelineEvents_.size()) {
		return;
	}

	char groupName[64];
	sprintf_s(groupName, "PlayingTimeline/Events/Event%03zu", index);
	auto& event = *timelineEvents_[index];
	debugParam_.Register("TimeSeconds", event.timeSeconds, 0, groupName);
	debugParam_.Register("ArcPosition", event.arcPosition, 1, groupName);
}

void EnemyManager::RegisterTimelineEventCount() {
	debugParam_.Register("EventCount", timelineEventCount_, 0, "PlayingTimeline");
}

void EnemyManager::SanitizeTimelineEvents() {
	for (auto& event : timelineEvents_) {
		event->timeSeconds = (std::max)(event->timeSeconds, 0.0f);
		event->arcPosition = (std::clamp)(event->arcPosition, 0.0f, 1.0f);
	}
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

Enemy* EnemyManager::Pop(int num, Vector2 position, EnemyType type) {
	Enemy* firstSpawnedEnemy = nullptr;
	for (int i = 0; i < num; ++i) {
		if (freeEnemyIndices_.empty()) {
			// 敵のプールが空の場合は何もしない
			SF::error("[Manager::Pop()]: No free index in pool.", "Enemy");
			return firstSpawnedEnemy;
		}
		// プールから敵を取得
		int index = freeEnemyIndices_.back();
		freeEnemyIndices_.pop_back();
		// 敵をアクティブにする
		activeEnemies_[index] = enemies_[index].get();
		enemies_[index]->SetActive(true);
		if (!firstSpawnedEnemy) {
			firstSpawnedEnemy = enemies_[index].get();

			auto& audioManager = GameEngine::AudioManager::GetInstance();
			const uint32_t enemySpawnSoundHandle =
				audioManager.GetHandleByName(kEnemySpawnSoundName);
			audioManager.Stop(enemySpawnSoundHandle);
			audioManager.Play(enemySpawnSoundHandle, kEnemySpawnSoundVolume, false);
		}
		enemies_[index]->SetUp(position, configList_[static_cast<int>(type)], type, commonEffect_->SecureEffectID());
		// 敵の登場演出
		effectManager_->StartSpawnEffect(Vector3(position.x,0.0f, position.y));

		if (type == EnemyType::Snake) {
			enemies_[index]->SetSnake(swingWidth_, snakeSpeed_);
		} else if (type == EnemyType::Round) {
			enemies_[index]->SetRound(roundSpeed_);
		}
	}
	return firstSpawnedEnemy;
}

void EnemyManager::Despawn(Enemy* enemy)
{
	if (!enemy)
	{
		return;
	}

	for (auto it = activeEnemies_.begin(); it != activeEnemies_.end(); ++it)
	{
		if (it->second != enemy)
		{
			continue;
		}

		const int index = it->first;
		if (commonEffect_)
		{
			commonEffect_->ReleaseEffectID(enemy->GetEffectID());
		}
		enemy->Initialize();
		activeEnemies_.erase(it);
		freeEnemyIndices_.push_back(index);
		activeEnemyVector_.erase(
			(std::remove)(activeEnemyVector_.begin(), activeEnemyVector_.end(), enemy),
			activeEnemyVector_.end());
		worldTransforms_.UpdateTransformMatrix(maxEnemyNum_);
		return;
	}

	for (auto it = deadEnemies_.begin(); it != deadEnemies_.end(); ++it)
	{
		if (it->second != enemy)
		{
			continue;
		}

		const int index = it->first;
		enemy->Initialize();
		deadEnemies_.erase(it);
		freeEnemyIndices_.push_back(index);
		worldTransforms_.UpdateTransformMatrix(maxEnemyNum_);
		return;
	}
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


Enemy* EnemyManager::FindNearestTargetable(const Vector3& position, float maxDistance) const {
	Enemy* nearest = nullptr;
	const float safeMaxDistance = (std::max)(maxDistance, 0.0f);
	float nearestDistanceSquared = safeMaxDistance * safeMaxDistance;

	for (const auto& [index, enemy] : activeEnemies_) {
		if (!enemy || !enemy->IsTargetable()) {
			continue;
		}

		const Vector3 offset = enemy->GetPosition() - position;
		const float distanceSquared = offset.x * offset.x + offset.z * offset.z;

		if (distanceSquared <= nearestDistanceSquared) {
			nearest = enemy;
			nearestDistanceSquared = distanceSquared;
		}
	}

	return nearest;
}

size_t EnemyManager::GetCarrierTargetCount() const {
	size_t count = 0;
	for (const auto& [index, enemy] : activeEnemies_) {
		if (enemy && enemy->IsTargetCarrier()) {
			count++;
		}
	}
	return count;
}
