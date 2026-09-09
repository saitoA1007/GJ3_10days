#include "EnergySpawner.h"

#include <algorithm>
#include <cassert>
#include <cmath>

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
	constexpr std::array<FieldZone, kEnergySizeCount> kEnergySpawnZones = 
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
	debugParameter_->Register("AppearDuration", settings_.appearDuration, 1, "Spawn");
	debugParameter_->Register("Lifetime", settings_.lifetime, 2, "Spawn");
	debugParameter_->Register("DissolveDuration", settings_.dissolveDuration, 3, "Spawn");
	debugParameter_->Register("GroundHeight", settings_.groundHeight, 4, "Spawn");
	debugParameter_->Register("MaxActiveCount", settings_.maxActiveCount, 5, "Spawn");
	debugParameter_->Register("InitialCountPerZone", settings_.initialCountPerZone, 6, "Spawn");
	debugParameter_->Register("AngleCenterDegrees", settings_.spawnAngleCenterDegrees, 7, "Spawn");
	debugParameter_->Register("AngleRangeDegrees", settings_.spawnAngleRangeDegrees, 8, "Spawn");
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

	debugParameter_->Apply();
	SanitizeSettings();
	SetUpdateOrder(10);
}

void EnergySpawner::Initialize()
{
	ApplyDebugParameters();
	gameplayEnabled_ = true;
	autoSpawnEnabled_ = false;
	spawnTimer_ = 0.0f;
	// シーン再初期化時に落下・予約・運搬状態を残さない。
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

	if (autoSpawnEnabled_)
	{
		spawnTimer_ += FpsCounter::gameDeltaTime;
		if (spawnTimer_ >= settings_.spawnInterval)
		{
			spawnTimer_ = 0.0f;
			SpawnRandom();
		}
	}
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

	ImGui::End();
#endif
}

