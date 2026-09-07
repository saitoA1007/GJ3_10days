#pragma once
#include "DebugParameter.h"
#include "IGameObject.h"
#include "Camera.h"

class ResultMoveCamera : public GameEngine::IGameObject {
public:

	enum class Phase {
		kWait, // 発射までの待機
		kMove, // 移動
		kStop  // 止まる
	};

public:
	ResultMoveCamera();

	void Initialize() override;
	void Update() override;

public:

	void Start() {
		isFinished_ = false;
		phase_ = Phase::kWait;
		timer_ = 0.0f;
		// メインカメラに映す
		//mainCamera_->SetCamera(camera_);
		renderQueue_->SetCamera(&camera_);
	}

	// 現在のフェーズを取得
	Phase GetCurrentPhase() const { return phase_; }

	bool IsFinished() const {
		return isFinished_;
	}

private:

	float kWaitMaxTime_ = 2.0f;
	float kMoveMaxTime_ = 2.0f;
	float kStopMaxTime_ = 2.0f;

	Vector3 startPos_ = { 0.0f,0.0f,0.0f };
	Vector3 endPos_ = { 80.0f,50.0f,0.0f };
	Vector3 baseRotate_ = { 0.0f,0.0f,0.0f };

	// カメラシェイク用
	float shakeStartPower_ = 0.02f; // 待機開始時の揺れ幅
	float shakeEndPower_ = 0.50f;   // 発射直前の揺れ幅
	float shakeCurve_ = 3.0f;       // 揺れの立ち上がり方
	float shakeFreqStart_ = 12.0f;  // 待機開始時の揺れの速さ
	float shakeFreqEnd_ = 40.0f;    // 発射直前の揺れの速さ
	float shakeRotRate_ = 0.02f;    // 位置の揺れに対する回転の揺れの割合
	float moveShakePower_ = 0.35f;  // 移動開始時の揺れ幅

private:
	GameEngine::DebugParameter debugParam_{ "ResultMoveCamera" };

	GameEngine::Camera camera_;

	Phase phase_ = Phase::kWait;

	float timer_ = 0.0f;

	// 揺れを乗せる前の位置
	Vector3 basePos_ = { 0.0f,0.0f,0.0f };
	// このフレームの揺れの強さ
	float shakePower_ = 0.0f;
	// 揺れの位相
	float shakePhase_ = 0.0f;
	// 揺れの速さ
	float shakeFreq_ = 0.0f;

	bool isFinished_ = false;

private:

	void UpdateWait(float deltaTime);

	void UpdateMove(float deltaTime);

	// シェイク
	Vector3 CalcShakeOffset(float power, float seed) const;
};