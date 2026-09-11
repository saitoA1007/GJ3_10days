#include "Unit.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "AudioManager.h"
#include "RenderQueue.h"

#include "Application/Rocket/Rocket.h"
#include "Application/Enemy/Enemy.h"
#include "Application/CollisionConfig.h"
#include "Application/Energy/EnergyPickup.h"
#include "Application/Energy/EnergySpawner.h"
#include "UnitEffectManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

namespace
{
	constexpr float kPi = 3.1415926535f;
	constexpr const char* kBlackholeSpawnSoundName = "horl.mp3";
	constexpr const char* kUnitEnergySoundName = "unitEnergy.mp3";
	constexpr float kBlackholeSpawnSoundVolume = 1.0f;
	constexpr float kUnitEnergySoundVolume = 1.0f;

	void PlayBlackholeSpawnSound()
	{
		auto& audioManager = AudioManager::GetInstance();
		const uint32_t soundHandle =
			audioManager.GetHandleByName(kBlackholeSpawnSoundName);
		audioManager.Stop(soundHandle);
		audioManager.Play(soundHandle, kBlackholeSpawnSoundVolume, false);
	}

	void PlayUnitEnergySound()
	{
		auto& audioManager = AudioManager::GetInstance();
		const uint32_t soundHandle =
			audioManager.GetHandleByName(kUnitEnergySoundName);
		audioManager.Stop(soundHandle);
		audioManager.Play(soundHandle, kUnitEnergySoundVolume, false);
	}
}

UnitEffectManager* Unit::unitEffectManager_ = nullptr;

void Unit::StaticInitialize(UnitEffectManager* unitEffectManager) {
	unitEffectManager_ = unitEffectManager;
}

Unit::Unit(GameEngine::Model* model, GameEngine::Model* circleModel,
	Rocket* rocket, const UnitSettings* settings,
	GameEngine::Model* bameModel, uint32_t beamGH,
	GameEngine::Model* arrowModel, uint32_t lineGH,
	EnergySpawner* energySpawner, GameEngine::Model* markerModel)
	: rocket_(rocket), settings_(settings), RopeEffect_(bameModel, beamGH), energySpawner_(energySpawner), NaviEffect_(bameModel, lineGH, arrowModel)
{
	modelComponent_ = std::make_unique<ModelComponent>(model);
	modelComponent_->materialData_->enableLighting = true;
	modelComponent_->materialData_->metallic = 0.3f;

	if (circleModel)
	{
		blackholeModel_ = std::make_unique<GameEngine::ModelComponent>(circleModel);
		blackholeModel_->materialData_->enableLighting = false; // エフェクトなのでライティング不要
		blackholeModel_->materialData_->color = { 0.1f, 0.1f, 0.1f, 0.8f }; // 黒系の半透明
	}

	if (markerModel)
	{
		targetMarkerModel_ = std::make_unique<GameEngine::ModelComponent>(markerModel);
		targetMarkerModel_->materialData_->enableLighting = false; 
	}

	// コライダーのセットアップ
	GameEngine::UserData userData;
	userData.typeID = uint32_t(CollisionTypeID::kUnit);
	userData.object = this;

	collider_.SetCollisionAttribute(uint32_t(CollisionTypeID::kUnit));
	// 敵との衝突を検知できるように、kEnemyをマスクに含める
	collider_.SetCollisionMask(uint32_t(CollisionTypeID::kEnemy));
	collider_.SetUserData(userData);
	collider_.SetRadius(settings_->collisionRadius);

	// 最初は待機状態なのでOFF
	collider_.SetActive(false);

	NaviEffect_.Initialize();
}

