#include "EnergySpawner.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>

#include "FPSCounter.h"
#include "ImGuiManager.h"
#include "MyMath.h"
#include "RandomGenerator.h"

using namespace GameEngine;

namespace
{
	constexpr std::array<const char*, kEnergySizeCount> kEnergySizeNames = 
	{
		"Small",
		"Medium",
		"Large",
		"Special",
	};
	// 自然落下するEnergyは生成禁止帯を除いた3領域だけから抽選する。
	constexpr std::array<FieldZone, 3> kEnergySpawnZones =
	{
		FieldZone::Near,
		FieldZone::Middle,
		FieldZone::Far,
	};
}

EnergySpawner::EnergySpawner(Model* energyModel, Field* field, GameEngine::TextureManager* textureManager,
	GameEngine::Model* planeModel, size_t capacity)
	: field_(field)
{
	// Energyは生成頻度が高いため、固定プールを作って実行中の確保を避ける
	const size_t safeCapacity = (std::max)(capacity, size_t{ 1 });
	pickups_.reserve(safeCapacity);
	for (size_t i = 0; i < safeCapacity; ++i) {
		pickups_.push_back(std::make_unique<EnergyPickup>(energyModel, planeModel, textureManager));
	}

	debugParameter_ = std::make_unique<DebugParameter>("Energy");
	debugParameter_->Register("SpawnInterval", settings_.spawnInterval, 0, "Spawn");
	debugParameter_->Register("RandomSpawnEnabled", settings_.randomSpawnEnabled, 1, "Spawn");
	debugParameter_->Register("AppearDuration", settings_.appearDuration, 2, "Spawn");
	debugParameter_->Register("Lifetime", settings_.lifetime, 3, "Spawn");
	debugParameter_->Register("DissolveDuration", settings_.dissolveDuration, 4, "Spawn");
	debugParameter_->Register("GroundHeight", settings_.groundHeight, 5, "Spawn");
	debugParameter_->Register("MaxActiveCount", settings_.maxActiveCount, 6, "Spawn");
	debugParameter_->Register("InitialCountPerZone", settings_.initialCountPerZone, 7, "Spawn");
	debugParameter_->Register("AngleCenterDegrees", settings_.spawnAngleCenterDegrees, 8, "Spawn");
	debugParameter_->Register("AngleRangeDegrees", settings_.spawnAngleRangeDegrees, 9, "Spawn");
	debugParameter_->Register("FloatingAmplitude", settings_.floatingAmplitude, 0, "Animation");
	debugParameter_->Register("FloatingSpeed", settings_.floatingSpeed, 1, "Animation");
	debugParameter_->Register("RotationSpeed", settings_.rotationSpeed, 2, "Animation");

	for (size_t i = 0; i < typeSettings_.size(); ++i) {
		const std::string group = std::string("Type/") + kEnergySizeNames[i];
		debugParameter_->Register("Scale", typeSettings_[i].scale, 0, group);
		debugParameter_->Register("Value", typeSettings_[i].value, 1, group);
		debugParameter_->Register("Color", typeSettings_[i].color, 2, group);
		debugParameter_->Register("rimColor", typeSettings_[i].rimColor, 2, group);
		debugParameter_->Register("dissolveEdgeColor", typeSettings_[i].dissolveEdgeColor, 2, group);
		// Specialだけが使う虹色アニメーションの設定。
		debugParameter_->Register("rainbowSpeed", typeSettings_[i].rainbowSpeed, 3, group);
		debugParameter_->Register("rainbowSaturation", typeSettings_[i].rainbowSaturation, 4, group);
	}
	debugParameter_->Register("EventCount", timelineEventCount_, 0, "PlayingTimeline");

	debugParameter_->Apply();
	SanitizeSettings();
	InitializeTimelineEvents();
	SetUpdateOrder(10);
}

