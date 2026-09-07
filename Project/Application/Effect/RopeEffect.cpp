#include "RopeEffect.h"
#include "FPSCounter.h"
#include "EasingManager.h"
#include "MyMath.h"
#include "ModelManager.h"
#include "TextureManager.h"
using namespace GameEngine;

RopeEffect::RopeEffect(GameEngine::Model* beamModel, uint32_t beamGH) : beamModel_(beamModel) {

	beamModel_.materialData_->textureHandle = beamGH;

	// 登録
	Register();
}

void RopeEffect::Initialize() {

}

void RopeEffect::Update() {
	debugParame_->ApplyIfDirty();

	// スクロール
	uvtransform_.translate.x += scrollSpeed_ * FpsCounter::gameDeltaTime;

	// uvの更新
	beamModel_.materialData_->uvTransform = Math::MakeWorldMatrixFromEulerRotation(uvtransform_.translate, uvtransform_.rotate, uvtransform_.scale);

	// 更新
	beamModel_.Update();
}

void RopeEffect::Draw() {
	//basePlaneModel_.Draw(renderQueue_, Draw3dType::Default, "WBOITAccumulatePass");
	beamModel_.Draw(renderQueue_, Draw3dType::DefaultAdd, "WBOITAccumulatePass");
	//mainPlaneModel_.Draw(renderQueue_, Draw3dType::DefaultAdd, "WBOITAccumulatePass");
}

void RopeEffect::Register() {
	debugParame_ = std::make_unique<GameEngine::DebugParameter>("RopeEffect");
	std::string subGroup = "Beam";
	debugParame_->Register("BeamScaleX", beamModel_.worldTransform_.transform_.scale.x,0, subGroup);
	debugParame_->Register("BeamScaleY", beamModel_.worldTransform_.transform_.scale.y,0, subGroup);
	debugParame_->Register("Color", beamModel_.materialData_->color, 0, subGroup);
	debugParame_->Register("uvScale", uvtransform_.scale, 0, subGroup);
	debugParame_->Register("uvPos", uvtransform_.translate, 0, subGroup);
	debugParame_->Register("scrollSpeed", scrollSpeed_, 0, subGroup);
	debugParame_->Apply();
}

void RopeEffect::Start(Vector3 startPos, Vector3 endPos) {

	Vector3 dir = endPos - startPos;
	//dir.y = 0.0f;

	// 位置
	beamModel_.worldTransform_.transform_.translate = startPos;
	// サイズ
	float scale = dir.Length();
	beamModel_.worldTransform_.transform_.scale.z = scale;
	// 回転
	dir.Normalize();
	beamModel_.worldTransform_.transform_.rotate.y = std::atan2f(dir.x, dir.z);
	beamModel_.worldTransform_.transform_.rotate.x = std::atan2(-dir.y, std::sqrt(dir.x * dir.x + dir.z * dir.z));
}
