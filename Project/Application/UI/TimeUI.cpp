#include "TimeUI.h"
#include "FPSCounter.h"
#include "EasingManager.h"
using namespace GameEngine;

TimeUI::TimeUI(uint32_t unitIceGH) {

	unitSprite_.textureHandle_ = unitIceGH;

	debugParame_.RegisterWorld("Base", baseWorld_);
	debugParame_.RegisterSprite("Bar", barSprite_);
	debugParame_.RegisterSprite("UnitIce", unitSprite_);
	debugParame_.Register("unitT", unitT_);
	debugParame_.Register("startPosX", startPosX_);
	debugParame_.Register("endPosX", endPosX_);
	debugParame_.Register("heightScale", heightScale_);
	debugParame_.Register("heightColor", heightColor_);
	debugParame_.Apply();

	// ペアレント
	unitSprite_.SetParent(&baseWorld_);
	barSprite_.SetParent(&baseWorld_);

	heightSprites_.resize(3);
	for (uint32_t i = 0; i < 3; ++i) {

		if (i == 0) {
			heightSprites_[i].position_ = { 0.0f,0.0f };
			heightSprites_[i].color_ = { 0.0f,0.2f,1.0f,1.0f };
		} else if (i == 1) {
			heightSprites_[i].position_ = { barSprite_.scale_.x * 0.5f, 0.0f };
			heightSprites_[i].color_ = { 1.0f,1.0f,0.0f,1.0f };
		} else {
			heightSprites_[i].position_ = { barSprite_.scale_.x,0.0f };
			heightSprites_[i].color_ = { 1.0f,0.0f,0.0f,1.0f };
		}

		heightSprites_[i].SetParent(&baseWorld_);
		heightSprites_[i].anchorPoint_ = { 0.5f,0.5f };
		heightSprites_[i].SetSize({ 1.0f,1.0f });
	}
}

void TimeUI::Initialize() {

}

void TimeUI::Update() {
	debugParame_.ApplyIfDirty();

	unitSprite_.position_.x = Lerp(startPosX_, endPosX_, unitT_, EaseType::kLinear);

	// 更新処理
	baseWorld_.UpdateTransformMatrix();
	unitSprite_.Update();
	barSprite_.Update();
	for (auto& sprite : heightSprites_) {
		sprite.scale_ = heightScale_;
		//sprite.color_ = heightColor_;
		sprite.Update();
	}
}

void TimeUI::Draw() {
	renderQueue_->SubmitSprite(&barSprite_);
	for (auto& sprite : heightSprites_) {
		renderQueue_->SubmitSprite(&sprite);
	}
	renderQueue_->SubmitSprite(&unitSprite_);
}
