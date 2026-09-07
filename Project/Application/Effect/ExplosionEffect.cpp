#include "ExplosionEffect.h"
#include "FPSCounter.h"
#include "EasingManager.h"
#include "MyMath.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "GameObjectManager.h"
#include "ParticleBehavior.h"
using namespace GameEngine;

ExplosionEffect::ExplosionEffect(GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager, GameEngine::GameObjectManager* objectManager) 
	: ringModel_(modelManager->GetNameByModel("plane.obj")) , flashModel_(modelManager->GetNameByModel("plane.obj")) {

	auto* planeModel = modelManager->GetNameByModel("plane.obj");
	planeModel->SetDefaultIsEnableLight(false);
	explosionParticle_ = objectManager->AddObject<ParticleBehavior>("ExplosionParticle", 128, textureManager, planeModel);
	explosionLineParticle_ = objectManager->AddObject<ParticleBehavior>("ExplosionLineParticle", 128, textureManager, planeModel);

	// 無効化
	explosionParticle_->SetActive(false);
	explosionLineParticle_->SetActive(false);

	// 登録
	Register();
}

void ExplosionEffect::Initialize() {

}

void ExplosionEffect::Update() {
	debugParame_.ApplyIfDirty();

	// フラッシュ
	float scale = Lerp(startFlashScale_, endFlashScale_, timer_, EaseType::kEaseInOutQuad);
	flashModel_.worldTransform_.transform_.scale = { scale,scale ,scale };

	// リング
	scale = Lerp(startRingScale_, endRingScale_, timer_, EaseType::kEaseInOutQuad);
	ringModel_.worldTransform_.transform_.scale = { scale,scale ,scale };


	// 更新処理
	Matrix4x4 cameraMatrix = renderQueue_->GetMainCamera().GetWorldMatrix();
	if (renderQueue_->GetUseDebugCamera()) {
		cameraMatrix = renderQueue_->GetDebugCameraWorldMatrix();
	}
	// リング
	ringModel_.worldTransform_.UpdateWorldMatrix(
		Math::MakeYAxisBillboardMatrix(ringModel_.worldTransform_.transform_.scale, ringModel_.worldTransform_.transform_.translate, cameraMatrix));
	// フラッシュ
	flashModel_.worldTransform_.UpdateWorldMatrix(
		Math::MakeYAxisBillboardMatrix(flashModel_.worldTransform_.transform_.scale, flashModel_.worldTransform_.transform_.translate, cameraMatrix));
}

void ExplosionEffect::Draw() {
	flashModel_.Draw(renderQueue_, Draw3dType::DefaultAdd, "WBOITAccumulatePass");
	ringModel_.Draw(renderQueue_, Draw3dType::DefaultAdd, "WBOITAccumulatePass");
}

void ExplosionEffect::Register() {
	std::string subGroup = "Ring";
	debugParame_.Register("Color", ringModel_.materialData_->color, 0, subGroup);
	subGroup = "flash";
	debugParame_.Register("Color", flashModel_.materialData_->color, 0, subGroup);
	debugParame_.Apply();
}

void ExplosionEffect::Start(Vector3 pos) {
	// 有効
	explosionParticle_->SetActive(true);
	explosionLineParticle_->SetActive(true);
	// 位置を設定
	explosionParticle_->SetEmitterPos(pos);
	explosionLineParticle_->SetEmitterPos(pos);

	ringModel_.worldTransform_.transform_.translate = pos;
	flashModel_.worldTransform_.transform_.translate = pos;
}