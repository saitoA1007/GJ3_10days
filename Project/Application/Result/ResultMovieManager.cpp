#include "ResultMovieManager.h"
#include "Application/GameCamera/ResultMoveCamera.h"
#include "Application/Effect/Moonobject.h"
#include "Application/Effect/RocketEffect.h"
using namespace GameEngine;

ResultMovieManager::ResultMovieManager(ResultMoveCamera* camera, MoonObject* moonObject, RocketEffect* rocketEffect) {
	resultMoveCamera_ = camera;
	moonObject_ = moonObject;
	rocketEffect_ = rocketEffect;


	// 停止
	resultMoveCamera_->SetActive(false);
	moonObject_->SetActive(false);
	rocketEffect_->SetActive(false);
}

void ResultMovieManager::Initialize() {

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
		rocketEffect_->Start(1.1f);
		break;

	case ResultMoveCamera::Phase::kStop:

		if (rocketEffect_->GetEnergy() > 1.0f) {
			moonObject_->Break();
		}
		break;
	}
}

void ResultMovieManager::Draw() {

}

void ResultMovieManager::Start() {
	// 有効
	resultMoveCamera_->SetActive(true);
	moonObject_->SetActive(true);
	rocketEffect_->SetActive(true);

	// カメラ演出を開始
	resultMoveCamera_->Start();
	rocketEffect_->Reset();
}

bool ResultMovieManager::IsFin() const {
	return resultMoveCamera_->IsFinished();
}