void EnergySpawner::Initialize()
{
	ApplyDebugParameters();
	gameplayEnabled_ = true;
	autoSpawnEnabled_ = false;
	playingTimelineActive_ = false;
	ResetAll();
}

void EnergySpawner::ResetAll()
{
	spawnTimer_ = 0.0f;
	playingTimelineElapsed_ = 0.0f;
	for (auto& event : timelineEvents_)
	{
		event->hasSpawned = false;
	}
	activeEnergies_.clear();

	// 落下・予約・運搬状態を残さず、全個体をプールへ戻す。
	for (auto& pickup : pickups_)
	{
		pickup->Reset();
	}

	// 各サイズを最初から確認できるよう、3つの生成領域へ同数ずつ置く。
	for (int32_t i = 0; i < settings_.initialCountPerZone; ++i) 
	{
		SpawnInZone(FieldZone::Near);
		SpawnInZone(FieldZone::Middle);
		SpawnInZone(FieldZone::Far);
	}
}

void EnergySpawner::Update()
{
	ApplyDebugParameters();
	if (!gameplayEnabled_)
	{
		return;
	}

	UpdatePickups(FpsCounter::gameDeltaTime);
	UpdatePlayingTimeline(FpsCounter::deltaTime);

	if (autoSpawnEnabled_ && settings_.randomSpawnEnabled)
	{
		spawnTimer_ += FpsCounter::gameDeltaTime;
		if (spawnTimer_ >= settings_.spawnInterval)
		{
			spawnTimer_ = 0.0f;
			SpawnRandom();
		}
	}
}

void EnergySpawner::BeginPlayingTimeline()
{
	playingTimelineElapsed_ = 0.0f;
	playingTimelineActive_ = true;
	for (auto& event : timelineEvents_)
	{
		event->hasSpawned = false;
	}
}

void EnergySpawner::EndPlayingTimeline()
{
	playingTimelineActive_ = false;
}

void EnergySpawner::DebugUpdate()
{
	ApplyDebugParameters();
	UpdatePickups(0.0f);
	DrawDebugWindow();
}

void EnergySpawner::Draw()
{
	for (auto& pickup : pickups_) 
	{
		pickup->Draw(renderQueue_);
	}
}

bool EnergySpawner::SpawnInZone(FieldZone zone) 
{
	if (!gameplayEnabled_)
	{
		return false;
	}
	// Centerと3つのBufferには自然生成しない。
	if (zone != FieldZone::Near && zone != FieldZone::Middle && zone != FieldZone::Far) 
	{
		return false;
	}
	if (GetActiveCount() >= static_cast<size_t>(settings_.maxActiveCount)) 
	{
		return false;
	}

	// 非アクティブな個体だけをプールから再利用する。
	auto available = std::find_if(pickups_.begin(), pickups_.end(), [](const auto& pickup)
		{
		return !pickup->IsActive();
		});
	if (available == pickups_.end()) 
	{
		return false;
	}

	const EnergySize size = GetEnergySize(zone);
	(*available)->Spawn(
		size,
		MakeSpawnPosition(zone),
		settings_.appearDuration, 
		typeSettings_[static_cast<size_t>(size)]
	);
	return true;
}

bool EnergySpawner::SpawnAtArcPosition(EnergySize size, float arcPosition)
{
	if (!gameplayEnabled_)
	{
		return false;
	}

	const size_t sizeIndex = static_cast<size_t>(size);
	if (sizeIndex >= typeSettings_.size())
	{
		return false;
	}
	if (GetActiveCount() >= static_cast<size_t>(settings_.maxActiveCount))
	{
		return false;
	}

	auto available = std::find_if(pickups_.begin(), pickups_.end(), [](const auto& pickup)
		{
			return !pickup->IsActive();
		});
	if (available == pickups_.end())
	{
		return false;
	}

	(*available)->Spawn(
		size,
		MakeArcSpawnPosition(size, arcPosition),
		settings_.appearDuration,
		typeSettings_[sizeIndex]);
	return true;
}

