#include "ResultMoveCamera.h"
#include "FPSCounter.h"
#include "EasingManager.h"
using namespace GameEngine;

ResultMoveCamera::ResultMoveCamera(GameEngine::Camera* mainCamera) {
	mainCamera_ = mainCamera;

	debugParam_.Register("WaitMaxTime", kWaitMaxTime_);
	debugParam_.Register("MoveMaxTime", kMoveMaxTime_);
	debugParam_.Register("StopMaxTime", kStopMaxTime_);
	debugParam_.Register("startPos", startPos_);
	debugParam_.Register("endPos", endPos_);
	debugParam_.Register("baseRotate", baseRotate_);

	debugParam_.Register("shakeStartPower", shakeStartPower_, 0, "Shake");
	debugParam_.Register("shakeEndPower", shakeEndPower_, 1, "Shake");
	debugParam_.Register("shakeCurve", shakeCurve_, 2, "Shake");
	debugParam_.Register("shakeFreqStart", shakeFreqStart_, 3, "Shake");
	debugParam_.Register("shakeFreqEnd", shakeFreqEnd_, 4, "Shake");
	debugParam_.Register("shakeRotRate", shakeRotRate_, 5, "Shake");
	debugParam_.Register("moveShakePower", moveShakePower_, 6, "Shake");
	debugParam_.Apply();

	camera_.Initialize({ { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f },{} },1280,720);

	camera_.transform_.translate;
}

void ResultMoveCamera::Initialize() {
	phase_ = Phase::kWait;
	timer_ = 0.0f;
	shakePhase_ = 0.0f;
	shakePower_ = 0.0f;
	basePos_ = startPos_;
	// メインカメラに映す
	mainCamera_->SetCamera(camera_);
}

void ResultMoveCamera::Update() {
	debugParam_.ApplyIfDirty();

	if (isFinished_) { return; }

	const float deltaTime = FpsCounter::gameDeltaTime;

	switch (phase_)
	{
	case ResultMoveCamera::Phase::kWait:
		UpdateWait(deltaTime);
		break;


	case ResultMoveCamera::Phase::kMove:
		UpdateMove(deltaTime);
		break;

	case ResultMoveCamera::Phase::kStop:
		timer_ += deltaTime / kStopMaxTime_;

		if (timer_ >= 1.0f) {
			timer_ = 0.0f;
			phase_ = Phase::kWait;
			basePos_ = endPos_;
			isFinished_ = true;
		}
		break;
	}

	shakePhase_ += deltaTime * shakeFreq_;
	const Vector3 shakeOffset = CalcShakeOffset(shakePower_, 0.0f);
	const Vector3 shakeRotate = CalcShakeOffset(shakePower_ * shakeRotRate_, 17.3f);
	camera_.transform_.translate = basePos_ + shakeOffset;
	camera_.transform_.rotate = baseRotate_ + shakeRotate;

	camera_.Update();
}

void ResultMoveCamera::UpdateWait(float deltaTime) {
	timer_ += deltaTime / kWaitMaxTime_;
	const float t = std::clamp(timer_, 0.0f, 1.0f);

	basePos_ = startPos_;

	const float rate = std::pow(t, shakeCurve_);
	shakePower_ = shakeStartPower_ + (shakeEndPower_ - shakeStartPower_) * rate;
	shakeFreq_ = shakeFreqStart_ + (shakeFreqEnd_ - shakeFreqStart_) * rate;

	if (timer_ >= 1.0f) {
		phase_ = Phase::kMove;
		timer_ = 0.0f;
	}
}

void ResultMoveCamera::UpdateMove(float deltaTime) {
	timer_ += deltaTime / kMoveMaxTime_;
	const float t = std::clamp(timer_, 0.0f, 1.0f);

	basePos_ = Lerp(startPos_, endPos_, t, EaseType::kEaseInCubic);
	basePos_.y = Lerp(startPos_.y, endPos_.y, t, EaseType::kEaseOutSine);

	// 発射直後が一番揺れて、上昇するにつれて落ち着く
	shakePower_ = moveShakePower_ * (1.0f - t) * (1.0f - t);
	shakeFreq_ = shakeFreqEnd_;

	if (timer_ >= 1.0f) {
		timer_ = 0.0f;
		phase_ = Phase::kStop;
		shakePower_ = 0.0f;
	}
}

Vector3 ResultMoveCamera::CalcShakeOffset(float power, float seed) const {
	// sin波を足し合わせて揺れを作成
	if (power <= 0.0f) {
		return { 0.0f, 0.0f, 0.0f };
	}

	const float t = shakePhase_ + seed;

	Vector3 offset{};
	offset.x = std::sin(t * 1.00f) * 0.6f + std::sin(t * 2.31f + 1.3f) * 0.4f;
	offset.y = std::sin(t * 1.17f + 1.7f) * 0.6f + std::sin(t * 2.73f + 4.1f) * 0.4f;
	offset.z = std::sin(t * 0.83f + 3.1f) * 0.6f + std::sin(t * 3.11f + 2.2f) * 0.4f;

	return offset * power;
}
