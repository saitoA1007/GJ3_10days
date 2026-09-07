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

	void Start(Vector3 pos, Vector3 dir, bool isActive);

public:

	// 基準位置
	GameEngine::WorldTransform baseWorld_;

private:

	float kInMaxTime_ = 2.0f;
	float kEndMaxTime_ = 2.0f;

	float kInStartPos_ = 50.0f;
	float kInEndScale_ = 1.0f;

private:
	// パラメータ機能
	GameEngine::DebugParameter debugParame_{"RocketEffect"};

	// 炎
	GameEngine::ParticleBehavior* fireEffect_ = nullptr;

	// ウェーブ
	//GameEngine::ModelComponent waveModel_;
	Transform waveUvtransform_ = { {1.0f,1.0f,1.0f},{},{} };

	float timer_ = 0.0f;

private:

	// 登録する
	void Register();
};