EnergyPickup* EnergySpawner::SpawnOnGround(EnergySize size, const Vector3& position)
{
	const size_t sizeIndex = static_cast<size_t>(size);
	if (sizeIndex >= typeSettings_.size())
	{
		return nullptr;
	}

	auto available = std::find_if(pickups_.begin(), pickups_.end(), [](const auto& pickup)
		{
		return !pickup->IsActive();
		});
	if (available == pickups_.end()) 
	{
		return nullptr;
	}

	// 呼び出し元のY座標に関係なく、Energy用の地面高さへ揃える。
	Vector3 groundPosition = position;
	groundPosition.y = settings_.groundHeight;
	(*available)->SpawnOnGround(size, groundPosition, typeSettings_[sizeIndex]);
	return available->get();
}

EnergyPickup* EnergySpawner::FindNearestAvailable(const Vector3& position, float maxDistance) 
{
	EnergyPickup* nearest = nullptr;
	const float safeMaxDistance = (std::max)(maxDistance, 0.0f);
	float nearestDistanceSquared = safeMaxDistance * safeMaxDistance;

	// Appearing・Reserved・Carriedはロックオン候補に含めない。
	for (auto& pickup : pickups_) 
	{
		if (!pickup->IsTargetable()) 
		{
			continue;
		}

		const Vector3 offset = pickup->GetPosition() - position;
		const float distanceSquared = offset.x * offset.x + offset.z * offset.z;
		if (distanceSquared <= nearestDistanceSquared) 
		{
			nearest = pickup.get();
			nearestDistanceSquared = distanceSquared;
		}
	}

	return nearest;
}

size_t EnergySpawner::GetActiveCount() const
{
	return static_cast<size_t>(std::count_if(pickups_.begin(), pickups_.end(), [](const auto& pickup)
		{
		return pickup->IsActive();
		}));
}

void EnergySpawner::ApplyDebugParameters()
{
	debugParameter_->ApplyIfDirty();

	const int32_t clampedEventCount = (std::clamp)(
		timelineEventCount_,
		0,
		static_cast<int32_t>(pickups_.size()));
	if (clampedEventCount != timelineEventCount_)
	{
		timelineEventCount_ = clampedEventCount;
		RegisterTimelineEventCount();
	}
	if (static_cast<size_t>(timelineEventCount_) != timelineEvents_.size())
	{
		ResizeTimelineEvents(static_cast<size_t>(timelineEventCount_));
		// 新規イベントに同名の保存済みグループがあれば、その値を取り込む。
		debugParameter_->Apply();
	}
	SanitizeSettings();
}

void EnergySpawner::SanitizeSettings() 
{
	settings_.spawnInterval = (std::max)(settings_.spawnInterval, 0.1f);
	settings_.appearDuration = (std::max)(settings_.appearDuration, 0.01f);
	settings_.lifetime = (std::max)(settings_.lifetime, 0.1f);
	settings_.dissolveDuration = (std::max)(settings_.dissolveDuration, 0.01f);
	settings_.spawnAngleRangeDegrees = (std::clamp)(settings_.spawnAngleRangeDegrees, 0.0f, 360.0f);
	settings_.floatingAmplitude = (std::max)(settings_.floatingAmplitude, 0.0f);
	settings_.floatingSpeed = (std::max)(settings_.floatingSpeed, 0.0f);
	settings_.maxActiveCount = (std::clamp)(
		settings_.maxActiveCount,
		1,
		static_cast<int32_t>(pickups_.size()));
	settings_.initialCountPerZone = (std::clamp)(
		settings_.initialCountPerZone,
		0,
		settings_.maxActiveCount / 3);

	for (auto& type : typeSettings_)
	{
		type.scale = (std::max)(type.scale, 0.0f);
		type.value = (std::max)(type.value, 0);
		type.rainbowSpeed = (std::max)(type.rainbowSpeed, 0.0f);
		type.rainbowSaturation = (std::clamp)(type.rainbowSaturation, 0.0f, 1.0f);
	}
	SanitizeTimelineEvents();
}

