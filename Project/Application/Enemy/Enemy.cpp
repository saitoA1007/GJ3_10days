#include "Enemy.h"
#include <Vector2.h>
#include <FPSCounter.h>
#include <Application/CollisionConfig.h>
#include <Application/Unit/Unit.h>
#include <Application/Utils/ShigeFunc.h>

#include "Application/Energy/EnergyPickup.h"
#include "Application/Energy/EnergySpawner.h"
#include "Application/Field/Field.h"
#include "Application/Rocket/Rocket.h"
#include "Application/Unit/UnitManager.h"
#include <RandomGenerator.h>
#include <numbers>

Enemy::Enemy(GameEngine::WorldTransforms::TransformData* data) : data_(data) {
	GameEngine::UserData userData;
	userData.typeID = uint32_t(CollisionTypeID::kEnemy);
	userData.object = this;
	collider_.SetCollisionAttribute(uint32_t(CollisionTypeID::kEnemy));
	collider_.SetCollisionMask(uint32_t(CollisionTypeID::kPlayer) | uint32_t(CollisionTypeID::kUnit) | uint32_t(CollisionTypeID::kRocket));
	collider_.SetUserData(userData);
	collider_.SetRadius(2.0f);

	collider_.SetOnCollisionEnterCallback([this](const GameEngine::CollisionResult& result) {
		if (isDead_) {
			return;
		}

		switch (result.userData.typeID) {
		case uint32_t(CollisionTypeID::kPlayer):
			break;

		case uint32_t(CollisionTypeID::kUnit):
		{
			Unit* hitUnit = static_cast<Unit*>(result.userData.object);
			if (!hitUnit) break;

			if (hitUnit->IsBlackhole()) break;

			// UnitがEnergyを持って運搬中に当たった場合
			if (hitUnit->IsCarryingEnergy()) {
				// 敵の勝ち
				hitUnit->DefeatAndDropEnergy();
			}
			// UnitがEnergyを持っていない場合
			else {
				if (hitUnit->GetStamina() > 0.0f) {
					hp_ = 0;
					damageTimer_ = 0.0f;

					if (hp_ <= 0) {
						EnergyPickup* droppedEnergy = this->DefeatAndDropEnergy();
						if (droppedEnergy) {
							hitUnit->StartCarryingEnergy(droppedEnergy);
						}
					}
				} else {
					// スタミナが0の場合：相打ち
					this->DefeatAndDropEnergy();

					hitUnit->ReturnToStorageAfterDefeat();
				}
			}
			break;
		}

		case uint32_t(CollisionTypeID::kRocket):
			// ロケット到達時：ダメージ通知を与えて消滅
			if (context_.rocket) {
				context_.rocket->ReceiveEnemyHit();
			}
			hp_ = 0;
			isDead_ = true;
			break;
		}
		});

	data_->transform.translate.y = 0.0f;
	data_->color = { 1.0f, 0.4f, 0.6f, 1.0f };
	collider_.SetActive(false);
}

void Enemy::SetUp(Vector2 position, Config config, EnemyType type) {
	config_ = config;
	type_ = type;
	snakeSpeed_ = 0.0f;
	snakeWidth_ = 0.0f;
	roundSpeed_ = 0.0f;

	data_->transform.scale = { config.size_, config.size_, config.size_ };
	data_->transform.translate = { position.x, 0.0f, position.y };
	data_->color = config_.normalColor_;

	distance_ = position.Length();
	direction_ = position;
	direction_.Normalize();

	isActive_ = true;
	isDead_ = false;
	wasDefeated_ = false;
	isReservedForAttack_ = false;
	isHighlighted_ = false;
	targetUnit_ = nullptr;

	hp_ = config_.hp;
	damageTimer_ = 100.f;
	snakeTimer_ = 0.0f;

	collider_.SetActive(true);
	collider_.SetRadius(collisionRadius_ * config.size_);

	collider_.SetWorldPosition(data_->transform.translate);

	timer_ = RandomGenerator::Get(0.0f, 10.0f);
}


void Enemy::Initialize() {
	isActive_ = false;
	isDead_ = true;
	wasDefeated_ = false;
	isReservedForAttack_ = false;
	isHighlighted_ = false;
	targetUnit_ = nullptr;

	data_->transform.scale = {};
	collider_.SetActive(false);
}

void Enemy::Update() {
	if (!isActive_ || isDead_) return;

	timer_ += GameEngine::FpsCounter::deltaTime;

	// 運搬中ユニットの索敵
	UpdateTarget();

	// 移動
	if (isBeingPulled_) {
		isBeingPulled_ = false; // 次フレーム用にフラグを落とす
	}
	else if (targetUnit_ && targetUnit_->IsCarryingEnergy()) {
		TrackingMovement(GameEngine::FpsCounter::deltaTime);
	}
	else {
		if (type_ == EnemyType::Round) {
			RoundMovement();
		}
		else {
			DefaultMovement();
		}
	}

	collider_.SetWorldPosition(data_->transform.translate);

	// ロケット到達等の距離判定
	if (distance_ < 0.2f) {
		if (context_.rocket) {
			context_.rocket->ReceiveEnemyHit();
		}
		isDead_ = true;
	}

	// 被弾・ハイライト等の色変化
	damageTimer_ += GameEngine::FpsCounter::deltaTime;
	if (damageTimer_ < damageTime_) {
		data_->color = config_.hitColor_;
	}
	else if (isHighlighted_) {
		data_->color = config_.highlightColor_ * 10.0f;
	}
	else {
		data_->color = config_.normalColor_;
	}
}

