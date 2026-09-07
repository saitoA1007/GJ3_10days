#include "RocketEffect.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "GameObjectManager.h"
#include "ParticleBehavior.h"
using namespace GameEngine;

RocketEffect::RocketEffect(GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager, GameEngine::GameObjectManager* objectManager) {

	// エフェクト用モデル
	auto* planeModel = modelManager->GetNameByModel("plane.obj");
	planeModel->SetDefaultIsEnableLight(false);
	fireEffect_ = objectManager->AddObject<ParticleBehavior>("RocketFireEffect", 32, textureManager, planeModel);
	fireEffect_->SetActive(false);
}

void RocketEffect::Initialize() {

}

void RocketEffect::Update() {

}

void RocketEffect::Draw() {

}

void RocketEffect::Start(Vector3 pos, Vector3 dir, bool isActive) {
	fireEffect_->SetActive(isActive);
	fireEffect_->SetDirection(dir);
	fireEffect_->SetEmitterPos(pos);
}