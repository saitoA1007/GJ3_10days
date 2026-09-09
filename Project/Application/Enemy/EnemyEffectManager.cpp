#include "EnemyEffectManager.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "GameObjectManager.h"
using namespace GameEngine;

EnemyEffectManager::EnemyEffectManager(GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager, GameEngine::GameObjectManager* objectManager) {

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
	beamEffects_.reserve(16);
	for (uint32_t i = 0; i < 16; ++i) {
		EnemySpawnEffect* beam = objectManager->AddObject<EnemySpawnEffect>(beamModel, planeModel, waveModel, textureManager);
		beam->SetActive(false);
		beamEffects_.push_back(beam);
	}

	deadEffects_.reserve(16);
	for (uint32_t i = 0; i < 16; ++i) {
		auto* deadEffect = objectManager->AddObject<ParticleBehavior>("EnemyDead", 64, textureManager, planeModel);
		deadEffect->SetActive(false);
		deadEffects_.push_back(deadEffect);
	}

	deadEffectIndex_ = 0;
}

void EnemyEffectManager::Initialize() {

}

void EnemyEffectManager::Update() {
}

void EnemyEffectManager::Draw() {

}

void EnemyEffectManager::StartSpawnEffect(Vector3 pos) {

	for (uint32_t i = 0; i < static_cast<uint32_t>(beamEffects_.size()); ++i) {
		// スキップ
		if (beamEffects_[i]->IsActive()) { continue; }
		beamEffects_[i]->Reset();
		beamEffects_[i]->Start(pos);
		beamEffects_[i]->SetActive(true);
		break;
	}
}

void EnemyEffectManager::StartDeadEffect(Vector3 pos) {
	deadEffects_[deadEffectIndex_]->Emit(pos);
	deadEffects_[deadEffectIndex_]->SetActive(true);
	deadEffectIndex_ = (deadEffectIndex_ + 1) % deadEffects_.size();
}
