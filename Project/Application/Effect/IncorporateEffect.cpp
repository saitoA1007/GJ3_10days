#include "IncorporateEffect.h"
#include <algorithm>
#include "FPSCounter.h"
#include "EasingManager.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "GameObjectManager.h"
#include "Application/Effect/BlackHoleEffect.h"
using namespace GameEngine;

IncorporateEffect::IncorporateEffect(GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager, GameEngine::GameObjectManager* objectManager) {
	auto* planeModel = modelManager->GetNameByModel("plane.obj");
	planeModel->SetDefaultIsEnableLight(false);
	blackHoleParticle_ = objectManager->AddObject<ParticleBehavior>("blackHoleParticle", 64, textureManager, planeModel);
	starParticle_ = objectManager->AddObject<ParticleBehavior>("starParticle", 16, textureManager, planeModel);
	ringParticle_ = objectManager->AddObject<ParticleBehavior>("ringParticle", 8, textureManager, planeModel);

	// 一度だけ発生させたいパーティクルのループを切る
	starParticle_->SetIsLoop(false);
	ringParticle_->SetIsLoop(false);

	// 無効化
	starParticle_->SetActive(false);
	ringParticle_->SetActive(false);
	blackHoleParticle_->SetActive(false);
}	

void IncorporateEffect::Initialize() {

}

void IncorporateEffect::Update() {
	debugParame_.ApplyIfDirty();

	// 再生していない時は更新しない
	if (!isPlay_) {
		return;
	}

	timer_ += FpsCounter::gameDeltaTime;

	float t = 0.0f;

	switch (phase_)
	{
	case IncorporateEffect::Phase::kIn: {
		t = GetProgress(0.0f, kInMaxTime_);

		// スター発生
		if (!isEmitStar_) {
			isEmitStar_ = true;
			starParticle_->SetActive(true);
			starParticle_->Emit(emitPos_);
		}

		float ringTimer = GetProgress(ringDelayTime_, ringSpreadTime_);

		// リング発生
		if (!isEmitRing_ && ringTimer >= 1.0f) {
			isEmitRing_ = true;
			ringParticle_->SetActive(true);
			ringParticle_->Emit(emitPos_);
		}

		if (t >= 1.0f) {
			phase_ = Phase::kMain;

			// ブラックホールの周りのパーティクルを発生
			blackHoleParticle_->SetActive(true);
			blackHoleParticle_->SetEmitterPos(emitPos_);
		}
		break;
	}

	case IncorporateEffect::Phase::kMain: {
		t = GetProgress(0.0f, kMainMaxTime_);

		// ブラックホールの出現
		if (t <= 0.2f) {
			float localT = t / 0.2f;
			float scale = Lerp(0.0f, endBlacHoleScale_, localT, EaseType::kEaseInOutQuad);
			blackHoleEffect_->baseWorld_.transform_.scale = { scale,scale,scale };
		}

		if (t >= 1.0f) {
			blackHoleParticle_->SetActive(false);
			phase_ = Phase::kEnd;
		}
		break;
	}

	case IncorporateEffect::Phase::kEnd:
		t = GetProgress(0.0f, kEndMaxTime_);

		float scale = Lerp(endBlacHoleScale_, 0.0f, t, EaseType::kEaseInOutQuad);
		blackHoleEffect_->baseWorld_.transform_.scale = { scale,scale,scale };

		if (t >= 1.0f) {
			
		}
		break;
	}
}

void IncorporateEffect::Draw() {

}

float IncorporateEffect::GetProgress(float delayTime, float maxTime) const {
	if (maxTime <= 0.0f) {
		return 1.0f;
	}
	return std::clamp((timer_ - delayTime) / maxTime, 0.0f, 1.0f);
}

void IncorporateEffect::Start(Vector3 pos, float scale) {
	

	endBlacHoleScale_ = scale;
}