void EnergySpawner::UpdatePickups(float deltaTime)
{
	// 毎フレームリストをクリアして再構築
	activeEnergies_.clear();
	activeEnergies_.reserve(pickups_.size());

	for (auto& pickup : pickups_)
	{
		if (pickup->IsActive())
		{
			pickup->Update(
				deltaTime,
				settings_.floatingAmplitude,
				settings_.floatingSpeed,
				settings_.rotationSpeed,
				settings_.lifetime,
				settings_.dissolveDuration);

			// アクティブな個体のみ生ポインタを格納
			activeEnergies_.push_back(pickup.get());
		}
	}
}

void EnergySpawner::SpawnRandom()
{
	const int zoneIndex = RandomGenerator::Get<int>(0, static_cast<int>(kEnergySpawnZones.size()) - 1);
	SpawnInZone(kEnergySpawnZones[static_cast<size_t>(zoneIndex)]);
}

void EnergySpawner::UpdatePlayingTimeline(float deltaTime)
{
	if (!playingTimelineActive_)
	{
		return;
	}

	playingTimelineElapsed_ += (std::max)(deltaTime, 0.0f);
	for (auto& event : timelineEvents_)
	{
		if (event->hasSpawned || playingTimelineElapsed_ < event->timeSeconds)
		{
			continue;
		}

		const EnergySize size = static_cast<EnergySize>(event->energySize);
		// 上限やプール不足時は生成済みにせず、次フレーム以降に再試行する。
		event->hasSpawned = SpawnAtArcPosition(size, event->arcPosition);
	}
}

Vector3 EnergySpawner::MakeSpawnPosition(FieldZone zone) const 
{
	// 各生成領域の直前にあるBufferを内周として、生成可能な円環を求める。
	FieldZone innerZone = FieldZone::Center;
	switch (zone) {
	case FieldZone::Middle:
		innerZone = FieldZone::NearBuffer;
		break;
	case FieldZone::Far:
		innerZone = FieldZone::MiddleBuffer;
		break;
	case FieldZone::Near:
	default:
		break;
	}

	const float innerRadius = field_->GetRadius(innerZone);
	const float outerRadius = field_->GetRadius(zone);
	const float minRadius = (std::min)(innerRadius, outerRadius);
	const float maxRadius = (std::max)(innerRadius, outerRadius);

	// 半径そのものではなく二乗を一様抽選し、円環の外側へ偏る問題を防ぐ。
	const float radiusSquared = RandomGenerator::Get<float>(
		minRadius * minRadius,
		maxRadius * maxRadius);
	const float radius = std::sqrt(radiusSquared);
	// 0度を+X方向とし、中心角を基準に指定幅の扇形から抽選する。
	// 360度なら従来どおり円環全周、180度なら中心角の左右90度が生成範囲になる。
	const float halfAngleRangeDegrees = settings_.spawnAngleRangeDegrees * 0.5f;
	const float angleDegrees = RandomGenerator::Get<float>(
		settings_.spawnAngleCenterDegrees - halfAngleRangeDegrees,
		settings_.spawnAngleCenterDegrees + halfAngleRangeDegrees);
	const float angle = angleDegrees * (PI / 180.0f);
	const Vector3 center = field_->GetSettings().center;

	return 
	{
		center.x + std::cos(angle) * radius,
		settings_.groundHeight,
		center.z + std::sin(angle) * radius,
	};
}

