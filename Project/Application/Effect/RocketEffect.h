#pragma once
#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"
#include "ParticleBehavior.h"

namespace GameEngine {
	class TextureManager;
	class ModelManager;
	class GameObjectManager;
}

class RocketEffect : public GameEngine::IGameObject {
public:
	RocketEffect(GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager, GameEngine::GameObjectManager* objectManager);

	void Initialize() override;
	void Update() override;
	void Draw() override;

public:

	void Start();

public:

	//ロケット
	GameEngine::ModelComponent rocketModel_;

private:

	float kMoveMaxTime_ = 2.0f;

	Vector3 startPos_ = {0.0f,0.0f,0.0f};
	Vector3 endPos_ = {100.0f,50.0f,0.0f};
	
private:
	// パラメータ機能
	GameEngine::DebugParameter debugParame_{"RocketEffect"};

	// 炎
	GameEngine::ParticleBehavior* fireEffect_ = nullptr;

	float timer_ = 0.0f;

	bool isActiveAnimation_ = false;

	// 補正
	float kRotateOffsetZ_ = 0.0f;

private:

	// 登録する
	void Register();

	void UpdateMove();

	Vector3 CalcPos(float t) const;
};