void Unit::Initialize() 
{
	// 再初期化前に確保していたEnergyがあれば、消さずに地面へ戻す
	if (targetEnergy_ && targetEnergy_->IsActive()) 
	{
		targetEnergy_->DropOnGround(targetEnergy_->GetPosition());
	}

	targetEnergy_ = nullptr;
	targetEnemy_ = nullptr;
	state_ = UnitState::Stored;
	stamina_ = 0.0f;
	maxStamina_ = 0.0f;
	position_ = rocket_->GetPosition() + settings_->launchOffset;
	position_.y = settings_->groundY;
	collider_.SetActive(false);
	blackholeTimer_ = 0.0f;
	highlightTimer_ = 0.0f;
	absorbedBasePoint_ = 0;
	bounceElapsedTime_ = 0.0f;
	injectionTimer_ = 0.0f;
	injectionScaleElapsedTime_ = 0.0f;
	SyncModel();
}

void Unit::Update()
{
	if (highlightTimer_ > 0.0f)
	{
		highlightTimer_ -= FpsCounter::deltaTime;
	}

	if (injectionTimer_ > 0.0f) {
		injectionTimer_ -= FpsCounter::deltaTime;
	}

	UpdateBounce(FpsCounter::deltaTime);
	UpdateInjectionScale(FpsCounter::deltaTime);

	// 各状態の責務を分け、遷移は到達・衝突が成立した関数内だけで行う
	switch (state_) {
	case UnitState::Stored:
		return;
	case UnitState::MovingToEnergy:
		UpdateMovingToEnergy(FpsCounter::deltaTime);
		break;
	case UnitState::MovingToEnemy:
		UpdateMovingToEnemy(FpsCounter::deltaTime);
		break;
	case UnitState::MovingToPosition:
		UpdateMovingToPosition(FpsCounter::deltaTime);
		break;
	case UnitState::TutorialStatic:
		collider_.SetActive(false);
		SyncModel();
		return;
	case UnitState::ReturningToRocket:
		UpdateReturningToRocket(FpsCounter::deltaTime);
		break;
	case UnitState::Blackhole:
		UpdateBlackhole(FpsCounter::deltaTime);
		break;
	}

	// maxChargeEnergyCost に対する現在のスタミナ比率を計算 (0.0 ～ 1.0 にクランプ)
	const float maxChargeEnergy = static_cast<float>(settings_->bhEnergyThreshold);
	const float staminaRatio = (maxChargeEnergy > 0.0f)
		? std::clamp(stamina_ / maxChargeEnergy, 0.0f, 1.0f)
		: 0.0f;

	constexpr float minScale = 0.05f;
	constexpr float maxScale = 1.8f;

	const float currentScale = minScale + (maxScale - minScale) * staminaRatio;
	RopeEffect_.SetScale(currentScale);

	RopeEffect_.Start(GetVisualPosition(), { 0.0f, 5.0f, 0.0f });
	RopeEffect_.Update();

	NaviEffect_.Start(GetVisualPosition(), targetPosition_);
	NaviEffect_.Update();

	SyncModel();

	if (isBeingPulledByBlackhole_) {
		collider_.SetActive(false);
		isBeingPulledByBlackhole_ = false; // 次フレーム判定用にリセット
	}
	else if (IsDeployed() && state_ != UnitState::Blackhole) {
		collider_.SetActive(true);  // 通常出撃中のみ有効化
	}
	else {
		collider_.SetActive(false); // 待機中またはブラックホール中は無効化
	}
}

void Unit::Draw()
{
	if (IsDeployed() && state_ != UnitState::Blackhole)
	{
		modelComponent_->DrawRaytracing(renderQueue_);
		if (!IsTutorialStatic())
		{
			RopeEffect_.Draw();
		}
	}

	if (state_ == UnitState::Blackhole && blackholeModel_)
	{
		/*blackholeModel_->DrawRaytracing(renderQueue_);*/
	}

	if (state_ == UnitState::MovingToPosition && targetMarkerModel_)
	{
		targetMarkerModel_->Draw(renderQueue_);
		NaviEffect_.Draw();
	}
}

void Unit::RefreshVisual() 
{
	if (targetEnergy_ && targetEnergy_->IsCarried()) {
		targetEnergy_->SetCarriedPosition(GetVisualPosition() + settings_->carryOffset);
	}
	SyncModel();
}

