#pragma once
#include "IGameObject.h"
#include"Application/Effect/IncorporateEffect.h"

namespace GameEngine {
	class ModelManager;
	class TextureManager;
	class GameObjectManager;
}

class UnitEffectManager : public GameEngine::IGameObject {
public:
	UnitEffectManager(GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager, GameEngine::GameObjectManager* objectManager);

	void Initialize() override;
	void Update() override;
	void Draw() override;

public:

	// ブラックホールの出現
	void StartBlackHole(Vector3 pos,float radius);

private:

	// ブラックホールの演出
	std::vector<IncorporateEffect*> incorporateEffects_;
};
