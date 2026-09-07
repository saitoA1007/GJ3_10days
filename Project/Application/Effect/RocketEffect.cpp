#include "RocketEffect.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "GameObjectManager.h"
#include "ParticleBehavior.h"
#include "FPSCounter.h"
#include "EasingManager.h"
using namespace GameEngine;
using namespace GameEngine;

RocketEffect::RocketEffect(GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager, GameEngine::GameObjectManager* objectManager)
	: rocketModel_(modelManager->GetNameByModel("Rocket.gltf")) {

	debugParame_.Register("MoveMaxTime", kMoveMaxTime_);
	debugParame_.Register("RotateOffsetZ", kRotateOffsetZ_);
	debugParame_.Apply();

	// エフェクト用モデル
	auto* planeModel = modelManager->GetNameByModel("plane.obj");
	planeModel->SetDefaultIsEnableLight(false);
	fireEffect_ = objectManager->AddObject<ParticleBehavior>("RocketFireEffect", 128, textureManager, planeModel);
	fireEffect_->SetActive(false);

	rocketModel_.worldTransform_.transform_.scale = { 3.0f,3.0f,3.0f };
	rocketModel_.worldTransform_.transform_.rotate.y = 3.14f;
}

void RocketEffect::Initialize() {

}

void RocketEffect::Update() {
	debugParame_.ApplyIfDirty();

	UpdateMove();

	rocketModel_.Update();
}

void RocketEffect::Draw() {
	// ロケット描画
	rocketModel_.DrawRaytracing(renderQueue_);
}

void RocketEffect::Start() {
	fireEffect_->SetActive(true);
	isActiveAnimation_ = true;
}

void RocketEffect::UpdateMove() {
	if (!isActiveAnimation_) { return; }
	timer_ += FpsCounter::gameDeltaTime / kMoveMaxTime_;
	const float t = std::clamp(timer_, 0.0f, 1.0f);

	rocketModel_.worldTransform_.transform_.translate = CalcPos(t);
	rocketModel_.worldTransform_.transform_.rotate.z = Lerp(0.0f, 1.57f, t, EaseType::kEaseInOutSine);

	float h = 0.01f;
	Vector3 tangent = CalcPos(std::min(t + h, 1.0f)) - CalcPos((std::max)(t - h, 0.0f));
	Vector3 dir = tangent;
	dir.z = 0.0f;
	dir.Normalize();

	// エフェクト
	fireEffect_->SetDirection(dir * -1.0f);
	fireEffect_->SetEmitterPos(rocketModel_.worldTransform_.transform_.translate + (dir * -2.0f));

	if (timer_ >= 1.0f) {
		isActiveAnimation_ = false;
		timer_ = 0.0f;
		rocketModel_.worldTransform_.transform_.translate = endPos_;
		fireEffect_->SetActive(false);
	}
}

Vector3 RocketEffect::CalcPos(float t) const {
	Vector3 p = Lerp(startPos_, endPos_, t, EaseType::kEaseInCubic);
	p.y = Lerp(startPos_.y, endPos_.y, t, EaseType::kEaseOutSine);
	return p;
}