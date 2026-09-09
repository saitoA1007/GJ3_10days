#include "IncorporateEffect.h"
#include <algorithm>
#include "FPSCounter.h"
#include "EasingManager.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "GameObjectManager.h"
#include "Application/Effect/BlackHoleEffect.h"
//#include "ImguiManager.h"
using namespace GameEngine;

IncorporateEffect::IncorporateEffect(GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager, GameEngine::GameObjectManager* objectManager) {
	auto* planeModel = modelManager->GetNameByModel("plane.obj");
	planeModel->SetDefaultIsEnableLight(false);
	blackHoleParticle_ = objectManager->AddObject<ParticleBehavior>("blackHoleParticle", 64, textureManager, planeModel);
	starParticle_ = objectManager->AddObject<ParticleBehavior>("starParticle", 16, textureManager, planeModel);
	ringParticle_ = objectManager->AddObject<ParticleBehavior>("ringParticle", 8, textureManager, planeModel);
	afterRingParticle_ = objectManager->AddObject<ParticleBehavior>("afterRingParticle", 8, textureManager, planeModel);

	// 一度だけ発生させたいパーティクルのループを切る
	starParticle_->SetIsLoop(false);
	ringParticle_->SetIsLoop(false);
	afterRingParticle_->SetIsLoop(false);

	// 無効化
	starParticle_->SetActive(false);
	ringParticle_->SetActive(false);
	blackHoleParticle_->SetActive(false);
	afterRingParticle_->SetActive(false);

	// ブラックホール
	auto* sphereModel = modelManager->GetNameByModel("sphere.obj");
	auto* ringModel = modelManager->GetNameByModel("blackHoleRing.gltf");
	blackHoleEffect_ = objectManager->AddObject<BlackHoleEffect>(sphereModel, ringModel);
	blackHoleEffect_->baseWorld_.transform_.scale = { 0.0f,0.0f,0.0f };
	blackHoleEffect_->SetActive(false);

	// 登録
	Register();
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

	switch (phase_)
	{
	case IncorporateEffect::Phase::kIn: {
		float t = GetProgress(0.0f, kInMaxTime_);

		// スター発生
		if (!isEmitStar_) {
			isEmitStar_ = true;
			starParticle_->SetActive(true);
			starParticle_->Emit(emitPos_);
		}

		// リング発生。スターの後に少し遅らせて出す
		if (!isEmitRing_ && timer_ >= ringDelayTime_) {
			isEmitRing_ = true;
			ringParticle_->SetActive(true);
			ringParticle_->Emit(emitPos_);
		}

		// 出し切ったパーティクルは、全て消えた時点で更新を止める
		StopFinishedParticle(starParticle_, isEmitStar_, starFadeTime_);
		StopFinishedParticle(ringParticle_, isEmitRing_, ringDelayTime_ + ringSpreadTime_);

		if (t >= 1.0f) {
			ChangePhase(Phase::kMain);
			starParticle_->SetActive(false);
			ringParticle_->SetActive(false);

			// ブラックホールの周りのパーティクルを発生
			blackHoleParticle_->SetIsLoop(true);
			blackHoleParticle_->SetActive(true);
			blackHoleParticle_->SetEmitterPos(emitPos_);
			// 中心に吸い込まれるようにする
			blackHoleParticle_->SetAttractionTarget(emitPos_);

			// ブラックホールを表示
			SetBlackHoleScale(0.0f);
			blackHoleEffect_->SetActive(true);
		}
		break;
	}

	case IncorporateEffect::Phase::kMain: {
		float t = GetProgress(0.0f, kMainMaxTime_);

		// ブラックホールの出現。出しきった後はその大きさを保つ
		float appearT = GetProgress(0.0f, blackHoleAppearTime_);
		SetBlackHoleScale(Lerp(0.0f, endBlacHoleScale_, appearT, EaseType::kEaseOutBack));

		if (t >= 1.0f) {
			ChangePhase(Phase::kEnd);
			// 新しい発生だけを止めて、残っている分は自然に消えさせる
			blackHoleParticle_->SetIsLoop(false);

			afterRingParticle_->SetActive(true);
			afterRingParticle_->Emit(emitPos_);
		}
		break;
	}

	case IncorporateEffect::Phase::kEnd: {
		float t = GetProgress(0.0f, kEndMaxTime_);

		// ブラックホールを縮めて消す
		SetBlackHoleScale(Lerp(endBlacHoleScale_, 0.0f, t, EaseType::kEaseInBack));

		if (t >= 1.0f) {
			afterRingParticle_->SetActive(false);
			// ブラックホールを消す
			blackHoleEffect_->SetActive(false);

			// 周りのパーティクルが消えきってから演出を終了する
			if (blackHoleParticle_->GetCurrentNumInstance() == 0) {
				Stop();
			}
		}
		break;
	}
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
	// 再生を開始する
	isPlay_ = true;
	phase_ = Phase::kIn;
	timer_ = 0.0f;
	isEmitStar_ = false;
	isEmitRing_ = false;

	// 位置と大きさを設定
	emitPos_ = pos;
	endBlacHoleScale_ = scale;

	// パーティクルの発生位置を合わせる
	starParticle_->SetEmitterPos(pos);
	ringParticle_->SetEmitterPos(pos);
	afterRingParticle_->SetEmitterPos(pos);
	blackHoleParticle_->SetEmitterPos(pos);
	blackHoleParticle_->SetAttractionTarget(pos);
	blackHoleParticle_->SetShapeEmit(8.0f * scale);

	float escale = 10.0f * scale;
	starParticle_->SetScale({ escale, 1.5f * scale ,1.0f });

	escale = 5.0f * scale;
	afterRingParticle_->SetScale({ escale,escale,1.0f });

	float ss = -8.0f * scale;
	ringParticle_->SetSizeOverLifeTime(ss);

	// ブラックホールは入りの演出が終わってから出すので、この時点では消しておく
	blackHoleEffect_->baseWorld_.transform_.translate = pos;
	SetBlackHoleScale(0.0f);
	blackHoleEffect_->SetActive(false);

	starParticle_->SetActive(false);
	ringParticle_->SetActive(false);
	afterRingParticle_->SetActive(false);
	blackHoleParticle_->SetActive(false);
}

void IncorporateEffect::Stop() {
	// 再生を終了させて初期状態に戻す
	isPlay_ = false;
	phase_ = Phase::kIn;
	timer_ = 0.0f;
	isEmitStar_ = false;
	isEmitRing_ = false;

	starParticle_->SetActive(false);
	ringParticle_->SetActive(false);
	afterRingParticle_->SetActive(false);
	blackHoleParticle_->SetActive(false);
	// 次の再生でも発生できるようにループを戻す
	blackHoleParticle_->SetIsLoop(true);

	SetBlackHoleScale(0.0f);
	blackHoleEffect_->SetActive(false);
}

void IncorporateEffect::ChangePhase(Phase phase) {
	phase_ = phase;
	// 進行度はフェーズごとに求めるので、タイマーを数え直す
	timer_ = 0.0f;
}

void IncorporateEffect::SetBlackHoleScale(float scale) {
	blackHoleEffect_->baseWorld_.transform_.scale = { scale,scale,scale };
}

void IncorporateEffect::StopFinishedParticle(GameEngine::ParticleBehavior* particle, bool isEmitted, float endTime) const {
	if (!isEmitted || !particle->IsActive()) {
		return;
	}

	// 発生させた分が全て寿命を迎えていたら止める
	if (timer_ >= endTime && particle->GetCurrentNumInstance() == 0) {
		particle->SetActive(false);
	}
}

void IncorporateEffect::Register() {
	std::string subGroup = "Phase";
	debugParame_.Register("InMaxTime", kInMaxTime_, 0, subGroup);
	debugParame_.Register("MainMaxTime", kMainMaxTime_, 1, subGroup);
	debugParame_.Register("EndMaxTime", kEndMaxTime_, 2, subGroup);
	subGroup = "Star";
	debugParame_.Register("FadeTime", starFadeTime_, 0, subGroup);
	subGroup = "Ring";
	debugParame_.Register("DelayTime", ringDelayTime_, 0, subGroup);
	debugParame_.Register("SpreadTime", ringSpreadTime_, 1, subGroup);
	subGroup = "BlackHole";
	debugParame_.Register("AppearTime", blackHoleAppearTime_, 0, subGroup);
	debugParame_.Apply();
}
