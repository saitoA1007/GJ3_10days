#include "Enemy.h"
#include <Vector2.h>
#include <FPSCounter.h>
#include <Application/CollisionConfig.h>

Enemy::Enemy(GameEngine::WorldTransforms::TransformData* data) : data_(data) {
	GameEngine::UserData userData;
	userData.typeID = uint32_t(CollisionTypeID::kEnemy);
	collider_.SetCollisionMask(uint32_t(CollisionTypeID::kPlayer) | uint32_t(CollisionTypeID::kPikumi));
	collider_.SetUserData(userData);
	collider_.SetRadius(2.0f);

	collider_.SetOnCollisionEnterCallback([this](const GameEngine::CollisionResult& result) {
		switch (result.userData.typeID) {
		case  uint32_t(CollisionTypeID::kPlayer):

			break;
		case uint32_t(CollisionTypeID::kPikumi):
			hp_--;
			if (hp_ <= 0) {
				isDead_ = true;
			}
			break;
		case uint32_t(CollisionTypeID::kTower):
			// タワーに当たった場合の処理
			hp_ = 0;
			isDead_ = true;
			break;
		}
	});

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