Vector3 EnergySpawner::MakeArcSpawnPosition(EnergySize size, float arcPosition) const
{
	const FieldZone zone = GetSpawnZone(size);
	FieldZone innerZone = FieldZone::Center;
	switch (zone)
	{
	case FieldZone::Middle:
		innerZone = FieldZone::NearBuffer;
		break;
	case FieldZone::Far:
		innerZone = FieldZone::MiddleBuffer;
		break;
	case FieldZone::Near:
	default:
		break;
	}

	const float innerRadius = field_->GetRadius(innerZone);
	const float outerRadius = field_->GetRadius(zone);
	const float radius = (innerRadius + outerRadius) * 0.5f;
	const float normalizedPosition = (std::clamp)(arcPosition, 0.0f, 1.0f);
	const float startDegrees =
		settings_.spawnAngleCenterDegrees - settings_.spawnAngleRangeDegrees * 0.5f;
	const float angleDegrees = startDegrees + settings_.spawnAngleRangeDegrees * normalizedPosition;
	const float angle = angleDegrees * (PI / 180.0f);
	const Vector3 center = field_->GetSettings().center;

	return
	{
		center.x + std::cos(angle) * radius,
		settings_.groundHeight,
		center.z + std::sin(angle) * radius,
	};
}

EnergySize EnergySpawner::GetEnergySize(FieldZone zone) const
{
	switch (zone)
	{
	case FieldZone::Near:
		return EnergySize::Small;
	case FieldZone::Middle:
		return EnergySize::Medium;
	case FieldZone::Far:
		return EnergySize::Large;
	default:
		return EnergySize::Small;
	}
}

FieldZone EnergySpawner::GetSpawnZone(EnergySize size) const
{
	switch (size)
	{
	case EnergySize::Small:
		return FieldZone::Near;
	case EnergySize::Medium:
		return FieldZone::Middle;
	case EnergySize::Large:
	case EnergySize::Special:
		return FieldZone::Far;
	default:
		return FieldZone::Near;
	}
}

void EnergySpawner::InitializeTimelineEvents()
{
	const int32_t clampedCount = (std::clamp)(
		timelineEventCount_,
		0,
		static_cast<int32_t>(pickups_.size()));
	if (clampedCount != timelineEventCount_)
	{
		timelineEventCount_ = clampedCount;
		RegisterTimelineEventCount();
	}

	ResizeTimelineEvents(static_cast<size_t>(timelineEventCount_));
	// EventCountを先に読み、その個数分の動的フィールドを登録してから値を読む。
	debugParameter_->Apply();
	SanitizeTimelineEvents();
}

void EnergySpawner::ResizeTimelineEvents(size_t count)
{
	const size_t safeCount = (std::min)(count, pickups_.size());
	while (timelineEvents_.size() > safeCount)
	{
		const size_t removedIndex = timelineEvents_.size() - 1;
		char groupName[64];
		sprintf_s(groupName, "PlayingTimeline/Events/Event%03zu", removedIndex);
		debugParameter_->RemoveGroup(groupName);
		timelineEvents_.pop_back();
	}

	while (timelineEvents_.size() < safeCount)
	{
		const size_t index = timelineEvents_.size();
		auto event = std::make_unique<ScheduledEnergySpawnEvent>();
		if (!timelineEvents_.empty())
		{
			event->timeSeconds = timelineEvents_.back()->timeSeconds + 1.0f;
		}
		timelineEvents_.push_back(std::move(event));
		RegisterTimelineEvent(index);
	}

	timelineEventCount_ = static_cast<int32_t>(timelineEvents_.size());
}

void EnergySpawner::RegisterTimelineEvent(size_t index)
{
	if (index >= timelineEvents_.size())
	{
		return;
	}

	char groupName[64];
	sprintf_s(groupName, "PlayingTimeline/Events/Event%03zu", index);
	auto& event = *timelineEvents_[index];
	debugParameter_->Register("TimeSeconds", event.timeSeconds, 0, groupName);
	debugParameter_->Register("EnergySize", event.energySize, 1, groupName);
	debugParameter_->Register("ArcPosition", event.arcPosition, 2, groupName);
}

