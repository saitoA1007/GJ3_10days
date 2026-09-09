#include "UnitManager.h"

#include <algorithm>
#include <cassert>

#include "FPSCounter.h"
#include "ImGuiManager.h"

#include "Application/Enemy/Enemy.h"
#include "Application/Energy/EnergyPickup.h"
#include "Application/Enemy/EnemyManager.h"
#include "Application/Energy/EnergySpawner.h"
#include "Application/Rocket/Rocket.h"

using namespace GameEngine;

UnitManager::UnitManager(Model* unitModel, GameEngine::Model* circleModel, Rocket* rocket,
	GameEngine::Model* baemModel, GameEngine::Model* markerModel, uint32_t beamGH,
	GameEngine::Model* arrowModel, uint32_t lineGH,
	UnitEffectManager* unitEffectManager, EnergySpawner* energySpawner, EnemyManager* enemyManager, size_t capacity)
	: rocket_(rocket), energySpawner_(energySpawner), enemyManager_(enemyManager)
{
	// エフェクト管理機能を取得
	Unit::StaticInitialize(unitEffectManager);

	// Unitは倒れても再利用するため、最大候補数を固定プールとして確保する。
	const size_t safeCapacity = (std::max)(capacity, size_t{ 1 });
	units_.reserve(safeCapacity);
	for (size_t i = 0; i < safeCapacity; ++i)
	{
		units_.push_back(std::make_unique<Unit>(
			unitModel, circleModel, rocket_, &settings_.unit, baemModel, beamGH, arrowModel, lineGH, energySpawner_, markerModel));
	}

	debugParameter_ = std::make_unique<DebugParameter>("Unit");
	debugParameter_->Register("UnitCount", settings_.unitCount, 0, "Manager");
	debugParameter_->Register("GroundY", settings_.unit.groundY, 0, "Transform");
	debugParameter_->Register("LaunchOffset", settings_.unit.launchOffset, 1, "Transform");
	debugParameter_->Register("LaunchDistance", settings_.unit.launchDistance, 2, "Transform");
	debugParameter_->Register("Scale", settings_.unit.scale, 3, "Transform");
	debugParameter_->Register("CarryOffset", settings_.unit.carryOffset, 4, "Transform");
	debugParameter_->Register("NormalSpeed", settings_.unit.normalSpeed, 0, "Move");
	debugParameter_->Register("BoostedSpeed", settings_.unit.boostedSpeed, 1, "Move");
	debugParameter_->Register("PickupRadius", settings_.unit.pickupRadius, 2, "Move");
	debugParameter_->Register("DeliveryRadius", settings_.unit.deliveryRadius, 3, "Move");
	debugParameter_->Register("Radius", settings_.unit.collisionRadius, 0, "Collision");
	debugParameter_->Register("DrainPerSecond", settings_.unit.staminaDrainPerSecond, 0, "Stamina");
	debugParameter_->Register("DistanceDrainRate", settings_.unit.distanceDrainRate, 1, "Stamina");
	debugParameter_->Register("Normal", settings_.unit.normalColor, 0, "Color");
	debugParameter_->Register("WithStamina", settings_.unit.staminaColor, 1, "Color");

	debugParameter_->Register("BHEnergyThreshold", settings_.unit.bhEnergyThreshold, 0, "Blackhole");
	debugParameter_->Register("BHDuration", settings_.unit.bhDuration, 1, "Blackhole");
	debugParameter_->Register("BHPullSpeed", settings_.unit.bhPullSpeed, 2, "Blackhole");
	debugParameter_->Register("BHKillRadius", settings_.unit.bhKillRadius, 3, "Blackhole");
	debugParameter_->Register("BHBaseRadius", settings_.unit.bhBaseRadius, 4, "Blackhole");
	debugParameter_->Register("BHNearDistance", settings_.unit.bhNearDistance, 5, "Blackhole");
	debugParameter_->Register("BHFarDistance", settings_.unit.bhFarDistance, 6, "Blackhole");
	debugParameter_->Register("BHRadiusNearMult", settings_.unit.bhRadiusNearMultiplier, 7, "Blackhole");
	debugParameter_->Register("BHRadiusMidMult", settings_.unit.bhRadiusMidMultiplier, 8, "Blackhole");
	debugParameter_->Register("BHRadiusFarMult", settings_.unit.bhRadiusFarMultiplier, 9, "Blackhole");
	debugParameter_->Register("BHSpecialMult", settings_.unit.bhSpecialMultiplier, 10, "Blackhole");
	debugParameter_->Apply();
	SanitizeSettings();
	SetUpdateOrder(20);
}

void UnitManager::Initialize() 
{
	ApplyDebugParameters();
	gameplayEnabled_ = true;
	for (auto& unit : units_)
	{
		unit->Initialize();
	}
}

