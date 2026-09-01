#pragma once
#include <IGameObject.h>
#include <WorldTransforms.h>
#include <Vector2.h>
#include <Collider.h>

enum class EnemyType {
	Straight_S,	//対象にまっすぐ近づく 小さい
	Straight_M,	//対象にまっすぐ近づく 中くらい
	Straight_L,	//対象にまっすぐ近づく 大きい

	Round,		//対象を中心に回る
	Snake,		//蛇行して対象に近づく

	Count
};

//EnemyManagerで更新を呼び出したいので、GameObjectManagerに登録しない。
class Enemy : public GameEngine::IGameObject {
public:

	struct Config {
		float speed_ = 2.0f;
		int hp = 1;
		float size_ = 1.0f;
		Vector4 normalColor_ = { 1.0f, 0.4f, 0.6f, 1.0f };
		Vector4 hitColor_ = { 0.9f, 0.0f, 0.0f, 1.0f };
	};

public:

	Enemy(GameEngine::WorldTransforms::TransformData* data);
	~Enemy() {};

	static void SetCollisionRadius(float radius) { collisionRadius_ = radius; }

	void SetUp(Vector2 position, Config config, EnemyType type);

	void Initialize() override;
	void Update() override;
	void DeadUpdate();

	//SetUp後に呼び出す。
	void SetSnake(float width, float speed) { snakeWidth_ = width; snakeSpeed_ = speed; }
	void SetRound(float speed) { roundSpeed_ = speed; }

	void SetDamageTime(float time) { damageTime_ = time; }

private:

	void DefaultMovement();
	void RoundMovement();

	void Destroy() override {
		isActive_ = false;
		isDead_ = true;
	}

	static inline float collisionRadius_ = 1.f;

	GameEngine::WorldTransforms::TransformData* data_;
	GameEngine::SphereCollider collider_;

	//対象との距離
	float distance_ = 0.0f;
	Vector2 direction_ = { 0.0f, 0.0f };

	int hp_ = 1;
	float damageTimer_ = 0.0f;
	float snakeTimer_ = 0.0f;
	float roundTimer_ = 0.0f;

	//=== 設定項目 ====================================================
	float damageTime_ = 0.02f;
	Config config_;
	EnemyType type_;

	float snakeWidth_ = 0.0f;
	float snakeSpeed_ = 0.0f;
	float roundSpeed_ = 0.0f;
};
