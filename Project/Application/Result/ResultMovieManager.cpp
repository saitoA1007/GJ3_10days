#include "ResultMovieManager.h"
#include "Application/GameCamera/ResultMoveCamera.h"
#include "Application/Effect/Moonobject.h"
#include "Application/Effect/RocketEffect.h"
#include "Application/Effect/ExplosionEffect.h"
#include "AudioManager.h"
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

	subFireSH_ = AudioManager::GetInstance().GetHandleByName("roketSubFire.mp3");
	mainFireSH_ = AudioManager::GetInstance().GetHandleByName("roketMainFire.mp3");
	moonBreakSH_ = AudioManager::GetInstance().GetHandleByName("moonBreak.mp3");
}

void ResultMovieManager::Initialize() {
	letterboxUI_.Initialize();
	letterboxUI_.Update();
	rocketEffect_->Reset();

	explosionEffect_->SetActive(false);
	resultMoveCamera_->SetActive(false);
	rocketEffect_->SetActive(false);
	isPlay_ = false;
}

void ResultMovieManager::Update() {

	ResultMoveCamera::Phase phase = resultMoveCamera_->GetCurrentPhase();

	switch (phase)
	{
	case ResultMoveCamera::Phase::kWait:

		rocketEffect_->rocketModel_.worldTransform_.transform_.translate = { 0.0f,5.5f,0.0f };

		if (!isPlay_) {
			return;
		}

		// バーを出現
		letterboxUI_.SetBarActive(true);

		if (!isSubFire_) {
			AudioManager::GetInstance().Play(subFireSH_, 0.5f, true);
			isSubFire_ = true;
		}
		break;

	case ResultMoveCamera::Phase::kMove:
		if (!isMainFire_) {
			AudioManager::GetInstance().Stop(subFireSH_);
			AudioManager::GetInstance().Play(mainFireSH_, 0.5f, true);
			isMainFire_ = true;
		}

		// ロケット演出
		rocketEffect_->Start(clearRate_);

		if (rocketEffect_->GetPhase() == RocketEffect::Phase::Overrun && !isMainFireStop_) {
			isMainFireStop_ = true;
			AudioManager::GetInstance().Stop(mainFireSH_);
		}
		break;

	case ResultMoveCamera::Phase::kStop:

		if (!isMainFireStop_) {
			AudioManager::GetInstance().Stop(mainFireSH_);
			isMainFireStop_ = true;
		}

		// バーをおろす
		letterboxUI_.SetBarActive(false);

		if (rocketEffect_->GetEnergy() >= 1.0f) {
			if (isExplo_) { return; }
			AudioManager::GetInstance().Play(moonBreakSH_, 0.5f, false);
			isExplo_ = true;
			moonObject_->Break();
			explosionEffect_->Start({80.0f,50.0f,0.0f});
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

	rocketEffect_->rocketModel_.worldTransform_.transform_.translate = { 0.0f,5.5f,0.0f };

	// リセット
	moonObject_->Reset();

	isExplo_ = false;
	isSubFire_ = false;
	isMainFire_ = false;
	isMainFireStop_ = false;
	isPlay_ = true;
}

bool ResultMovieManager::IsFin() const {
	return resultMoveCamera_->IsFinished();
}