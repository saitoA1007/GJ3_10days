#pragma once
#include "DebugParameter.h"
#include "IGameObject.h"
#include "ModelComponent.h"

namespace GameEngine {
	class TextureManager;
	class ModelManager;
	class GameObjectManager;
	class ParticleBehavior;
}

class ExplosionEffect : public GameEngine::IGameObject {
public:
	ExplosionEffect(GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager, GameEngine::GameObjectManager* objectManager);

	void Initialize() override;
	void Update() override;
	void Draw() override;

public:

	void Start(Vector3 pos);

private:

	// フラッシュ
	float startFlashScale_ = 1.0f;
	float endFlashScale_ = 1.0f;

	// リング
	float startRingScale_ = 1.0f;
	float endRingScale_ = 1.0f;

private:
	// パラメータ機能
	GameEngine::DebugParameter debugParame_{"ExplosionEffect"};

	// 爆発
	GameEngine::ParticleBehavior* explosionParticle_;
	GameEngine::ParticleBehavior* explosionLineParticle_;

	// リング
	GameEngine::ModelComponent ringModel_;

	// フラッシュ
	GameEngine::ModelComponent flashModel_;
	
	float timer_ = 0.0f;

private:

	// 登録する
	void Register();
};