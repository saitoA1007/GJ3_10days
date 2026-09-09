#include "ExplosionEffect.h"
#include <algorithm>
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
	explosionRingParticle_ = objectManager->AddObject<ParticleBehavior>("ExplosionRingParticle", 16, textureManager, planeModel);
	explosionParticle_ = objectManager->AddObject<ParticleBehavior>("ExplosionParticle", 64, textureManager, planeModel);
	explosionLineParticle_ = objectManager->AddObject<ParticleBehavior>("ExplosionLineParticle", 64, textureManager, planeModel);

	// 一度だけ発生させたいのでループを切る
	explosionRingParticle_->SetIsLoop(false);
	explosionParticle_->SetIsLoop(false);
	explosionLineParticle_->SetIsLoop(false);

	// 無効化
	explosionRingParticle_->SetActive(false);
	explosionParticle_->SetActive(false);
	explosionLineParticle_->SetActive(false);

	// テクスチャを設定
	flashModel_.materialData_->textureHandle = textureManager->GetHandleByName("effectCircle.png");
	ringModel_.materialData_->textureHandle = textureManager->GetHandleByName("star.png");

	// ライティングを切る
	flashModel_.SetEnableLighting(false);
	ringModel_.SetEnableLighting(false);

	// 開始時は見えない状態にする
	flashModel_.materialData_->color.w = 0.0f;
	ringModel_.materialData_->color.w = 0.0f;

	// 登録
	Register();
}

void ExplosionEffect::Initialize() {

}

void ExplosionEffect::Update() {
	debugParame_.ApplyIfDirty();

	// 再生していない時は更新しない
	if (!isPlay_) {
		return;
	}

	// 経過時間を加算
	timer_ += FpsCounter::gameDeltaTime;

	// フラッシュ。一気に広がってすぐ消える
	float t = GetProgress(0.0f, flashScaleTime_);
	float scale = Lerp(startFlashScale_, endFlashScale_, t, EaseType::kEaseOutExpo);
	flashModel_.worldTransform_.transform_.scale = { scale,scale ,scale };
	flashModel_.materialData_->color = flashColor_;
	flashModel_.materialData_->color.w = Lerp(flashColor_.w, 0.0f, GetProgress(0.0f, flashFadeTime_), EaseType::kEaseInQuad);

	// リング。フラッシュの後から広がりながら消える
	t = GetProgress(ringDelayTime_, ringSpreadTime_);
	scale = Lerp(startRingScale_, endRingScale_, t, EaseType::kEaseOutCubic);
	ringModel_.worldTransform_.transform_.scale = { scale,scale ,scale };
	ringModel_.materialData_->color = ringColor_;
	ringModel_.materialData_->color.w = Lerp(ringColor_.w, 0.0f, t, EaseType::kEaseInQuad);

	// 爆発パーティクルを一度だけ発生させる
	if (!isEmit_ && timer_ >= particleDelayTime_) {
		isEmit_ = true;
		explosionRingParticle_->SetActive(true);
		explosionParticle_->SetActive(true);
		explosionLineParticle_->SetActive(true);
		explosionRingParticle_->Emit(emitPos_);
		explosionParticle_->Emit(emitPos_);
		explosionLineParticle_->Emit(emitPos_);
	}

	// 更新処理
	UpdateMatrix();

	// 再生終了
	if (timer_ >= playMaxTime_) {
		isPlay_ = false;
		explosionRingParticle_->SetActive(false);
		explosionParticle_->SetActive(false);
		explosionLineParticle_->SetActive(false);
		flashModel_.materialData_->color.w = 0.0f;
		ringModel_.materialData_->color.w = 0.0f;
	}
}

void ExplosionEffect::Draw() {
	// 再生していない時は描画しない
	if (!isPlay_) {
		return;
	}

	if (flashModel_.materialData_->color.w > 0.0f) {
		flashModel_.Draw(renderQueue_, Draw3dType::DefaultAdd, "WBOITAccumulatePass");
	}
	if (ringModel_.materialData_->color.w > 0.0f) {
		ringModel_.Draw(renderQueue_, Draw3dType::DefaultAdd, "WBOITAccumulatePass");
	}
}

void ExplosionEffect::Register() {
	std::string subGroup = "Ring";
	debugParame_.Register("DelayTime", ringDelayTime_, 0, subGroup);
	debugParame_.Register("SpreadTime", ringSpreadTime_, 1, subGroup);
	debugParame_.Register("StartScale", startRingScale_, 2, subGroup);
	debugParame_.Register("EndScale", endRingScale_, 3, subGroup);
	debugParame_.Register("Color", ringColor_, 4, subGroup);
	subGroup = "flash";
	debugParame_.Register("ScaleTime", flashScaleTime_, 0, subGroup);
	debugParame_.Register("FadeTime", flashFadeTime_, 1, subGroup);
	debugParame_.Register("StartScale", startFlashScale_, 2, subGroup);
	debugParame_.Register("EndScale", endFlashScale_, 3, subGroup);
	debugParame_.Register("Color", flashColor_, 4, subGroup);
	subGroup = "Particle";
	debugParame_.Register("DelayTime", particleDelayTime_, 0, subGroup);
	debugParame_.Register("PlayMaxTime", playMaxTime_, 1, subGroup);
	debugParame_.Apply();
}

void ExplosionEffect::Start(Vector3 pos) {
	// 再生を開始する
	isPlay_ = true;
	isEmit_ = false;
	timer_ = 0.0f;

	// 位置を設定
	emitPos_ = pos;
	explosionRingParticle_->SetEmitterPos(pos);
	explosionParticle_->SetEmitterPos(pos);
	explosionLineParticle_->SetEmitterPos(pos);

	ringModel_.worldTransform_.transform_.translate = pos;
	flashModel_.worldTransform_.transform_.translate = pos;

	// 開始時の見た目を反映させる
	flashModel_.worldTransform_.transform_.scale = { startFlashScale_,startFlashScale_ ,startFlashScale_ };
	flashModel_.materialData_->color = flashColor_;
	ringModel_.worldTransform_.transform_.scale = { startRingScale_,startRingScale_ ,startRingScale_ };
	ringModel_.materialData_->color = ringColor_;
	ringModel_.materialData_->color.w = 0.0f;

	UpdateMatrix();
}

float ExplosionEffect::GetProgress(float delayTime, float maxTime) const {
	if (maxTime <= 0.0f) {
		return 1.0f;
	}
	return std::clamp((timer_ - delayTime) / maxTime, 0.0f, 1.0f);
}

void ExplosionEffect::UpdateMatrix() {
	Matrix4x4 cameraMatrix = renderQueue_->GetMainCamera().GetWorldMatrix();
	if (renderQueue_->GetUseDebugCamera()) {
		cameraMatrix = renderQueue_->GetDebugCameraWorldMatrix();
	}
	// リング
	ringModel_.worldTransform_.UpdateWorldMatrix(
		Math::MakeBillboardMatrix(ringModel_.worldTransform_.transform_.scale, ringModel_.worldTransform_.transform_.translate, cameraMatrix));
	// フラッシュ
	flashModel_.worldTransform_.UpdateWorldMatrix(
		Math::MakeBillboardMatrix(flashModel_.worldTransform_.transform_.scale, flashModel_.worldTransform_.transform_.translate, cameraMatrix));
}
