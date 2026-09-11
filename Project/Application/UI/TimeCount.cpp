#include "TimeCount.h"
#include "FPSCounter.h"
#include "EasingManager.h"
using namespace GameEngine;

TimeCountUI::TimeCountUI(uint32_t oneGH, uint32_t twoGH, uint32_t threeGH) {

	oneGH_ = oneGH;
	twoGH_ = twoGH;
	threeGH_ = threeGH;

	numSprite_.textureHandle_ = threeGH_;
	numSprite_.anchorPoint_ = { 0.5f,0.5f };
	numSprite_.position_ = { 640.0f,360.0f };
	numSprite_.size_ = { 1.0f,1.0f };
	numSprite_.scale_= { 128.0f,128.0f };

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("TimeCount");
	debugParame_->RegisterSprite("numSprite", numSprite_);
	debugParame_->Apply();
}

void TimeCountUI::Initialize() {
	debugParame_->Apply();
	remainingTime_ = 0.0f;
	isVisible_ = false;
}

void TimeCountUI::Update() {
	debugParame_->ApplyIfDirty();

	// 残り3秒以内の間だけ表示
	isVisible_ = remainingTime_ > 0.0f && remainingTime_ <= 3.0f;
	if (!isVisible_) {
		return;
	}

	// 残り時間を切り上げて数字を決める（2.3秒→3、1.5秒→2、0.2秒→1）
	const int count = static_cast<int>(std::ceil(remainingTime_));
	switch (count) {
	case 3:  numSprite_.textureHandle_ = threeGH_; break;
	case 2:  numSprite_.textureHandle_ = twoGH_;   break;
	default: numSprite_.textureHandle_ = oneGH_;   break;
	}

	// 数字が切り替わってからの経過割合（0→1）
	const float t = static_cast<float>(count) - remainingTime_;
	// 切り替わった瞬間に大きく出して、元の大きさへ縮める
	const float rate = Lerp(popRate_, 1.0f, t, EaseType::kEaseOutBack);
	numSprite_.scale_ = { baseScale_.x * rate, baseScale_.y * rate };

	
	// 更新処理
	numSprite_.Update();
}

void TimeCountUI::Draw() {
	if (!isVisible_) {
		return;
	}
	// 描画
	renderQueue_->SubmitSprite(&numSprite_);
}