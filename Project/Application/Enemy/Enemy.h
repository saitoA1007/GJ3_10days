#pragma once
#include <IGameObject.h>
#include <WorldTransforms.h>
#include <Vector2.h>
#include <Collider.h>

//EnemyManagerで更新を呼び出したいので、GameObjectManagerに登録しない。
class Enemy : public GameEngine::IGameObject {
public:

	struct Config {
		float speed_ = 2.0f;
		int hp = 1;
		Vector4 normalColor_ = { 1.0f, 0.4f, 0.6f, 1.0f };
		Vector4 hitColor_ = { 0.9f, 0.0f, 0.0f, 1.0f };
	};

public:

	Enemy(GameEngine::WorldTransforms::TransformData* data);
	~Enemy() {};

	void SetUp(Vector2 position, Config config = {});

	void Initialize() override;
	void Update() override;
	void DeadUpdate();

	Vector3 GetPosition() const { return data_->transform.translate; }

	void SetDamageTime(float time) { damageTime_ = time; }

private:

	void Destroy() override {
		isActive_ = false;
		isDead_ = true;
	}

	GameEngine::WorldTransforms::TransformData* data_;
	GameEngine::SphereCollider collider_;

	int hp_ = 1;
	float damageTimer_ = 0.0f;

	float damageTime_ = 0.02f;
	Config config_;
};