bool Unit::DispatchToEnergy(EnergyPickup* target, int32_t requestedEnergy)
{
	// 対象予約が成功してからEnergyを消費し、派遣失敗時の誤消費を防ぐ。
	if (!target || !IsAvailable() || !target->TryReserve())
	{
		return false;
	}

	AllocateStamina(requestedEnergy);
	SetLaunchPositionTowards(target->GetPosition());
	targetEnergy_ = target;
	targetEnemy_ = nullptr;
	state_ = UnitState::MovingToEnergy;
	collider_.SetActive(true);
	SyncModel();
	return true;
}

bool Unit::DispatchToEnemy(Enemy* target, int32_t requestedEnergy) 
{
	// 敵も予約制にし、同じ敵へ複数体が同時出撃するのを防ぐ
	if (!target || !IsAvailable() || !target->TryReserveForAttack()) 
	{
		return false;
	}

	AllocateStamina(requestedEnergy);
	SetLaunchPositionTowards(target->GetPosition());
	targetEnergy_ = nullptr;
	targetEnemy_ = target;
	state_ = UnitState::MovingToEnemy;
	collider_.SetActive(true);
	SyncModel();
	return true;
}


bool Unit::DispatchToPosition(const Vector3& targetPosition, int32_t requestedEnergy)
{
	if (!IsAvailable())
	{
		return false;
	}

	AllocateStamina(requestedEnergy);
	targetEnergy_ = nullptr;
	targetEnemy_ = nullptr;
	targetPosition_ = targetPosition;
	targetPosition_.y = settings_->groundY;
	SetLaunchPositionTowards(targetPosition_);
	state_ = UnitState::MovingToPosition;

	if (targetMarkerModel_)
	{
		targetMarkerModel_->worldTransform_.transform_.translate = targetPosition_;
		targetMarkerModel_->worldTransform_.transform_.translate.y = settings_->groundY + 0.02f;
		targetMarkerModel_->Update();
	}

	collider_.SetActive(true);
	SyncModel();
	return true;
}

bool Unit::SetUpTutorialStatic(const Vector3& position)
{
	if (!IsAvailable())
	{
		return false;
	}

	targetEnergy_ = nullptr;
	targetEnemy_ = nullptr;
	targetPosition_ = position;
	position_ = position;
	position_.y = settings_->groundY;
	stamina_ = 0.0f;
	maxStamina_ = 0.0f;
	state_ = UnitState::TutorialStatic;
	collider_.SetActive(false);
	SyncModel();
	return true;
}

bool Unit::DefeatAndDropEnergy() 
{
	if (!IsCarryingEnergy())
	{
		return false;
	}

	// 運搬物だけを接触地点へ残し、Unitはプールから削除せず再出撃可能に
	targetEnergy_->DropOnGround(position_);
	targetEnergy_ = nullptr;
	ReturnToStorageAfterDefeat();
	return true;
}

void Unit::Recall() 
{
	if (targetEnergy_ && targetEnergy_->IsActive())
	{
		targetEnergy_->DropOnGround(targetEnergy_->GetPosition());
	}
	if (targetEnemy_)
	{
		targetEnemy_->CancelAttackReservation();
	}

	targetEnergy_ = nullptr;
	targetEnemy_ = nullptr;
	state_ = UnitState::Stored;
	stamina_ = 0.0f;
	maxStamina_ = 0.0f;
	position_ = rocket_->GetPosition() + settings_->launchOffset;
	position_.y = settings_->groundY;
	collider_.SetActive(false);
	blackholeTimer_ = 0.0f;
	absorbedBasePoint_ = 0;
	bounceElapsedTime_ = 0.0f;
	injectionTimer_ = 0.0f;
	injectionScaleElapsedTime_ = 0.0f;
	SyncModel();
}

bool Unit::IsCarryingEnergy() const 
{
	return state_ == UnitState::ReturningToRocket &&
		targetEnergy_ != nullptr &&
		targetEnergy_->IsCarried();
}

