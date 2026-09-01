#pragma once
#include <WorldTransforms.h>
#include <RenderQueue.h>

class EnemyRenderer {
public:

	EnemyRenderer(GameEngine::RenderQueue* renderQueue);

	void SetModel(const GameEngine::Model* model);
	void SetTransforms(GameEngine::WorldTransforms* transforms);

	void Draw();

private:

	GameEngine::RenderQueue* renderQueue_ = nullptr;
	GameEngine::WorldTransforms* transforms_ = nullptr;

	const GameEngine::Model* model_ = nullptr;

};
