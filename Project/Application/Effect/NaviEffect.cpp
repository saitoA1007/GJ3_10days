#include "NaviEffect.h"
#include "FPSCounter.h"
#include "EasingManager.h"
#include "MyMath.h"
#include "ModelManager.h"
#include "TextureManager.h"
using namespace GameEngine;

NaviEffect::NaviEffect(GameEngine::Model* beamModel, uint32_t beamGH, GameEngine::Model* arrowModel) : beamModel_(beamModel), arrowModel_(arrowModel) {

	beamModel_.materialData_->textureHandle = beamGH;

	arrowModel_.materialData_->textureHandle = 0;
	arrowModel_.materialData_->enableLighting = false;
	arrowModel_.materialData_->color = { 1.0f,0.0f,0.0f,1.0f };

	// 登録
	Register();
}

void NaviEffect::Initialize() {
}

void NaviEffect::Update() {
	debugParame_->ApplyIfDirty();

	// uvの更新
	beamModel_.materialData_->uvTransform = Math::MakeWorldMatrixFromEulerRotation(uvtransform_.translate, uvtransform_.rotate, uvtransform_.scale);

	timer_ += FpsCounter::gameDeltaTime;
	if (timer_ <= 0.5f) {
		float localT = timer_ / 0.5f;
		arrowModel_.worldTransform_.transform_.translate.y = Lerp(2.0f, 4.0f, localT, EaseType::kEaseInQuad);
	} else {
		float localT = (timer_ - 0.5f) / 0.5f;
		arrowModel_.worldTransform_.transform_.translate.y = Lerp(4.0f, 2.0f, localT, EaseType::kEaseOutQuad);
	}

	if (timer_ >= 1.0f) {
		timer_ = 0.0f;
	}

	// 更新
	beamModel_.Update();
	arrowModel_.Update();
}

void NaviEffect::Draw() {
	arrowModel_.DrawRaytracing(renderQueue_);
	beamModel_.Draw(renderQueue_, Draw3dType::DefaultAdd, "WBOITAccumulatePass");
}

void NaviEffect::Register() {
	debugParame_ = std::make_unique<GameEngine::DebugParameter>("NaviEffect");
	std::string subGroup = "Beam";
	debugParame_->Register("BeamScaleX", beamModel_.worldTransform_.transform_.scale.x, 0, subGroup);
	debugParame_->Register("BeamScaleY", beamModel_.worldTransform_.transform_.scale.y, 0, subGroup);
	debugParame_->Register("Color", beamModel_.materialData_->color, 0, subGroup);
	debugParame_->Register("uvScale", uvtransform_.scale, 0, subGroup);
	debugParame_->Register("uvPos", uvtransform_.translate, 0, subGroup);
	debugParame_->Register("ArrowScale", arrowModel_.worldTransform_.transform_.scale);
	debugParame_->Apply();
}

void NaviEffect::Start(Vector3 startPos, Vector3 endPos) {

	arrowModel_.worldTransform_.transform_.translate = endPos;
	arrowModel_.worldTransform_.transform_.translate.y = 0.0f;

	Vector3 dir = endPos - startPos;
	dir.y = 0.0f;

	// 位置
	beamModel_.worldTransform_.transform_.translate = startPos;
	beamModel_.worldTransform_.transform_.translate.y = 0.5f;
	// サイズ
	float scale = dir.Length();
	beamModel_.worldTransform_.transform_.scale.z = scale;
	// 回転
	dir.Normalize();
	beamModel_.worldTransform_.transform_.rotate.y = std::atan2f(dir.x, dir.z);
	beamModel_.worldTransform_.transform_.rotate.x = std::atan2(-dir.y, std::sqrt(dir.x * dir.x + dir.z * dir.z));

	uvtransform_.scale.x = std::max(1.0f, std::floor(scale * (20.0f / 10.0f)));
}
