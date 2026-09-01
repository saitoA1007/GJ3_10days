#include "Enemy.h"
#include <Vector2.h>
#include <FPSCounter.h>
#include <Application/CollisionConfig.h>

Enemy::Enemy(GameEngine::WorldTransforms::TransformData* data) : data_(data) {
	collider_.SetCollisionMask(uint32_t(CollisionTypeID::kPlayer));
	collider_.SetRadius(2.0f);

	data_->transform.translate.y = 0.0f;
	data_->color = { 1.0f, 0.4f, 0.6f, 1.0f };

	collider_.SetActive(false);
}

void Enemy::SetUp(Vector2 position) {
	data_->transform.scale = { 1.0f, 1.0f, 1.0f };

	data_->transform.translate.x = position.x;
	data_->transform.translate.z = position.y;

	isActive_ = true;
	isDead_ = false;

	hp_ = 1;

	collider_.SetActive(true);
}

void Enemy::Initialize() {
	isActive_ = false;
	isDead_ = true;
}

void Enemy::Update() {
	Vector3 position = data_->transform.translate;
	Vector2 toMid = { -position.x, -position.z };
	toMid.Normalize();

	toMid = toMid * speed_ * GameEngine::FpsCounter::deltaTime;

	data_->transform.translate.x += toMid.x;
	data_->transform.translate.z += toMid.y;

	collider_.SetWorldPosition(data_->transform.translate);

	//暫定的な死亡判定
	if (position.Length() < 0.2f) {
		isDead_ = true;
	}
}

void Enemy::DeadUpdate() {
	data_->transform.scale = {};
	isActive_ = false;
	collider_.SetActive(false);
}
