#include"UnitEffectManager.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "GameObjectManager.h"
using namespace GameEngine;

UnitEffectManager::UnitEffectManager(GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager, GameEngine::GameObjectManager* objectManager) {

	// 
	auto* beamModel = modelManager->GetNameByModel("beam.gltf");
	beamModel->SetDefaultIsEnableLight(false);
	// 
	auto* waveModel = modelManager->GetNameByModel("RushPower.obj");
	waveModel->SetDefaultIsEnableLight(false);
	// 
	auto* planeModel = modelManager->GetNameByModel("plane.obj");
	planeModel->SetDefaultIsEnableLight(false);

	// 敵登場演出のメモリを確保
	incorporateEffects_.reserve(16);
	for (uint32_t i = 0; i < 16; ++i) {
		IncorporateEffect* incorporate = objectManager->AddObject<IncorporateEffect>(modelManager, textureManager, objectManager);
		incorporate->SetActive(false);
		incorporateEffects_.push_back(incorporate);
	}
}

void UnitEffectManager::Initialize() {

}

void UnitEffectManager::Update() {

}

void UnitEffectManager::Draw() {

}

void UnitEffectManager::StartBlackHole(Vector3 pos, float radius) {

	for (uint32_t i = 0; i < static_cast<uint32_t>(incorporateEffects_.size()); ++i) {
		// スキップ
		if (incorporateEffects_[i]->IsPlay()) { continue; }
		incorporateEffects_[i]->Start(pos,radius);
		incorporateEffects_[i]->SetActive(true);
		break;
	}
}