void Unit::UpdateMovingToEnergy(float deltaTime) {
	// 他処理で予約が解除された場合は、無効なポインタを追わず帰還扱いに
	if (!targetEnergy_ || !targetEnergy_->IsReserved()) {
		Recall();
		return;
	}

	if (!IsBeingInjected()) {
		MoveTowards(targetEnergy_->GetPosition(), deltaTime);
		ConsumeStamina(deltaTime);
	}

	const float pickupRadiusSquared = settings_->pickupRadius * settings_->pickupRadius;
	if (DistanceSquaredXZ(position_, targetEnergy_->GetPosition()) <= pickupRadiusSquared) {
		// 回収成立後は同じEnergyを保持したまま帰還状態へ遷移
		if (targetEnergy_->BeginCarry())
		{
			targetEnergy_->SetCarriedPosition(GetVisualPosition() + settings_->carryOffset);
			state_ = UnitState::ReturningToRocket;
			PlayUnitEnergySound();
		}
		else 
		{
			Recall();
		}
	}
}

void Unit::UpdateMovingToEnemy(float deltaTime) 
{
	if (!targetEnemy_ || !targetEnemy_->IsActive())
	{
		Recall();
		return;
	}

	if (!IsBeingInjected()) {
		MoveTowards(targetEnemy_->GetPosition(), deltaTime); 
		ConsumeStamina(deltaTime);
	}

}

void Unit::UpdateMovingToPosition(float deltaTime)
{
	if (!IsBeingInjected()) {
		MoveTowards(targetPosition_, deltaTime);
		ConsumeStamina(deltaTime);
	}

	if (targetMarkerModel_)
	{
		targetMarkerModel_->worldTransform_.transform_.rotate.y += 2.5f * deltaTime;

		const float phase = targetMarkerModel_->worldTransform_.transform_.rotate.y;

		
		const float baseScale = 0.7f; 
		const float pulseHorizontal = std::sin(phase * 2.0f) * 0.15f; 
		const float pulseVertical = std::cos(phase * 2.0f) * 0.10f; 

		targetMarkerModel_->worldTransform_.transform_.scale = {
			baseScale + pulseHorizontal,
			baseScale + pulseVertical,
			baseScale + pulseHorizontal
		};

		const float floatOffset = (std::sin(phase * 3.0f) + 1.0f) * 0.03f; 

		targetMarkerModel_->worldTransform_.transform_.translate = targetPosition_;
		targetMarkerModel_->worldTransform_.transform_.translate.y = settings_->groundY + 0.02f + floatOffset;

		targetMarkerModel_->Update();
	}

	const float arrivalRadiusSquared = settings_->pickupRadius * settings_->pickupRadius;
	if (DistanceSquaredXZ(position_, targetPosition_) <= arrivalRadiusSquared)
	{
		state_ = UnitState::ReturningToRocket;
	}
}

void Unit::UpdateReturningToRocket(float deltaTime)
{
	if (targetEnergy_ && !targetEnergy_->IsCarried())
	{
		Recall();
		return;
	}

	if (!IsBeingInjected()) {
		MoveTowards(rocket_->GetPosition(), deltaTime);
		ConsumeStamina(deltaTime);
	}

	if (targetEnergy_ && targetEnergy_->IsCarried())
	{
		targetEnergy_->SetCarriedPosition(GetVisualPosition() + settings_->carryOffset);
	}

	const float deliveryRadiusSquared = settings_->deliveryRadius * settings_->deliveryRadius;
	if (DistanceSquaredXZ(position_, rocket_->GetPosition()) <= deliveryRadiusSquared)
	{
		if (targetEnergy_ && targetEnergy_->IsCarried())
		{
			rocket_->DepositEnergy(targetEnergy_->Deliver());
		}

		targetEnergy_ = nullptr;
		targetEnemy_ = nullptr;
		state_ = UnitState::Stored;
		stamina_ = 0.0f;
		maxStamina_ = 0.0f;
		collider_.SetActive(false);
	}
}

void Unit::AllocateStamina(int32_t requestedEnergy) 
{
	// EnergyChange.amountは消費時に負数なので、符号を反転してスタミナ残量にする
	stamina_ = static_cast<float>((std::max)(requestedEnergy, 0));
	maxStamina_ = static_cast<float>(settings_->bhEnergyThreshold);
}

