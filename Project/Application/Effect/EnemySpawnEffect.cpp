#include "EnemySpawnEffect.h"
#include "FPSCounter.h"
#include "EasingManager.h"
#include "MyMath.h"
#include "ModelManager.h"
#include "TextureManager.h"
using namespace GameEngine;

EnemySpawnEffect::EnemySpawnEffect(GameEngine::Model* model, GameEngine::Model* planeModel, GameEngine::Model* waveModel, GameEngine::TextureManager* textureManager)
	: beamModel_(model), basePlaneModel_(planeModel), outPlaneModel_(planeModel), waveModel_(waveModel), mainPlaneModel_(planeModel) {

	uint32_t outGH = textureManager->GetHandleByName("Ring_01.png");
	uint32_t waveGH = textureManager->GetHandleByName("Power.png");
	uint32_t mainGH = textureManager->GetHandleByName("tilingNoise.png");

	outPlaneModel_.materialData_->textureHandle = outGH;
	waveModel_.materialData_->textureHandle = waveGH;
	mainPlaneModel_.materialData_->textureHandle = mainGH;
	basePlaneModel_.materialData_->textureHandle = 0;

	// ペアレント化
	beamModel_.worldTransform_.SetParent(&baseWorld_);
	basePlaneModel_.worldTransform_.SetParent(&baseWorld_);
	outPlaneModel_.worldTransform_.SetParent(&baseWorld_);
	waveModel_.worldTransform_.SetParent(&baseWorld_);
	mainPlaneModel_.worldTransform_.SetParent(&baseWorld_);

	baseWorld_.transform_.scale = { 2.0f,32.0f,2.0f };

	// 登録
	Register();
}

void EnemySpawnEffect::Initialize() {

}

void EnemySpawnEffect::Update() {
	debugParame_->ApplyIfDirty();

	switch (phase_)
	{
	case EnemySpawnEffect::Phase::kIn: {

		timer_ += FpsCounter::gameDeltaTime / kInMaxTime_;

		baseWorld_.transform_.translate.y = Lerp(kInStartPos_, 32.0f, timer_, EaseType::kEaseInCubic);

		float scale = Lerp(0.0f, kInEndScale_, timer_, EaseType::kEaseInCubic);
		baseWorld_.transform_.scale.x = scale;
		baseWorld_.transform_.scale.z = scale;

		if (timer_ >= 1.0f) {
			phase_ = Phase::kEnd;
			timer_ = 0.0f;
			baseWorld_.transform_.translate.y = 32.0f;
		}

		break;
	}

	case EnemySpawnEffect::Phase::kEnd: {
		timer_ += FpsCounter::gameDeltaTime / kEndMaxTime_;

		float scale = Lerp(kInEndScale_, 0.0f, timer_, EaseType::kEaseInOutBounce);
		baseWorld_.transform_.scale.x = scale;
		baseWorld_.transform_.scale.z = scale;

		waveModel_.worldTransform_.transform_.rotate.y += 50.0f * FpsCounter::gameDeltaTime;

		if (timer_ >= 1.0f) {
			scale = 0.0f;
			baseWorld_.transform_.scale.x = scale;
			baseWorld_.transform_.scale.z = scale;
			waveModel_.worldTransform_.transform_.rotate.y = 0.0f;
			// 終了
			isActive_ = false;
		}
		break;
	}
	}

	mainUvtransform_.translate.y += scrollSpeed_ * FpsCounter::gameDeltaTime;

	Matrix4x4 cameraMatrix = renderQueue_->GetMainCamera().GetWorldMatrix();
	if (renderQueue_->GetUseDebugCamera()) {
		cameraMatrix = renderQueue_->GetDebugCameraWorldMatrix();
	}

	baseWorld_.UpdateWorldMatrix(
		Math::MakeYAxisBillboardMatrix(baseWorld_.transform_.scale, baseWorld_.transform_.translate, cameraMatrix));

	outPlaneModel_.Update();
	basePlaneModel_.Update();
	beamModel_.Update();
	waveModel_.Update();
	mainPlaneModel_.Update();
	// uvの更新
	outPlaneModel_.materialData_->uvTransform = Math::MakeWorldMatrixFromEulerRotation(outUvtransform_.translate, outUvtransform_.rotate, outUvtransform_.scale);
	waveModel_.materialData_->uvTransform = Math::MakeWorldMatrixFromEulerRotation(waveUvtransform_.translate, waveUvtransform_.rotate, waveUvtransform_.scale);
	mainPlaneModel_.materialData_->uvTransform = Math::MakeWorldMatrixFromEulerRotation(mainUvtransform_.translate, mainUvtransform_.rotate, mainUvtransform_.scale);
}

void EnemySpawnEffect::Draw() {
	beamModel_.DrawRaytracing(renderQueue_);
	basePlaneModel_.Draw(renderQueue_,Draw3dType::Default, "WBOITAccumulatePass");
	outPlaneModel_.Draw(renderQueue_, Draw3dType::DefaultAdd, "WBOITAccumulatePass");
	mainPlaneModel_.Draw(renderQueue_, Draw3dType::DefaultAdd, "WBOITAccumulatePass");

	if (phase_ == Phase::kEnd) {
		waveModel_.Draw(renderQueue_);
	}
}

void EnemySpawnEffect::Register() {
	debugParame_ = std::make_unique<GameEngine::DebugParameter>("EnemySpawnEffect");
	debugParame_->Register(" InMaxTime_", kInMaxTime_);
	debugParame_->Register(" EndMaxTime", kEndMaxTime_);
	debugParame_->Register(" InStartPos", kInStartPos_);
	debugParame_->Register(" InEndScale", kInEndScale_);
	std::string subGroup = "Beam";
	debugParame_->RegisterWorld("Beam", beamModel_.worldTransform_, subGroup);
	debugParame_->Register("Color", beamModel_.materialData_->color, 0, subGroup);
	subGroup = "BasePlane";
	debugParame_->RegisterWorld("plane", basePlaneModel_.worldTransform_, subGroup);
	debugParame_->Register("Color", basePlaneModel_.materialData_->color, 0, subGroup);
	subGroup = "OutLine";
	debugParame_->RegisterWorld("outLine", outPlaneModel_.worldTransform_, subGroup);
	debugParame_->Register("uvScale", outUvtransform_.scale,0, subGroup);
	debugParame_->Register("uvPos", outUvtransform_.translate,0, subGroup);
	debugParame_->Register("Color", outPlaneModel_.materialData_->color, 0, subGroup);
	subGroup = "Wave";
	debugParame_->RegisterWorld("", waveModel_.worldTransform_, subGroup);
	debugParame_->Register("uvScale", waveUvtransform_.scale, 0, subGroup);
	debugParame_->Register("uvrotate", waveUvtransform_.rotate, 0, subGroup);
	debugParame_->Register("uvPos", waveUvtransform_.translate, 0, subGroup);
	debugParame_->Register("Color", waveModel_.materialData_->color, 0, subGroup);
	subGroup = "mainPlane";
	debugParame_->RegisterWorld("", mainPlaneModel_.worldTransform_, subGroup);
	debugParame_->Register("uvScale", mainUvtransform_.scale, 0, subGroup);
	debugParame_->Register("uvPos", mainUvtransform_.translate, 0, subGroup);
	debugParame_->Register("Color", mainPlaneModel_.materialData_->color, 0, subGroup);
	debugParame_->Register("ScrollSpeed", scrollSpeed_, 1, subGroup);
	debugParame_->Apply();
}
