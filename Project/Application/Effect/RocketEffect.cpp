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
	debugParame_.Register("Energy", energy_);
	debugParame_.Register("Gravity", kGravity_);
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

void RocketEffect::Start(float energy) {
	if (isRocketStarted_) { return; }
	auto& tr = rocketModel_.worldTransform_.transform_;
	tr.translate = startPos_;
	tr.rotate.z = 0.0f;

	energy_ = energy;

	timer_ = 0.0f;
	velocity_ = { 0.0f,0.0f,0.0f };
	phase_ = Phase::Boost;
	isRocketStarted_ = true;
	isActiveAnimation_ = true;
	fireEffect_->SetActive(true);
}

void RocketEffect::UpdateMove() {

	switch (phase_) {
	case Phase::Boost: 
		UpdateBoost(); 
		break;

	case Phase::Overrun:
		UpdateOverrun();
		break;

	case Phase::Fall: 
		UpdateFall();
		break;
	}
}

void RocketEffect::UpdateBoost() {
	const float limit = std::clamp(energy_, 0.0f, 1.0f);

	timer_ += FpsCounter::gameDeltaTime / kMoveMaxTime_;
	const float t = std::clamp(timer_, 0.0f, limit);

	auto& tr = rocketModel_.worldTransform_.transform_;
	tr.translate = CalcPos(t);
	tr.rotate.z = Lerp(0.0f, 1.57f, t, EaseType::kEaseInOutSine);

	// エフェクト
	Vector3 dir = CalcVelocity(t);
	dir.z = 0.0f;
	dir.Normalize();
	fireEffect_->SetDirection(dir * -1.0f);
	fireEffect_->SetEmitterPos(tr.translate + (dir * -2.0f));

	if (timer_ < limit) { return; }

	if (limit >= 1.0f) {
		// 到達成功
		tr.translate = endPos_;
		fireEffect_->SetActive(false);

		if (energy_ > 1.0f) {
			phase_ = Phase::Overrun;
		} else {
			phase_ = Phase::Idle;
		}
		isActiveAnimation_ = false;
		timer_ = 0.0f;
	} else {
		// エネルギー切れ
		velocity_ = CalcVelocity(limit);
		fireEffect_->SetActive(false);
		phase_ = Phase::Fall;
	}
}

void RocketEffect::UpdateOverrun() {
	overrunTimer_ += FpsCounter::gameDeltaTime / overrunTime_;
	const float u = std::clamp(overrunTimer_, 0.0f, 1.0f);

	auto& tr = rocketModel_.worldTransform_.transform_;
	tr.translate = endPos_;
	tr.translate.x += Lerp(0.0f, kOverrunDistanceX_, u, EaseType::kEaseOutQuad);
	tr.rotate.z = 1.57f;   // 横向きのまま

	// エフェクト
	const Vector3 dir = { 1.0f,0.0f,0.0f };
	fireEffect_->SetDirection(dir * -1.0f);
	fireEffect_->SetEmitterPos(tr.translate + (dir * -2.0f));

	if (u >= 1.0f) {
		tr.translate.x = endPos_.x + kOverrunDistanceX_;
		fireEffect_->SetActive(false);
		phase_ = Phase::Idle;
		isActiveAnimation_ = false;
		timer_ = 0.0f;
	}
}

void RocketEffect::UpdateFall() {
	const float dt = FpsCounter::gameDeltaTime;
	auto& tr = rocketModel_.worldTransform_.transform_;

	velocity_.y += kGravity_ * dt;
	tr.translate = tr.translate + velocity_ * dt;

	// 機首を速度方向へ
	if (velocity_.x * velocity_.x + velocity_.y * velocity_.y > 1e-6f) {
		tr.rotate.z = 1.5707963f - std::atan2(velocity_.y, velocity_.x);
	}

	// 着地
	if (tr.translate.y <= kGroundY_) {
		tr.translate.y = kGroundY_;
		phase_ = Phase::Idle;
		isActiveAnimation_ = false;
		timer_ = 0.0f;
	}
}

Vector3 RocketEffect::CalcPos(float t) const {
	Vector3 p = Lerp(startPos_, endPos_, t, EaseType::kEaseInCubic);
	p.y = Lerp(startPos_.y, endPos_.y, t, EaseType::kEaseOutSine);
	return p;
}

Vector3 RocketEffect::CalcVelocity(float t) const {
	const float h = 0.01f;
	const float t0 = (std::max)(t - h, 0.0f);
	const float t1 = (std::min)(t + h, 1.0f);
	return (CalcPos(t1) - CalcPos(t0)) / ((t1 - t0) * kMoveMaxTime_);
}