void Unit::SetLaunchPositionTowards(const Vector3& destination)
{
	bounceElapsedTime_ = 0.0f;
	position_ = rocket_->GetPosition() + settings_->launchOffset;
	position_.y = settings_->groundY;

	Vector3 direction = destination - position_;
	direction.y = 0.0f;
	const float distance = direction.Length();
	if (distance <= 0.0f)
	{
		return;
	}

	direction.Normalize();
	position_ += direction * (std::min)(settings_->launchDistance, distance);
}

void Unit::ReturnToStorageAfterDefeat()
{
	// 移動中に予約していた Energy があれば予約を解除
	if (targetEnergy_ && targetEnergy_->IsActive())
	{
		if (targetEnergy_->IsCarried())
		{
			// 運搬中だった場合はその場に落とす
			targetEnergy_->DropOnGround(position_);
		}
		else
		{
			// 移動中だった場合は予約を解除して拾えるようにする
			targetEnergy_->DropOnGround(targetEnergy_->GetPosition());
		}
	}

	// 攻撃予約していた Enemy があれば予約を解除
	if (targetEnemy_ && targetEnemy_->IsActive())
	{
		targetEnemy_->CancelAttackReservation();
	}

	// 状態のクリア
	targetEnergy_ = nullptr;
	targetEnemy_ = nullptr;
	state_ = UnitState::Stored;
	stamina_ = 0.0f;
	maxStamina_ = 0.0f;
	position_ = rocket_->GetPosition() + settings_->launchOffset;
	position_.y = settings_->groundY;
	bounceElapsedTime_ = 0.0f;
	injectionTimer_ = 0.0f;
	injectionScaleElapsedTime_ = 0.0f;

	// コライダーの無効化（待機状態にするため）
	collider_.SetActive(false);
	SyncModel();
}

bool Unit::InjectEnergy(int32_t requestedAmount)
{
	if (!IsDeployed())
	{
		return false;
	}

	const float upperLimit = static_cast<float>(settings_->bhEnergyThreshold);
	const float staminaDeficit = upperLimit - stamina_;

	if (staminaDeficit <= 0.0f)
	{
		return false;
	}

	const int32_t actualRequest = (std::min)(requestedAmount, static_cast<int32_t>(std::ceil(staminaDeficit)));

	if (actualRequest <= 0)
	{
		return false;
	}

	const EnergyChange change = rocket_->AllocateEnergyToUnit(actualRequest);
	const int32_t actualAllocated = std::abs(change.amount);

	if (actualAllocated <= 0)
	{
		return false;
	}

	injectionTimer_ = 0.15f;
	bounceElapsedTime_ = 0.0f;

	stamina_ = (std::min)(stamina_ + static_cast<float>(actualAllocated), upperLimit);
	// UnitManagerより後に注入処理が走るため、このフレーム中に地面へ戻しておく。
	SyncModel();

	// スタミナが閾値以上になったらブラックホール化
	if (stamina_ >= settings_->bhEnergyThreshold)
	{
		ActivateBlackhole();
	}

	return true;
}

bool Unit::ActivateBlackhole()
{
	if (!IsDeployed() || !settings_ || !modelComponent_)
	{
		return false;
	}

	state_ = UnitState::Blackhole;
	blackholeTimer_ = settings_->bhDuration;
	absorbedBasePoint_ = 0;
	bhEffectTimer_ = 0.0f;

	// 通常プレイと同じブラックホール演出を現在位置から開始する。
	if (unitEffectManager_)
	{
		const Vector3 position = modelComponent_->worldTransform_.transform_.translate;
		unitEffectManager_->StartBlackHole(position, GetBlackholeRadius() * 0.2f);
	}
	PlayBlackholeSpawnSound();

	collider_.SetActive(false);

	if (targetEnergy_)
	{
		if (targetEnergy_->IsCarried())
		{
			targetEnergy_->DropOnGround(position_);
		}
		else
		{
			targetEnergy_->DropOnGround(targetEnergy_->GetPosition());
		}
		targetEnergy_ = nullptr;
	}

	if (targetEnemy_)
	{
		targetEnemy_->CancelAttackReservation();
		targetEnemy_ = nullptr;
	}

	return true;
}

