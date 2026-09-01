#pragma once
#include <IGameObject.h>
#include <WorldTransforms.h>
#include <Vector2.h>
#include <Collider.h>

//EnemyManagerで更新を呼び出したいので、GameObjectManagerに登録しない。
class Enemy : public GameEngine::IGameObject {
public:

	Enemy(GameEngine::WorldTransforms::TransformData* data);
	~Enemy() {};

	void SetUp(Vector2 position);

	void Initialize() override;
	void Update() override;
	void DeadUpdate();

private:

	void Destroy() override {
		isActive_ = false;
		isDead_ = true;
	}

	GameEngine::WorldTransforms::TransformData* data_;

	GameEngine::SphereCollider collider_;

	float speed_ = 2.0f;

	int hp_ = 1;
};
