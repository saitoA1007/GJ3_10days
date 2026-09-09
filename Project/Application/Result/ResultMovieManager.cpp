#include "ResultMovieManager.h"
#include "Application/GameCamera/ResultMoveCamera.h"
#include "Application/Effect/Moonobject.h"
#include "Application/Effect/RocketEffect.h"
#include "Application/Effect/ExplosionEffect.h"
using namespace GameEngine;

ResultMovieManager::ResultMovieManager(ResultMoveCamera* camera, MoonObject* moonObject, RocketEffect* rocketEffect, ExplosionEffect* explosionEffect)
	: letterboxUI_("LetterboxUI") {
	resultMoveCamera_ = camera;
	moonObject_ = moonObject;
	rocketEffect_ = rocketEffect;
	explosionEffect_ = explosionEffect;

	// 停止
	explosionEffect_->SetActive(false);
	resultMoveCamera_->SetActive(false);
	moonObject_->SetActive(true);
	rocketEffect_->SetActive(false);
}

void ResultMovieManager::Initialize() {
	letterboxUI_.Initialize();
	rocketEffect_->Reset();

	explosionEffect_->SetActive(false);
	resultMoveCamera_->SetActive(false);
	rocketEffect_->SetActive(false);
}

void ResultMovieManager::Update() {

	ResultMoveCamera::Phase phase = resultMoveCamera_->GetCurrentPhase();

	switch (phase)
	{
	case ResultMoveCamera::Phase::kWait:

		rocketEffect_->rocketModel_.worldTransform_.transform_.translate = { 0.0f,5.5f,0.0f };
		break;

	case ResultMoveCamera::Phase::kMove:

		// ロケット演出
		rocketEffect_->Start(clearRate_);
		break;

	case ResultMoveCamera::Phase::kStop:

		if (rocketEffect_->GetEnergy() > 1.0f) {
			if (isExplo_) { return; }
			isExplo_ = true;
			moonObject_->Break();
			explosionEffect_->Start({80.0f,50.0f,0.0f});
			// バーをおろす
			letterboxUI_.SetBarActive(false);
		}
		break;
	}

	letterboxUI_.Update();
}

void ResultMovieManager::Draw() {
	letterboxUI_.Draw();
}

void ResultMovieManager::Start(float clearRate) {
	clearRate_ = clearRate;

	// 有効
	resultMoveCamera_->SetActive(true);
	//moonObject_->SetActive(true);
	rocketEffect_->SetActive(true);
	explosionEffect_->SetActive(true);

	// カメラ演出を開始
	resultMoveCamera_->Start();
	rocketEffect_->Reset();

	// バーを出現
	letterboxUI_.SetBarActive(true);

	// リセット
	moonObject_->Reset();

	isExplo_ = false;
}

bool ResultMovieManager::IsFin() const {
	return resultMoveCamera_->IsFinished();
}