float Unit::GetBlackholeRadius() const
{
	// ロケットからの距離を計算
	float distToRocket = std::sqrt(DistanceSquaredXZ(position_, rocket_->GetPosition()));
	float multiplier = 1.0f;

	// 距離に応じて倍率を変化
	if (distToRocket <= settings_->bhNearDistance)
	{
		// 近距離帯
		float t = distToRocket / settings_->bhNearDistance;
		multiplier = std::lerp(settings_->bhRadiusNearMultiplier, settings_->bhRadiusMidMultiplier, t);
	}
	else if (distToRocket <= settings_->bhFarDistance)
	{
		// 中間～遠距離帯
		float t = (distToRocket - settings_->bhNearDistance) / (settings_->bhFarDistance - settings_->bhNearDistance);
		multiplier = std::lerp(settings_->bhRadiusMidMultiplier, settings_->bhRadiusFarMultiplier, t);
	}
	else
	{
		// 外側の最大倍率で固定
		multiplier = settings_->bhRadiusFarMultiplier;
	}

	return settings_->bhBaseRadius * multiplier;
}

void Unit::ProcessBlackholeAbsorption(
	const std::vector<Enemy*>& enemies,
	const std::vector<Unit*>& units,
	const std::vector<EnergyPickup*>& energies,
	float deltaTime)
{
	if (state_ != UnitState::Blackhole) return;

	const float radiusSq = std::pow(GetBlackholeRadius(), 2);
	const float killRadiusSq = std::pow(settings_->bhKillRadius, 2);
	const float rotateSpeed = settings_->bhPullSpeed * 2.0f; // 回転速度

	// 敵の吸い込み
	for (auto* enemy : enemies)
	{
		if (!enemy || !enemy->IsAlive()) continue;

		float distSq = DistanceSquaredXZ(position_, enemy->GetPosition());
		if (distSq <= radiusSq)
		{
			if (distSq <= killRadiusSq)
			{
				enemy->ForceDestroy();
				absorbedBasePoint_ += 1;
			}
			else
			{
				enemy->PullTowards(position_, settings_->bhPullSpeed, deltaTime);
			}
		}
	}

	// 他のユニットの吸い込み
	for (auto* unit : units)
	{
		if (unit == this || !unit->IsDeployed()) continue;
		if (unit->IsBlackhole()) continue;

		float distSq = DistanceSquaredXZ(position_, unit->GetPosition());
		if (distSq <= radiusSq)
		{
			if (distSq <= killRadiusSq)
			{
				unit->Recall();
				absorbedBasePoint_ += 1;
			}
			else
			{
				unit->SetBeingPulledByBlackhole(true);

				// 渦巻きベクトルの計算
				Vector3 pullDir = position_ - unit->position_;
				pullDir.y = 0.0f;
				pullDir.Normalize();

				Vector3 tangentDir = { -pullDir.z, 0.0f, pullDir.x };
				Vector3 velocity = (pullDir * settings_->bhPullSpeed) + (tangentDir * rotateSpeed);

				unit->position_ += velocity * deltaTime;
			}
		}
	}

	// エネルギーの吸い込み
	for (auto* energy : energies)
	{
		if (!energy || !energy->IsActive() || energy->IsCarried() || energy->GetSize() == EnergySize::Special) continue; if (!energy || !energy->IsActive() || energy->IsCarried()) continue;

		float distSq = DistanceSquaredXZ(position_, energy->GetPosition());
		if (distSq <= radiusSq)
		{
			if (distSq <= killRadiusSq)
			{
				switch (energy->GetSize())
				{
				case EnergySize::Small:   absorbedBasePoint_ += 1; break;
				case EnergySize::Medium:  absorbedBasePoint_ += 2; break;
				case EnergySize::Large:   absorbedBasePoint_ += 3; break;
				}
				energy->Deactivate();
			}
			else
			{
				// 渦巻きベクトルの計算
				Vector3 pullDir = position_ - energy->GetPosition();
				pullDir.y = 0.0f;
				pullDir.Normalize();

				Vector3 tangentDir = { -pullDir.z, 0.0f, pullDir.x };
				Vector3 velocity = (pullDir * settings_->bhPullSpeed) + (tangentDir * rotateSpeed);

				energy->SetPosition(energy->GetPosition() + velocity * deltaTime);
			}
		}
	}
}

