#include "Unit.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "RenderQueue.h"

#include "Application/Energy/EnergyPickup.h"
#include "Application/Rocket/Rocket.h"
#include "Application/Enemy/Enemy.h"
#include "Application/CollisionConfig.h"
#include "FPSCounter.h"

using namespace GameEngine;

Unit::Unit(Model* model, Rocket* rocket, const UnitSettings* settings)
	: rocket_(rocket), settings_(settings) 
{
	assert(model != nullptr && "unit requires unit.obj");
	assert(rocket_ != nullptr && "unit requires a rocket");
	assert(settings_ != nullptr && "unit requires settings");

	modelComponent_ = std::make_unique<ModelComponent>(model);
	modelComponent_->materialData_->enableLighting = true;

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
	position_ = rocket_->GetPosition() + settings_->launchOffset;
	position_.y = settings_->groundY;
	collider_.SetActive(false);
	SyncModel();
}

void Unit::Update()
{
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
	case UnitState::ReturningToRocket:
		UpdateReturningToRocket(FpsCounter::deltaTime);
		break;
	}

	SyncModel();
}

void Unit::Draw()
{
	if (IsDeployed())
	{
		modelComponent_->DrawRaytracing(renderQueue_);
	}
}

void Unit::RefreshVisual() 
{
	if (targetEnergy_ && targetEnergy_->IsCarried()) {
		targetEnergy_->SetCarriedPosition(position_ + settings_->carryOffset);
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
	position_ = rocket_->GetPosition() + settings_->launchOffset;
	position_.y = settings_->groundY;
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
	position_ = rocket_->GetPosition() + settings_->launchOffset;
	position_.y = settings_->groundY;
	targetEnergy_ = nullptr;
	targetEnemy_ = target;
	state_ = UnitState::MovingToEnemy;
	collider_.SetActive(true);
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
	position_ = rocket_->GetPosition() + settings_->launchOffset;
	position_.y = settings_->groundY;
	collider_.SetActive(false);
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

	MoveTowards(targetEnergy_->GetPosition(), deltaTime);
	ConsumeStamina(deltaTime);

	const float pickupRadiusSquared = settings_->pickupRadius * settings_->pickupRadius;
	if (DistanceSquaredXZ(position_, targetEnergy_->GetPosition()) <= pickupRadiusSquared) {
		// 回収成立後は同じEnergyを保持したまま帰還状態へ遷移する
		if (targetEnergy_->BeginCarry())
		{
			targetEnergy_->SetCarriedPosition(position_ + settings_->carryOffset);
			state_ = UnitState::ReturningToRocket;
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

	// 敵への移動とスタミナ消費のみを行う
	MoveTowards(targetEnemy_->GetPosition(), deltaTime);
	ConsumeStamina(deltaTime);
}

void Unit::UpdateReturningToRocket(float deltaTime)
{
	if (!targetEnergy_ || !targetEnergy_->IsCarried())
	{
		Recall();
		return;
	}

	MoveTowards(rocket_->GetPosition(), deltaTime);
	ConsumeStamina(deltaTime);
	targetEnergy_->SetCarriedPosition(position_ + settings_->carryOffset);

	const float deliveryRadiusSquared = settings_->deliveryRadius * settings_->deliveryRadius;
	if (DistanceSquaredXZ(position_, rocket_->GetPosition()) <= deliveryRadiusSquared)
	{
		// DeliverはEnergy個体をプールへ戻し、返された獲得量だけをRocketへ加算
		rocket_->DepositEnergy(targetEnergy_->Deliver());
		targetEnergy_ = nullptr;
		state_ = UnitState::Stored;
		stamina_ = 0.0f;
	}
}

void Unit::AllocateStamina(int32_t requestedEnergy) 
{
	// EnergyChange.amountは消費時に負数なので、符号を反転してスタミナ残量にする
	const EnergyChange allocated = rocket_->AllocateEnergyToUnit((std::max)(requestedEnergy, 0));
	stamina_ = static_cast<float>(std::abs(allocated.amount));
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
	position_ = rocket_->GetPosition() + settings_->launchOffset;
	position_.y = settings_->groundY;

	// コライダーの無効化（待機状態にするため）
	collider_.SetActive(false);
	SyncModel();
}

void Unit::StartCarryingEnergy(EnergyPickup* energy)
{
	if (!energy)
	{
		ReturnToStorageAfterDefeat();
		return;
	}

	// 生成されたEnergyの予約と運搬処理
	if (energy->TryReserve() && energy->BeginCarry())
	{
		targetEnergy_ = energy;
		targetEnergy_->SetCarriedPosition(position_ + settings_->carryOffset);
		state_ = UnitState::ReturningToRocket;
		targetEnemy_ = nullptr;
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

float Unit::DistanceSquaredXZ(const Vector3& a, const Vector3& b) const
{
	const float x = a.x - b.x;
	const float z = a.z - b.z;
	return x * x + z * z;
}

void Unit::SyncModel() 
{
	modelComponent_->worldTransform_.transform_.scale = settings_->scale;
	modelComponent_->worldTransform_.transform_.translate = position_;
	if (collider_.IsActive())
	{
		collider_.SetWorldPosition(position_);
	}
	// 水色なら高速移動可能、通常色ならスタミナ切れであることを示す
	modelComponent_->materialData_->color = stamina_ > 0.0f
		? settings_->staminaColor
		: settings_->normalColor;
	modelComponent_->Update();
}