void UnitManager::Update() 
{
	ApplyDebugParameters();
	ApplyUnitCount();
	if (!gameplayEnabled_) return;

	// 吸い込み対象のリストを取得
	const auto& enemies = enemyManager_ ? enemyManager_->GetEnemies() : std::vector<Enemy*>{};
	const auto& energies = energySpawner_ ? energySpawner_->GetEnergies() : std::vector<EnergyPickup*>{};

	// 吸い込み判定用
	std::vector<Unit*> rawUnits;
	rawUnits.reserve(GetUnitCount());
	for (size_t i = 0; i < GetUnitCount(); ++i) {
		rawUnits.push_back(units_[i].get());
	}

	const float deltaTime = FpsCounter::deltaTime;

	// 稼働中の各 Unit を更新
	for (size_t i = 0; i < GetUnitCount(); ++i)
	{
		// ブラックホール状態のユニットがあれば吸い込み処理を実行
		if (units_[i]->IsBlackhole())
		{
			units_[i]->ProcessBlackholeAbsorption(enemies, rawUnits, energies, deltaTime);
		}

		units_[i]->Update();
	}
}

void UnitManager::DebugUpdate() 
{
	ApplyDebugParameters();
	ApplyUnitCount();
	for (size_t i = 0; i < GetUnitCount(); ++i)
	{
		units_[i]->RefreshVisual();
	}
	DrawDebugWindow();
}

void UnitManager::Draw()
{
	for (size_t i = 0; i < GetUnitCount(); ++i)
	{
		units_[i]->Draw();
	}
}

bool UnitManager::DispatchToEnergy(EnergyPickup* target, int32_t requestedEnergy)
{
	if (!gameplayEnabled_ || !target || !target->IsTargetable())
	{
		return false;
	}

	// 待機中の最初の1体へ依頼し、利用可能数を越えた派遣は拒否する。
	for (size_t i = 0; i < GetUnitCount(); ++i)
	{
		if (units_[i]->IsAvailable())
		{
			return units_[i]->DispatchToEnergy(target, requestedEnergy);
		}
	}
	return false;
}

bool UnitManager::DispatchToEnemy(Enemy* target, int32_t requestedEnergy)
{
	if (!gameplayEnabled_ || !target || !target->IsTargetable())
	{
		return false;
	}

	for (size_t i = 0; i < GetUnitCount(); ++i) 
	{
		if (units_[i]->IsAvailable())
		{
			return units_[i]->DispatchToEnemy(target, requestedEnergy);
		}
	}
	return false;
}

bool UnitManager::DispatchToPosition(const Vector3& targetPosition, int32_t requestedEnergy)
{
	if (!gameplayEnabled_)
	{
		return false;
	}

	for (size_t i = 0; i < GetUnitCount(); ++i)
	{
		if (units_[i]->IsAvailable())
		{
			return units_[i]->DispatchToPosition(targetPosition, requestedEnergy);
		}
	}
	return false;
}

Unit* UnitManager::SpawnTutorialStaticUnit(const Vector3& position)
{
	for (size_t i = 0; i < GetUnitCount(); ++i)
	{
		if (units_[i]->SetUpTutorialStatic(position))
		{
			return units_[i].get();
		}
	}
	return nullptr;
}

void UnitManager::RecallTutorialStaticUnit(Unit* unit)
{
	if (unit)
	{
		unit->Recall();
	}
}

Unit* UnitManager::FindNearestCarryingUnit(const Vector3& position, float maxDistance) const 
{
	Unit* nearest = nullptr;
	const float safeMaxDistance = (std::max)(maxDistance, 0.0f);
	float nearestDistanceSquared = safeMaxDistance * safeMaxDistance;

	// Enemyの索敵用。sqrtを避けてXZ距離の二乗で最短個体を比較する。
	for (size_t i = 0; i < GetUnitCount(); ++i)
	{
		Unit* unit = units_[i].get();
		if (!unit->IsCarryingEnergy()) 
		{
			continue;
		}

		const Vector3 offset = unit->GetPosition() - position;
		const float distanceSquared = offset.x * offset.x + offset.z * offset.z;
		if (distanceSquared <= nearestDistanceSquared) 
		{
			nearest = unit;
			nearestDistanceSquared = distanceSquared;
		}
	}

	return nearest;
}

void UnitManager::RecallAll() 
{
	for (auto& unit : units_)
	{
		unit->Recall();
	}
}

