#include "EnemyRenderer.h"

EnemyRenderer::EnemyRenderer(GameEngine::RenderQueue* renderQueue) {
	renderQueue_ = renderQueue;
}

void EnemyRenderer::SetTransforms(GameEngine::WorldTransforms* transforms) {
	transforms_ = transforms;
}

void EnemyRenderer::SetModel(const GameEngine::Model* model) {
	model_ = model;
}

void EnemyRenderer::Draw() {
	renderQueue_->SubmitInstancing(model_, transforms_->GetNumInstance(), *transforms_);
}
