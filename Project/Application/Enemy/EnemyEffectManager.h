#pragma once
#include "IGameObject.h"
#include"Application/Effect/EnemySpawnEffect.h"
#include <ParticleBehavior.h>

namespace GameEngine {
	class ModelManager;
	class TextureManager;
	class GameObjectManager;
}

class EnemyEffectManager : public GameEngine::IGameObject {
public:
	EnemyEffectManager(GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager, GameEngine::GameObjectManager* objectManager);

	void Initialize() override;
	void Update() override;
	void Draw() override;

public:

	// 敵の出現演出
	void StartSpawnEffect(Vector3 pos);

	void StartDeadEffect(Vector3 pos);

private:

	// 敵が出撃するビームの演出
	std::vector<EnemySpawnEffect*> beamEffects_;

	std::vector<GameEngine::ParticleBehavior*> deadEffects_;
	int deadEffectIndex_ = 0;
};