void Unit::Highlight()
{
	highlightTimer_ = 0.1f;
}

void Unit::RefreshInjectionAnimation()
{
	if (IsDeployed())
	{
		injectionTimer_ = 0.15f;
	}
}

void Unit::UpdateBlackhole(float deltaTime)
{
	blackholeTimer_ -= deltaTime;
	bhEffectTimer_ += deltaTime;

	// ブラックホールエフェクトの更新
	if (blackholeModel_)
	{
		constexpr float kEffectSpeed = 1.5f;

		float loopProgress = std::fmod(bhEffectTimer_ * kEffectSpeed, 1.0f);
		float scaleRatio = 1.0f - loopProgress;

		float currentMaxRadius = GetBlackholeRadius();

		blackholeModel_->worldTransform_.transform_.scale =
		{
			currentMaxRadius * scaleRatio,
			1.0f, 
			currentMaxRadius * scaleRatio
		};

		blackholeModel_->worldTransform_.transform_.translate = position_;
		blackholeModel_->worldTransform_.transform_.translate.y = settings_->groundY + 0.05f;

		blackholeModel_->materialData_->color.w = scaleRatio * 0.8f;

		blackholeModel_->Update();
	}

	if (blackholeTimer_ <= 0.0f)
	{
		GenerateSpecialEnergy();
		Recall();
	}
}

void Unit::GenerateSpecialEnergy()
{
	// 何も吸い込んでいなければ生成しない
	if (absorbedBasePoint_ <= 0) return;

	// 最終的な獲得量
	int32_t finalValue = static_cast<int32_t>(absorbedBasePoint_ * settings_->bhSpecialMultiplier);

	// Special のエネルギーをドロップ
	if (energySpawner_)
	{
		EnergyPickup* specialEnergy = energySpawner_->SpawnOnGround(EnergySize::Special, position_);

		if (specialEnergy)
		{
			// 計算した獲得量を上書き
			specialEnergy->SetCustomValue(finalValue);
		}
	}
}

void Unit::StartCarryingEnergy(EnergyPickup* energy)
{
	if (!energy)
	{
		ReturnToStorageAfterDefeat();
		return;
	}

	if (targetEnemy_)
	{
		targetEnemy_->CancelAttackReservation();
		targetEnemy_ = nullptr;
	}

	if (targetEnergy_ && targetEnergy_ != energy && targetEnergy_->IsActive())
	{
		targetEnergy_->DropOnGround(targetEnergy_->GetPosition());
		targetEnergy_ = nullptr;
	}

	// 生成されたEnergyの予約と運搬処理
	if (energy->TryReserve() && energy->BeginCarry())
	{
		targetEnergy_ = energy;
		targetEnergy_->SetCarriedPosition(GetVisualPosition() + settings_->carryOffset);
		state_ = UnitState::ReturningToRocket;
		targetEnemy_ = nullptr;
		PlayUnitEnergySound();
	}
	else
	{
		ReturnToStorageAfterDefeat();
	}
	SyncModel();
}

void Unit::MoveTowards(const Vector3& target, float deltaTime) 
{
	Vector3 direction = target - position_;
	direction.y = 0.0f;
	const float distance = direction.Length();
	if (distance <= 0.0001f)
	{
		return;
	}

	direction.Normalize();
	// スタミナが1以上ではなく、わずかでも残っていれば高速移動を選ぶ
	const float speed = stamina_ > 0.0f ? settings_->boostedSpeed : settings_->normalSpeed;
	const float moveDistance = (std::min)(speed * (std::max)(deltaTime, 0.0f), distance);
	position_ += direction * moveDistance;
	modelComponent_->worldTransform_.transform_.rotate.y = std::atan2(direction.x, direction.z);
}