void Enemy::DeadUpdate() {
	data_->transform.scale = {};
	isActive_ = false;
	collider_.SetActive(false);
}

void Enemy::DefaultMovement() {
	distance_ -= config_.speed_ * GameEngine::FpsCounter::deltaTime;
	Vector2 localPos = {};

	snakeTimer_ += GameEngine::FpsCounter::deltaTime;
	localPos += { distance_, std::sin(snakeTimer_* snakeSpeed_)* snakeWidth_ };

	Vector2 position = SF::RotDir(localPos, direction_);
	data_->transform.translate.x = position.x;
	data_->transform.translate.y = 0.7f * config_.size_ + sinf(timer_) * 0.5f;
	data_->transform.translate.z = position.y;

	data_->transform.rotate.y = -(std::atan2f(direction_.y, direction_.x) + std::numbers::pi_v<float> * 0.5f);
}

void Enemy::RoundMovement() {
	distance_ -= config_.speed_ * GameEngine::FpsCounter::deltaTime;
	Vector2 localPos = {};

	roundTimer_ += GameEngine::FpsCounter::deltaTime;
	localPos += { std::cos(roundTimer_* roundSpeed_) * distance_, std::sin(roundTimer_* roundSpeed_)* distance_ };

	Vector2 position = SF::RotDir(localPos, direction_);
	data_->transform.translate.x = position.x;
	data_->transform.translate.z = position.y;
}

void Enemy::TrackingMovement(float deltaTime) {
	Vector3 targetPos = targetUnit_->GetPosition();
	Vector3 currentPos = GetPosition();
	Vector3 dir = targetPos - currentPos;
	dir.y = 0.0f;

	float dist = dir.Length();
	if (dist > 0.0001f) {
		dir.Normalize();
		float moveDist = GameEngine::Math::Min(config_.speed_ * deltaTime, dist);
		data_->transform.translate += dir * moveDist;

		// 距離をロケット中心からの距離へ再計算
		Vector2 pos2D = { data_->transform.translate.x, data_->transform.translate.z };
		distance_ = pos2D.Length();

		// 現在地からロケットへの方向を再計算し、direction_ を上書き
		if (distance_ > 0.0001f) {
			direction_ = pos2D / distance_;
		}
	}
}

void Enemy::UpdateTarget() {
	if (!context_.unitManager) return;

	// 追跡中のユニットがエネルギー運搬をやめた場合はターゲット解除
	if (targetUnit_) {
		if (!targetUnit_->IsCarryingEnergy()) {
			targetUnit_ = nullptr;
		}
	}

	// ターゲットがいない場合のみ、新たな運搬ユニットを検索
	if (!targetUnit_) {
		targetUnit_ = context_.unitManager->FindNearestCarryingUnit(GetPosition(), searchRadius_);
	}
}

bool Enemy::TryReserveForAttack() {
	if (!IsTargetable()) return false;
	isReservedForAttack_ = true;
	isHighlighted_ = false;
	return true;
}

void Enemy::CancelAttackReservation() {
	if (!isActive_) return;
	isReservedForAttack_ = false;
}

void Enemy::SetHighlighted(bool highlighted) {
	if (!isActive_) return;
	isHighlighted_ = highlighted;
}

EnergyPickup* Enemy::DefeatAndDropEnergy() {
	if (!isActive_ || isDead_) return nullptr;

	// 撃破フラグと死亡フラグを即座に立て、コライダーを無効化
	wasDefeated_ = true;
	isDead_ = true;
	isActive_ = false;
	collider_.SetActive(false);
	data_->transform.scale = {}; // 見た目も即時非表示

	// energySpawner が設定されている場合のみエナジーをドロップ
	if (context_.energySpawner) {
		Vector3 dropPos = GetPosition();
		EnergySize size = GetDropEnergySize();
		return context_.energySpawner->SpawnOnGround(size, dropPos);
	}

	return nullptr;
}

void Enemy::PullTowards(const Vector3& targetPos, float speed, float deltaTime) 
{
	if (!data_) return;
	Vector3 currentPos = data_->transform.translate;
	Vector3 dir = targetPos - currentPos;
	dir.y = 0.0f;
	if (dir.LengthSquared() > 0.0001f) {
		dir.Normalize();
		data_->transform.translate += dir * speed * deltaTime;

		// ロケットからの距離・方向を再計算して保持
		Vector2 pos2D = { data_->transform.translate.x, data_->transform.translate.z };
		distance_ = pos2D.Length();
		if (distance_ > 0.0001f) {
			direction_ = pos2D / distance_;
		}
	}
	isBeingPulled_ = true; // 吸い込みフラグを立てる
}

EnergySize Enemy::GetDropEnergySize() const {
	if (!context_.field) return EnergySize::Small;

	switch (context_.field->GetZone(GetPosition())) {
	case FieldZone::Center:
	case FieldZone::Near:
		return EnergySize::Small;
	case FieldZone::NearBuffer:
	case FieldZone::Middle:
		return EnergySize::Medium;
	default:
		return EnergySize::Large;
	}
}