void EnergySpawner::RegisterTimelineEventCount()
{
	debugParameter_->Register("EventCount", timelineEventCount_, 0, "PlayingTimeline");
}

void EnergySpawner::RemoveTimelineEvent(size_t index)
{
	if (index >= timelineEvents_.size())
	{
		return;
	}

	// インデックスを詰め直すため、一度イベント配下だけを登録解除して再構築する。
	debugParameter_->RemoveGroup("PlayingTimeline/Events");
	timelineEvents_.erase(timelineEvents_.begin() + index);
	timelineEventCount_ = static_cast<int32_t>(timelineEvents_.size());
	RegisterTimelineEventCount();
	for (size_t i = 0; i < timelineEvents_.size(); ++i)
	{
		RegisterTimelineEvent(i);
	}
}

void EnergySpawner::SanitizeTimelineEvents()
{
	const int32_t maxSize = static_cast<int32_t>(EnergySize::Count) - 1;
	for (auto& event : timelineEvents_)
	{
		event->timeSeconds = (std::max)(event->timeSeconds, 0.0f);
		event->energySize = (std::clamp)(event->energySize, 0, maxSize);
		event->arcPosition = (std::clamp)(event->arcPosition, 0.0f, 1.0f);
	}
}

void EnergySpawner::DrawDebugWindow()
{
#ifdef USE_IMGUI
	if (!ImGui::Begin("Energy"))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Active: %zu / %zu", GetActiveCount(), pickups_.size());
	if (ImGui::Button("Spawn Near / Small"))
	{
		SpawnInZone(FieldZone::Near);
	}
	if (ImGui::Button("Spawn Middle / Medium"))
	{
		SpawnInZone(FieldZone::Middle);
	}
	if (ImGui::Button("Spawn Far / Large"))
	{
		SpawnInZone(FieldZone::Far);
	}

	ImGui::Separator();
	ImGui::Text("Playing Spawn Timeline");
	ImGui::TextDisabled("Arc: 0.0 = Left, 0.5 = Center, 1.0 = Right");
	ImGui::Text("Elapsed: %.2f sec", playingTimelineElapsed_);
	if (ImGui::Button("Add Timeline Event") && timelineEvents_.size() < pickups_.size())
	{
		ResizeTimelineEvents(timelineEvents_.size() + 1);
		RegisterTimelineEventCount();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("Save from Parameter Inspector > Energy");

	size_t removeIndex = timelineEvents_.size();
	for (size_t i = 0; i < timelineEvents_.size(); ++i)
	{
		ImGui::PushID(static_cast<int>(i));
		auto& event = *timelineEvents_[i];
		const char* state = event.hasSpawned ? "Spawned" : "Pending";
		char header[96];
		sprintf_s(header, "Event %03zu  [%.2fs / %s]", i, event.timeSeconds, state);
		const bool open = ImGui::TreeNode(header);
		ImGui::SameLine();
		if (ImGui::SmallButton("Delete"))
		{
			removeIndex = i;
		}

		if (open)
		{
			bool changed = ImGui::DragFloat("Time Seconds", &event.timeSeconds, 0.1f, 0.0f, 3600.0f, "%.2f s");
			changed |= ImGui::Combo(
				"Energy Size",
				&event.energySize,
				kEnergySizeNames.data(),
				static_cast<int>(kEnergySizeNames.size()));
			changed |= ImGui::SliderFloat("Arc Position", &event.arcPosition, 0.0f, 1.0f, "%.3f");
			if (changed)
			{
				SanitizeTimelineEvents();
				// 独自UIで変えた値をParameter Inspectorの保存対象へ書き戻す。
				RegisterTimelineEvent(i);
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
	}
	if (removeIndex < timelineEvents_.size())
	{
		RemoveTimelineEvent(removeIndex);
	}

	ImGui::End();
#endif
}