void Unit::ConsumeStamina(float deltaTime) 
{
	if (stamina_ <= 0.0f) 
	{
		return;
	}

	const float distanceFromRocket = std::sqrt(DistanceSquaredXZ(position_, rocket_->GetPosition()));
	// ロケットから離れるほど倍率を線形に増やし、遠距離派遣のコストを高くする
	const float distanceMultiplier = 1.0f + distanceFromRocket * settings_->distanceDrainRate;
	const float consumed = settings_->staminaDrainPerSecond * distanceMultiplier * (std::max)(deltaTime, 0.0f);
	stamina_ = (std::max)(stamina_ - consumed, 0.0f);
}

void Unit::UpdateBounce(float deltaTime)
{
	if (!ShouldBounce())
	{
		// 注入中や移動終了時は、次の描画から必ず地面へ戻す。
		bounceElapsedTime_ = 0.0f;
		return;
	}

	bounceElapsedTime_ += (std::max)(deltaTime, 0.0f);
}

bool Unit::ShouldBounce() const
{
	if (!settings_->bounceEnabled ||
		settings_->bounceHeight <= 0.0f ||
		settings_->bounceFrequency <= 0.0f ||
		IsBeingInjected())
	{
		return false;
	}

	return state_ == UnitState::MovingToEnergy ||
		state_ == UnitState::MovingToEnemy ||
		state_ == UnitState::MovingToPosition ||
		state_ == UnitState::ReturningToRocket;
}

Vector3 Unit::GetVisualPosition() const
{
	Vector3 visualPosition = position_;
	if (ShouldBounce())
	{
		// abs(sin)で地面より下へ潜らず、Frequency回/秒のジャンプを作る。
		const float phase = bounceElapsedTime_ * settings_->bounceFrequency * kPi;
		visualPosition.y += std::abs(std::sin(phase)) * settings_->bounceHeight;
	}
	return visualPosition;
}

void Unit::UpdateInjectionScale(float deltaTime)
{
	if (!IsBeingInjected())
	{
		injectionScaleElapsedTime_ = 0.0f;
		return;
	}

	injectionScaleElapsedTime_ += (std::max)(deltaTime, 0.0f);
}

Vector3 Unit::GetAnimatedScale() const
{
	Vector3 animatedScale = settings_->scale;
	if (!IsBeingInjected() ||
		settings_->injectionScaleAmplitude <= 0.0f ||
		settings_->injectionScaleFrequency <= 0.0f)
	{
		return animatedScale;
	}

	const float phase = injectionScaleElapsedTime_ * settings_->injectionScaleFrequency * kPi * 2.0f;
	const float scaleFactor = (std::max)(
		1.0f + std::sin(phase) * settings_->injectionScaleAmplitude,
		0.0f);
	animatedScale.x *= scaleFactor;
	animatedScale.y *= scaleFactor;
	animatedScale.z *= scaleFactor;
	return animatedScale;
}

float Unit::DistanceSquaredXZ(const Vector3& a, const Vector3& b) const
{
	const float x = a.x - b.x;
	const float z = a.z - b.z;
	return x * x + z * z;
}

void Unit::SyncModel() 
{
	modelComponent_->worldTransform_.transform_.scale = GetAnimatedScale();
	modelComponent_->worldTransform_.transform_.translate = GetVisualPosition();
	if (collider_.IsActive())
	{
		collider_.SetWorldPosition(position_);
	}

	// Push入力中かつ範囲内にいる場合はハイライトカラー
	if (highlightTimer_ > 0.0f)
	{
		modelComponent_->materialData_->color = stamina_ > 0.0f
			? settings_->staminaColor * 10.0f
			: settings_->normalColor * 10.0f;
	}
	else
	{
		// 水色なら高速移動可能、通常色ならスタミナ切れであることを示す
		modelComponent_->materialData_->color = stamina_ > 0.0f
			? settings_->staminaColor
			: settings_->normalColor;
	}
	modelComponent_->Update();
}