bool UnitManager::InjectEnergyToUnitsAt(const Vector3& position, float radius, int32_t amount, bool* outInjectedAny)
{
	if (!gameplayEnabled_) return false;

	const float radiusSq = radius * radius;
	bool hasUnitInRadius = false;
	if (outInjectedAny) *outInjectedAny = false;

	for (size_t i = 0; i < GetUnitCount(); ++i)
	{
		Unit* unit = units_[i].get();
		if (!unit || !unit->IsDeployed() || unit->IsTutorialStatic()) continue;

		const Vector3 offset = unit->GetPosition() - position;
		const float distSq = offset.x * offset.x + offset.z * offset.z;

		if (distSq <= radiusSq)
		{
			// カーソル範囲内に存在するためハイライト化
			unit->Highlight();
			hasUnitInRadius = true;

			// 注入要求量が存在する場合のみ注入処理を行う
			if (amount > 0 && unit->InjectEnergy(amount))
			{
				if (outInjectedAny) *outInjectedAny = true;
			}
		}
	}

	return hasUnitInRadius;
}

size_t UnitManager::GetAvailableCount() const 
{
	size_t count = 0;
	for (size_t i = 0; i < GetUnitCount(); ++i) 
	{
		if (units_[i]->IsAvailable()) 
		{
			++count;
		}
	}
	return count;
}

size_t UnitManager::GetDeployedCount() const 
{
	return static_cast<size_t>(std::count_if(
		units_.begin(),
		units_.begin() + GetUnitCount(),
		[](const auto& unit) { return unit->IsDeployed(); }));
}

void UnitManager::ApplyDebugParameters()
{
	debugParameter_->ApplyIfDirty();
	SanitizeSettings();
}

void UnitManager::SanitizeSettings()
{
	settings_.unitCount = (std::clamp)(settings_.unitCount, 1, static_cast<int32_t>(units_.size()));
	settings_.unit.scale.x = (std::max)(settings_.unit.scale.x, 0.0f);
	settings_.unit.scale.y = (std::max)(settings_.unit.scale.y, 0.0f);
	settings_.unit.scale.z = (std::max)(settings_.unit.scale.z, 0.0f);
	settings_.unit.launchDistance = (std::max)(settings_.unit.launchDistance, 0.0f);
	settings_.unit.normalSpeed = (std::max)(settings_.unit.normalSpeed, 0.0f);
	settings_.unit.boostedSpeed = (std::max)(settings_.unit.boostedSpeed, settings_.unit.normalSpeed);
	settings_.unit.pickupRadius = (std::max)(settings_.unit.pickupRadius, 0.0f);
	settings_.unit.deliveryRadius = (std::max)(settings_.unit.deliveryRadius, 0.0f);
	settings_.unit.collisionRadius = (std::max)(settings_.unit.collisionRadius, 0.0f);
	settings_.unit.staminaDrainPerSecond = (std::max)(settings_.unit.staminaDrainPerSecond, 0.0f);
	settings_.unit.distanceDrainRate = (std::max)(settings_.unit.distanceDrainRate, 0.0f);

	settings_.unit.bhEnergyThreshold = (std::max)(settings_.unit.bhEnergyThreshold, 1);
	settings_.unit.bhDuration = (std::max)(settings_.unit.bhDuration, 0.1f);
	settings_.unit.bhPullSpeed = (std::max)(settings_.unit.bhPullSpeed, 0.0f);
	settings_.unit.bhKillRadius = (std::max)(settings_.unit.bhKillRadius, 0.0f);
	settings_.unit.bhBaseRadius = (std::max)(settings_.unit.bhBaseRadius, 0.0f);
	settings_.unit.bhNearDistance = (std::max)(settings_.unit.bhNearDistance, 0.0f);
	settings_.unit.bhFarDistance = (std::max)(settings_.unit.bhFarDistance, settings_.unit.bhNearDistance);
	settings_.unit.bhRadiusNearMultiplier = (std::max)(settings_.unit.bhRadiusNearMultiplier, 0.0f);
	settings_.unit.bhRadiusMidMultiplier = (std::max)(settings_.unit.bhRadiusMidMultiplier, 0.0f);
	settings_.unit.bhRadiusFarMultiplier = (std::max)(settings_.unit.bhRadiusFarMultiplier, 0.0f);
	settings_.unit.bhSpecialMultiplier = (std::max)(settings_.unit.bhSpecialMultiplier, 0.0f);
}

void UnitManager::ApplyUnitCount()
{
	// 実行中に参加数を減らした場合、範囲外になった個体の予約を安全に解放する。
	for (size_t i = GetUnitCount(); i < units_.size(); ++i) {
		if (!units_[i]->IsAvailable()) 
		{
			units_[i]->Recall();
		}
	}
}

void UnitManager::DrawDebugWindow() 
{
#ifdef USE_IMGUI
	if (!ImGui::Begin("Units")) 
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Stored: %zu", GetAvailableCount());
	ImGui::Text("Deployed: %zu", GetDeployedCount());
	if (ImGui::Button("Recall All")) 
	{
		RecallAll();
	}

	ImGui::End();
#endif
}

