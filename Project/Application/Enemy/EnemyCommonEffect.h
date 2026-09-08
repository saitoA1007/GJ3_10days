#pragma once
#include <ParticleBehavior.h>
#include <ModelManager.h>

class EnemyCommonEffect {
public:

	EnemyCommonEffect(GameEngine::TextureManager* textureManager, GameEngine::ModelManager* modelManager);

	void Initialize();
	void Update();
	void Draw();

	uint32_t SecureEffectID();
	void ReleaseEffectID(uint32_t effectID);

	void SetPosition(uint32_t effectID, const Matrix4x4& mat);

private:

	struct Effect {
		std::unique_ptr<GameEngine::ParticleBehavior> backCircle;
	};

	std::vector<uint32_t> freeList_;
	uint32_t freeListIndex_ = 0;
	std::vector<Effect> effects_;
};
