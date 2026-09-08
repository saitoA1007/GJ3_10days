#pragma once
#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DestructibleObject.h"
#include "Collider.h"

class MoonObject : public GameEngine::IGameObject {
public:
	MoonObject(GameEngine::Model* model, GameEngine::Model* fractureModel, uint32_t moonGH, uint32_t norGH);

	void Initialize() override;
	void Update() override;
	void Draw() override;

public:

	// 月を破壊する
	void Break() {
		pPos_.x = 65.0f;
		isFractureActive_ = true;
	}

	void Reset();

public:

	// 破壊の有効フラグ
	bool isFractureActive_ = false;

private:
	// パラメータ機能
	GameEngine::DebugParameter debugParam_{ "MoonObject" };

	// 通常描画用
	GameEngine::ModelComponent defaultModel_;

	// 破壊用
	GameEngine::DestructibleObject destructObject_;

	// obbの当たり判定
	GameEngine::OBBCollider collider_;

	GameEngine::SphereCollider pCollider_;
	Vector3 pPos_ = { 0.0f,0.0f,0.0f };

	// 当たり判定の大きさ
	Vector3 colliderSize_ = { 2.0f,2.0f,2.0f };
private:

	// 当たり判定
	void OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result);
};